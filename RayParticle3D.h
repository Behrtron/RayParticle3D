#pragma once

#include "raylib.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <execution>
#include <optional>

// Utility structure to represent a range of float values
struct FloatRange {
    float min, max;

    float RandomValue() const {
        return ((float)GetRandomValue(0, RAND_MAX) / (float)RAND_MAX) * (max - min) + min;
    }
};

// Utility structure to represent a range of int values
struct IntRange {
    int min, max;

    int RandomValue() const {
        return GetRandomValue(min, max);
    }
};

// Configuration structure for particle emitters
struct EmitterConfig {
    Vector3 direction;
    FloatRange velocity, directionAngle, velocityAngle, offset, originAcceleration, age;
    IntRange burst;
    size_t capacity, emissionRate;
    Vector3 origin, externalAcceleration;
    Color startColor, endColor;
    BlendMode blendMode;
    Model model;  // 3D model to be used for particles
    float gravity;  // Gravity affecting the particles
    bool collision;  // Enable collision detection
};

// Particle structure, aligned for performance and SIMD optimization
struct alignas(32) Particle {
    Vector3 origin, position, velocity, externalAcceleration;
    float originAcceleration, age, ttl, scale;
    bool active;

    bool IsExpired() const {
        return age > ttl;
    }

    void Init(const EmitterConfig& cfg) {
        age = 0;
        origin = cfg.origin;

        float randomAngleX = cfg.directionAngle.RandomValue();
        float randomAngleY = cfg.velocityAngle.RandomValue();
        Vector3 randomDir = Rotate(cfg.direction, randomAngleX, randomAngleY);

        velocity = Vector3Scale(randomDir, cfg.velocity.RandomValue());
        position = Vector3Add(cfg.origin, Vector3Scale(randomDir, cfg.offset.RandomValue()));
        originAcceleration = cfg.originAcceleration.RandomValue();
        externalAcceleration = cfg.externalAcceleration;
        ttl = cfg.age.RandomValue();
        scale = 1.0f;
        active = true;
    }

    void Update(float dt, const EmitterConfig& cfg) {
        if (!active) return;

        age += dt;
        if (IsExpired()) {
            active = false;
            return;
        }

        velocity.y -= cfg.gravity * dt;
        Vector3 toOrigin = Vector3Normalize(Vector3Subtract(origin, position));
        velocity = Vector3Add(velocity, Vector3Scale(toOrigin, originAcceleration * dt));
        velocity = Vector3Add(velocity, Vector3Scale(externalAcceleration, dt));
        position = Vector3Add(position, Vector3Scale(velocity, dt));

        if (cfg.collision && position.y <= 0.0f) {
            position.y = 0.0f;
            velocity.y *= -0.5f; // Simple bounce with energy loss
        }

        scale = 1.0f / (Vector3Distance(position, cfg.origin) * 0.1f + 1.0f);
    }

private:
    static Vector3 Rotate(const Vector3& v, float angleX, float angleY) {
        static const float radConversion = DEG2RAD;
        float radY = angleY * radConversion;
        Vector3 rotatedY = {
            cosf(radY) * v.x + sinf(radY) * v.z,
            v.y,
            -sinf(radY) * v.x + cosf(radY) * v.z
        };

        float radX = angleX * radConversion;
        Vector3 rotatedX = {
            rotatedY.x,
            cosf(radX) * rotatedY.y - sinf(radX) * rotatedY.z,
            sinf(radX) * rotatedY.y + cosf(radX) * rotatedY.z
        };

        return rotatedX;
    }
};

// Particle emitter class, managing the creation, updating, and drawing of particles
class Emitter {
public:
    Emitter(EmitterConfig cfg) 
        : config(std::move(cfg)), mustEmit(0), isEmitting(false) {
        config.direction = Vector3Normalize(config.direction);
        particles.resize(config.capacity);
    }

    void Start() { isEmitting = true; }
    void Stop() { isEmitting = false; }

    void Burst() {
        size_t emitted = 0;
        int amount = config.burst.RandomValue();
        for (auto& p : particles) {
            if (!p.active) {
                p.Init(config);
                if (++emitted >= amount) return;
            }
        }
    }

    unsigned long Update(float dt) {
        size_t emitNow = 0;
        unsigned long counter = 0;
        if (isEmitting) {
            mustEmit += dt * static_cast<float>(config.emissionRate);
            emitNow = static_cast<size_t>(mustEmit);
        }

        std::for_each(std::execution::par_unseq, particles.begin(), particles.end(), [&](Particle& p) {
            if (p.active) {
                p.Update(dt, config);
                ++counter;
            } else if (isEmitting && emitNow > 0) {
                p.Init(config);
                p.Update(dt, config);
                --emitNow;
                --mustEmit;
                ++counter;
            }
        });

        return counter;
    }

    void Draw() const {
        BeginBlendMode(config.blendMode);
        for (const auto& p : particles) {
            if (p.active) {
                Matrix translation = MatrixTranslate(p.position.x, p.position.y, p.position.z);
                Matrix scaling = MatrixScale(p.scale, p.scale, p.scale);
                Model modelCopy = config.model; // Avoid modifying the original model's transform
                modelCopy.transform = MatrixMultiply(scaling, translation);
                DrawModel(modelCopy, p.position, 1.0f, LinearFade(config.startColor, config.endColor, p.age / p.ttl));
            }
        }
        EndBlendMode();
    }

private:
    EmitterConfig config;
    float mustEmit;
    bool isEmitting;
    std::vector<Particle> particles;

    static Color LinearFade(const Color& c1, const Color& c2, float fraction) {
        return Color{
            static_cast<unsigned char>((c2.r - c1.r) * fraction + c1.r),
            static_cast<unsigned char>((c2.g - c1.g) * fraction + c1.g),
            static_cast<unsigned char>((c2.b - c1.b) * fraction + c1.b),
            static_cast<unsigned char>((c2.a - c1.a) * fraction + c1.a)
        };
    }
};

// Particle system class, managing multiple emitters
class ParticleSystem {
public:
    void Register(std::unique_ptr<Emitter> emitter) {
        emitters.push_back(std::move(emitter));
    }

    void SetOrigin(const Vector3& newOrigin) {
        for (auto& e : emitters) {
            e->SetOrigin(newOrigin);
        }
    }

    void Start() {
        for (auto& e : emitters) {
            e->Start();
        }
    }

    void Stop() {
        for (auto& e : emitters) {
            e->Stop();
        }
    }

    void Burst() {
        for (auto& e : emitters) {
            e->Burst();
        }
    }

    unsigned long Update(float dt) {
        unsigned long counter = 0;

        std::for_each(std::execution::par_unseq, emitters.begin(), emitters.end(), [&](auto& e) {
            counter += e->Update(dt);
        });

        return counter;
    }

    void Draw() const {
        for (const auto& e : emitters) {
            e->Draw();
        }
    }

private:
    std::vector<std::unique_ptr<Emitter>> emitters;
};

#endif // PARTICLE_SYSTEM_H
