#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <unordered_map>

const int screenWidth = 1280;
const int screenHeight = 720;

typedef struct {
    Vector2 position;
} Transform2D;

typedef struct {
    Vector2 value;
} Velocity;

typedef struct {
    float current;
    float max;
} Health;

typedef struct {
    float radius;
} CircleCollider;

typedef struct {
    float damage;
    float range;
    float cooldown;
    float timer;
} MeleeAttack;

enum class AIStateType {
    Chase,
    Attack
};

typedef struct {
    AIStateType state;
} AIState;

struct PlayerControl {};

using Entity = int;

struct World {
    int nextEntity = 0;
    std::vector<Entity> entities;

    std::unordered_map<Entity, Transform2D> transforms;
    std::unordered_map<Entity, Velocity> velocities;
    std::unordered_map<Entity, Health> healths;
    std::unordered_map<Entity, CircleCollider> colliders;
    std::unordered_map<Entity, MeleeAttack> meleeAttacks;
    std::unordered_map<Entity, AIState> aiStates;
    std::unordered_map<Entity, PlayerControl> playerControls;
};

Entity CreateEntity(World& world) {
    Entity e = world.nextEntity++;
    world.entities.push_back(e);
    return e;
}

void MovementSystem(World& world, float dt) {
    for (auto& [entity, velocity] : world.velocities) {
        if (world.transforms.find(entity) != world.transforms.end()) {
            world.transforms[entity].position.x += velocity.value.x * dt;
            world.transforms[entity].position.y += velocity.value.y * dt;
        }
    }
}

void InputSystem(World& world) {
    for (auto& [entity, control] : world.playerControls) {
        Vector2 dir = { 0, 0 };
        
        if (IsKeyDown(KEY_W)) dir.y -= 1;
        if (IsKeyDown(KEY_S)) dir.y += 1;
        if (IsKeyDown(KEY_A)) dir.x -= 1;
        if (IsKeyDown(KEY_D)) dir.x += 1;

        if (world.velocities.find(entity) != world.velocities.end()) {
            world.velocities[entity].value = Vector2Scale(Vector2Normalize(dir), 200);
        }
    }
}

void RenderSystem(World& world) {
    for (auto& [entity, transform] : world.transforms) {
        float radius = 10;

        if (world.colliders.find(entity) != world.colliders.end()) {
            radius = world.colliders[entity].radius;
        }

        DrawCircleV(transform.position, radius, RED);
    }
}

int main() {

    InitWindow(screenWidth, screenHeight, "game");

    SetTargetFPS(60);

    World world;

    // player
    Entity monster = CreateEntity(world);
    world.transforms[monster] = { { screenWidth/2.0f, screenHeight/2.0f } };
    world.velocities[monster] = { { 0, 0 } };
    world.healths[monster] = { 100, 100 };
    world.colliders[monster] = { 20 };
    world.meleeAttacks[monster] = { 20, 40, 1.0f, 0.0f };
    world.playerControls[monster] = {};

    while (!WindowShouldClose()) {

        float dt = GetFrameTime();

        InputSystem(world);
        MovementSystem(world, dt);

        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawFPS(10, 10);
        
        RenderSystem(world);
        
        EndDrawing();

    }

    CloseWindow();
    return 0;
}
