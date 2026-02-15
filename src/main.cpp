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

typedef struct {
    Color color;
} Renderable;

typedef struct {
    float timer;
} HitFlash;

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
    std::unordered_map<Entity, Renderable> renderables;
    std::unordered_map<Entity, HitFlash> hitFlashes;
};

Entity CreateEntity(World& world) {
    Entity e = world.nextEntity++;
    world.entities.push_back(e);
    return e;
}

Entity SpawnHero(World& world, Vector2 position) {
    Entity hero = CreateEntity(world);

    world.transforms[hero] = { position };
    world.velocities[hero] = { {0, 0} };
    world.healths[hero] = { 50, 50 };
    world.colliders[hero] = { 12 };
    world.meleeAttacks[hero] = { 
        5.0f,   // damage
        20.0f,  // range
        1.0f,   // cooldown
        0.0f    // timer
    };
    world.aiStates[hero] = { AIStateType::Chase };
    world.renderables[hero] = { BLUE };

    return hero;
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

        if (world.renderables.find(entity) == world.renderables.end()) continue;

        Color color = world.renderables[entity].color;
        float radius = 10;

        // hit flash
        if (world.hitFlashes.find(entity) != world.hitFlashes.end()) {
            color = WHITE;
        }

        if (world.colliders.find(entity) != world.colliders.end()) {
            radius = world.colliders[entity].radius;
        }

        DrawCircleV(transform.position, radius, color);

    }
}

void AISystem(World& world, Entity monster) {
    if (world.transforms.find(monster) == world.transforms.end()) return;

    Vector2 monsterPos = world.transforms[monster].position;

    for (auto& [entity, ai] : world.aiStates) {
        // have transform, velocity and attack
        if (world.transforms.find(entity) == world.transforms.end()) continue;
        if (world.velocities.find(entity) == world.velocities.end()) continue;
        if (world.meleeAttacks.find(entity) == world.meleeAttacks.end()) continue;
        
        Vector2 heroPos = world.transforms[entity].position;

        float dist = Vector2Distance(heroPos, monsterPos);
        float attackRnage = world.meleeAttacks[entity].range;

        if (dist > attackRnage) {
            ai.state = AIStateType::Chase;

            Vector2 dir = Vector2Subtract(monsterPos, heroPos);
            dir = Vector2Normalize(dir);

            world.velocities[entity].value = Vector2Scale(dir, 120);
        } else {
            ai.state = AIStateType::Attack;

            world.velocities[entity].value = { 0, 0 };
        }
    }
}

Entity FindClosestHero(World& world, Entity monster) {
    if (world.transforms.find(monster) == world.transforms.end()) return -1;

    Vector2 monsterPos = world.transforms[monster].position;

    float closestDist = 999999;
    Entity closest = -1;

    for (auto& [entity, ai] : world.aiStates) {
        if (world.transforms.find(entity) == world.transforms.end()) continue;

        float dist = Vector2Distance(monsterPos, world.transforms[entity].position);

        if (dist < closestDist) {
            closestDist = dist;
            closest = entity;
        }
    }

    return closest;
}

void AttackSystem(World& world, Entity monster, float dt) {

    for (auto& [entity, attack] : world.meleeAttacks) {
        if (attack.timer > 0) {
            attack.timer -= dt;
        }

        if (world.transforms.find(entity) == world.transforms.end()) continue;

        Entity target = -1;

        // hero attack monster
        if (world.aiStates.find(entity) != world.aiStates.end()) {
            target = monster;
        }
        // monster attack hero
        else if (world.playerControls.find(entity) != world.playerControls.end()) {
            target = FindClosestHero(world, monster);
        }

        if (target == -1) continue;

        // have transform, health
        if (world.transforms.find(entity) == world.transforms.end()) continue;
        if (world.healths.find(monster) == world.healths.end()) continue;

        Vector2 posA = world.transforms[entity].position;
        Vector2 posB = world.transforms[target].position;

        float distance = Vector2Distance(posA, posB);

        if (attack.timer <= 0.0f && distance <= attack.range) {
            world.healths[target].current -= attack.damage;
            attack.timer = attack.cooldown;
            world.hitFlashes[target] = { 0.1f };
        }
    }
}

void HealthSystem(World& world) {
    for (auto it = world.healths.begin(); it != world.healths.end(); ) {
        if (it->second.current <= 0) {
            Entity dead = it->first;

            // remove it
            world.transforms.erase(dead);
            world.velocities.erase(dead);
            world.colliders.erase(dead);
            world.meleeAttacks.erase(dead);
            world.aiStates.erase(dead);

            it = world.healths.erase(it);
        } else {
            ++it;
        }
    }
}

void HitFlashSystem(World& world, float dt) {
    for (auto it = world.hitFlashes.begin(); it != world.hitFlashes.end(); ){
        it->second.timer -= dt;

        if (it->second.timer <= 0) {
            it = world.hitFlashes.erase(it);
        } else {
            ++it;
        }
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
    world.renderables[monster] = { RED };

    for (int i = 0; i < 5; i++) {
        Vector2 pos = {
            200 + i * 60.0f,
            200
        };
        SpawnHero(world, pos);
    }

    while (!WindowShouldClose()) {

        float dt = GetFrameTime();

        InputSystem(world);
        AISystem(world, monster);
        MovementSystem(world, dt);
        AttackSystem(world, monster, dt);
        HealthSystem(world);
        HitFlashSystem(world, dt);

        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawFPS(10, 10);
        
        RenderSystem(world);
        
        EndDrawing();

    }

    CloseWindow();
    return 0;
}
