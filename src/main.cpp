#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <unordered_map>

const int screenWidth = 1280;
const int screenHeight = 720;

enum class GameState {
    Playing,
    GameOver
};

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
    int layer; // 0 hero, 1 monster, 2 obstacle
    bool isStatic;
} CircleCollider;

typedef struct {
    float damage;
    float range;
    float cooldown;
    float timer;
} MeleeAttack;

typedef struct {
    bool wantsToAttack;
} AttackIntent;

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

    std::unordered_map<Entity, Transform2D> transforms;
    std::unordered_map<Entity, Velocity> velocities;
    std::unordered_map<Entity, Health> healths;
    std::unordered_map<Entity, CircleCollider> colliders;
    std::unordered_map<Entity, MeleeAttack> meleeAttacks;
    std::unordered_map<Entity, AIState> aiStates;
    std::unordered_map<Entity, PlayerControl> playerControls;
    std::unordered_map<Entity, Renderable> renderables;
    std::unordered_map<Entity, HitFlash> hitFlashes;
    std::unordered_map<Entity, AttackIntent> attackIntentes;

    GameState gameState = GameState::Playing;
    float hitStopTimer = 0.0f;
    float shakeTime = 0.0f;
    float shakeMagnitude = 0.0f;
};

Entity CreateEntity(World& world) {
    Entity e = world.nextEntity++;
    return e;
}

Entity SpawnHero(World& world, Vector2 position) {
    Entity hero = CreateEntity(world);

    world.transforms[hero] = { position };
    world.velocities[hero] = { {0, 0} };
    world.healths[hero] = { 50, 50 };
    world.colliders[hero] = { 12.0f, 0, false };
    world.meleeAttacks[hero] = { 
        5.0f,   // damage
        32.5f,  // range
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

bool shouldCollide(const CircleCollider& a, const CircleCollider& b) {
    // for now true, later maybe changes
    return true;
}

void CollisionSystem(World& world) {
    for (auto& [e1, c1] : world.colliders) {
        if (world.transforms.find(e1) == world.transforms.end()) continue;

        Transform2D& t1 = world.transforms[e1];

        for (auto& [e2, c2] : world.colliders) {
            if (e1 >= e2) continue;

            if (world.transforms.find(e2) == world.transforms.end()) continue;

            if (!shouldCollide(c1, c2)) continue;

            Transform2D& t2 = world.transforms[e2];

            Vector2 diff = Vector2Subtract(t2.position, t1.position);
            float dist = Vector2Length(diff);
            if (dist <= 0.0001f) continue;
            float minDist = c1.radius + c2.radius;

            if (dist < minDist && dist > 0.0001f) {
                float overlap = minDist - dist;
                Vector2 normal = Vector2Scale(diff, 1.0f / dist);

                // static vs static - ignore
                if (c1.isStatic && c2.isStatic) continue;

                // static vs dynamic
                else if (c1.isStatic && !c2.isStatic) {
                    t2.position = Vector2Add(
                        t2.position,
                        Vector2Scale(normal, overlap * 0.5f)
                    );
                } else if (!c1.isStatic && c2.isStatic) {
                    t1.position = Vector2Subtract(
                        t1.position,
                        Vector2Scale(normal, overlap * 0.5f)
                    );
                }
                // dynamic vs dynamic
                else {
                    t1.position = Vector2Subtract(
                        t1.position,
                        Vector2Scale(normal, overlap * 0.5f)
                    );

                    t2.position = Vector2Add(
                        t2.position,
                        Vector2Scale(normal, overlap * 0.5f)
                    );
                }
            }
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

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            world.attackIntentes[entity].wantsToAttack = true;
        }

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

        // hp bar above hero alone
        if (world.healths.find(entity) != world.healths.end()
            && world.aiStates.find(entity) != world.aiStates.end()) {
            
                Health& hp = world.healths[entity];
                float ratio = hp.current / (float) hp.max;
                ratio = Clamp(ratio, 0.0f, 1.0f);

                Vector2 pos = transform.position;
                float barWidth = 24;
                float barHeight = 4;

                DrawRectangle(
                    (int) pos.x - (int) barWidth/2,
                    (int) pos.y - 25,
                    (int) barWidth,
                    (int) barHeight,
                    DARKGRAY
                );
                DrawRectangle(
                    (int) pos.x - (int) barWidth/2,
                    (int) pos.y - 25,
                    barWidth * ratio,
                    (int) barHeight,
                    GREEN
                );
        }
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
            if (world.attackIntentes.find(entity) == world.attackIntentes.end()) continue;
            if (!world.attackIntentes[entity].wantsToAttack) continue;

            target = FindClosestHero(world, monster);

            world.attackIntentes[entity].wantsToAttack = false;
        }

        if (target == -1) continue;

        // have transform, health
        if (world.transforms.find(entity) == world.transforms.end()) continue;
        if (world.healths.find(target) == world.healths.end()) continue;

        Vector2 posA = world.transforms[entity].position;
        Vector2 posB = world.transforms[target].position;

        float distance = Vector2Distance(posA, posB);

        if (attack.timer <= 0.0f && distance <= attack.range) {
            world.healths[target].current -= attack.damage;
            attack.timer = attack.cooldown;
            world.hitFlashes[target] = { 0.1f };
            world.hitStopTimer = 0.05f;

            if (world.velocities.find(target) != world.velocities.end()) {
                Vector2 dir = Vector2Subtract(posB, posA);
                dir = Vector2Normalize(dir);
                world.velocities[target].value = Vector2Scale(dir, 200);
            }

            if (target == monster) {
                world.shakeTime = 0.2f;
                world.shakeMagnitude = 8.0f;
            }
        }
    }
}

void HealthSystem(World& world, Entity monster) {
    for (auto it = world.healths.begin(); it != world.healths.end(); ) {
        if (it->second.current <= 0) {
            Entity dead = it->first;

            // remove it
            world.transforms.erase(dead);
            world.velocities.erase(dead);
            world.colliders.erase(dead);
            world.meleeAttacks.erase(dead);
            if (dead == monster) {
                world.gameState = GameState::GameOver;
            } else {
                world.aiStates.erase(dead);
            }

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
    world.colliders[monster] = { 20.0f, 1, false };
    world.meleeAttacks[monster] = { 20, 40, 1.0f, 0.0f };
    world.attackIntentes[monster] = { false };
    world.playerControls[monster] = {};
    world.renderables[monster] = { RED };

    Camera2D camera = {0};
    camera.zoom = 1.0f;
    camera.offset = { screenWidth / 2.0f, screenHeight/2.0f };

    for (int i = 0; i < 5; i++) {
        Vector2 pos = {
            200 + i * 60.0f,
            200
        };
        SpawnHero(world, pos);
    }

    while (!WindowShouldClose()) {

        float dt = GetFrameTime();

        if (world.hitStopTimer > 0) {
            world.hitStopTimer -= dt;
            dt = 0; // freeze
        }

        if (world.shakeTime > 0.0f) {
            world.shakeTime -= dt;

            float offsetX = GetRandomValue(-100,100)/100.0f * world.shakeMagnitude;
            float offsetY = GetRandomValue(-100,100)/100.0f * world.shakeMagnitude;

            camera.offset = {
                screenWidth/2.0f + offsetX,
                screenHeight/2.0f + offsetY
            };
        } else {
            camera.offset = { screenWidth/2.0f, screenHeight/2.0f };
        }

        if (world.gameState == GameState::Playing) {
            InputSystem(world);
            AISystem(world, monster);
            MovementSystem(world, dt);
            CollisionSystem(world);
            AttackSystem(world, monster, dt);
            HealthSystem(world, monster);
            HitFlashSystem(world, dt);
            if (world.transforms.find(monster) != world.transforms.end()) {
                camera.target = world.transforms[monster].position;
            }
        }

        BeginDrawing();
        ClearBackground(DARKGREEN);
        DrawFPS(10, 10);
    
        BeginMode2D(camera);
        RenderSystem(world);
        EndMode2D();

        if (world.healths.find(monster) != world.healths.end()) {
            // hp bar
            Health& hp = world.healths[monster];
            float barWidth = 400;
            float barHeight = 20;
            float x = screenWidth/2 - barWidth/2;
            float y = 30;
            DrawRectangle((int) x, (int) y, barWidth, (int) barHeight, DARKGRAY);
            float hpRatio = hp.current / hp.max;
            DrawRectangle((int) x, (int) y, barWidth * hpRatio, (int) barHeight, RED);
        }

        // radial cooldown
        MeleeAttack& attack = world.meleeAttacks[monster];
        float cdRatio = 1.0f - (attack.timer / attack.cooldown);
        if (cdRatio < 0) cdRatio = 0;
        if (cdRatio > 1) cdRatio = 1;

        Vector2 center = { screenWidth - 80, screenHeight - 80 };
        float radius = 40;

        DrawCircleV(center, radius, DARKGRAY);
        DrawCircleSector(
            center,
            radius,
            -90,
            -90 + 360 * cdRatio,
            60,
            RED
        );
        if (world.gameState == GameState::GameOver) {
            DrawText("GAME OVER",
                (int) screenWidth/2 - 120,
                (int) screenHeight/2,
                40,
                RED
            );
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
