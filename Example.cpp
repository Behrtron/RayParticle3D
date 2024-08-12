#include "raylib.h"
#include "ParticleSystem.h"

int main() {
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Realistic Fire Effect with Modern Particle System");

    Camera camera = { 0 };
    camera.position = (Vector3){ 5.0f, 5.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.type = CAMERA_PERSPECTIVE;

    // Load models to be used as particles
    Model fireModel = LoadModelFromMesh(GenMeshPlane(0.2f, 0.2f, 1, 1));
    Model smokeModel = LoadModelFromMesh(GenMeshPlane(0.3f, 0.3f, 1, 1));
    Model emberModel = LoadModelFromMesh(GenMeshSphere(0.05f, 8, 8));

    // Fire layer emitter
    EmitterConfig fireEmitterConfig = {
        .direction = { 0.0f, 1.0f, 0.0f },
        .velocity = { 1.0f, 2.0f },
        .directionAngle = { -10.0f, 10.0f },
        .velocityAngle = { -5.0f, 5.0f },
        .offset = { 0.0f, 0.5f },
        .originAcceleration = { 0.2f, 0.5f },
        .age = { 0.5f, 1.5f },
        .burst = { 0, 0 },
        .capacity = 500,
        .emissionRate = 200,
        .origin = { 0.0f, -2.0f, 0.0f },
        .externalAcceleration = { 0.0f, 0.0f, 0.0f },
        .startColor = Color{ 255, 150, 0, 255 },
        .endColor = Color{ 255, 50, 0, 0 },
        .blendMode = BLEND_ADDITIVE,
        .model = fireModel,
        .gravity = 0.0f,
        .collision = false
    };

    // Smoke layer emitter
    EmitterConfig smokeEmitterConfig = {
        .direction = { 0.0f, 1.0f, 0.0f },
        .velocity = { 0.5f, 1.5f },
        .directionAngle = { -20.0f, 20.0f },
        .velocityAngle = { -10.0f, 10.0f },
        .offset = { 0.0f, 0.5f },
        .originAcceleration = { 0.0f, 0.0f },
        .age = { 2.0f, 4.0f },
        .burst = { 0, 0 },
        .capacity = 300,
        .emissionRate = 100,
        .origin = { 0.0f, -2.0f, 0.0f },
        .externalAcceleration = { 0.0f, 0.0f, 0.0f },
        .startColor = Color{ 100, 100, 100, 150 },
        .endColor = Color{ 50, 50, 50, 0 },
        .blendMode = BLEND_ALPHA,
        .model = smokeModel,
        .gravity = 0.0f,
        .collision = false
    };

    // Ember layer emitter
    EmitterConfig emberEmitterConfig = {
        .direction = { 0.0f, 1.0f, 0.0f },
        .velocity = { 1.5f, 3.0f },
        .directionAngle = { -15.0f, 15.0f },
        .velocityAngle = { -10.0f, 10.0f },
        .offset = { 0.0f, 0.5f },
        .originAcceleration = { 0.0f, 0.0f },
        .age = { 5.0f, 15.0f },
        .burst = { 0, 0 },
        .capacity = 100,
        .emissionRate = 50,
        .origin = { 0.0f, -2.0f, 0.0f },
        .externalAcceleration = { 0.0f, 0.0f, 0.0f },
        .startColor = Color{ 255, 100, 0, 255 },
        .endColor = Color{ 255, 100, 0, 0 },
        .blendMode = BLEND_ADDITIVE,
        .model = emberModel,
        .gravity = 0.1f,  // Gravity to make embers fall down
        .collision = true
    };


    // Create a single particle system and add all emitters to it
    ParticleSystem fireEffectSystem;
    fireEffectSystem.Register(std::make_unique<Emitter>(fireEmitterConfig));
    fireEffectSystem.Register(std::make_unique<Emitter>(smokeEmitterConfig));
    fireEffectSystem.Register(std::make_unique<Emitter>(emberEmitterConfig));
    fireEffectSystem.Start();

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Update the particle system
        float dt = GetFrameTime();
        fireEffectSystem.Update(dt);

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);

        DrawGrid(10, 1.0f);

        // Draw all emitters (fire, smoke, embers)
        fireEffectSystem.Draw();

        EndMode3D();

        DrawText("Realistic Fire Effect with Multiple Emitters", 10, 10, 20, WHITE);

        EndDrawing();
    }

    // Clean up
    UnloadModel(fireModel);
    UnloadModel(smokeModel);
    UnloadModel(emberModel);
    CloseWindow();

    return 0;
}
