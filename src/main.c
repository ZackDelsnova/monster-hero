#include "raylib.h"
#include "raymath.h"

#include <stdio.h>
#include <stdlib.h>

#define TAG_PLAYER (1 << 0)
#define TAG_AI (1 << 1)
#define MAX_ENTITIES 256

typedef struct {
    Vector2 pos;
} Transform2D;

typedef struct {
    Vector2 vel;
} Velocity;

typedef struct {
    float radius;
    int health;
} Stats;

typedef struct {
    float speed;
} AI;

typedef int Entity;

static int entityCount = 0;
static int tags[MAX_ENTITIES];

static Transform2D transforms[MAX_ENTITIES];
static Velocity velocities[MAX_ENTITIES];
static Stats stats[MAX_ENTITIES];
static AI ais[MAX_ENTITIES];

char* DrawHealthText(const char *label, int value, int x, int y) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%d", label, value);
    DrawText(buf, x, y, 20, WHITE);
}

Entity CreateEntity(int tag) {
    Entity e = entityCount++;
    tags[e] = tag;
    velocities[e].vel = (Vector2){ 0, 0 };
    return e;
}

void PlayerInputSystem(float dt) {
    for (Entity e = 0; e < entityCount; e++) {
        if (tags[e] & TAG_PLAYER) {
            Vector2 dir = {0};

            if (IsKeyDown(KEY_W)) dir.y -= 1;
            if (IsKeyDown(KEY_S)) dir.y += 1;
            if (IsKeyDown(KEY_A)) dir.x -= 1;
            if (IsKeyDown(KEY_D)) dir.x += 1;

            if (Vector2Length(dir) > 0)
                dir = Vector2Normalize(dir);

            velocities[e].vel = Vector2Scale(dir, 300 * dt);
        }
    }
}

void AISystem(Entity player) {
    Vector2 playerPos = transforms[player].pos;

    for (Entity e = 0; e < entityCount; e++) {
        if (tags[e] & TAG_AI && stats[e].health > 0) {
            Vector2 dir = Vector2Subtract(playerPos, transforms[e].pos);

            if (Vector2Length(dir) > 0) dir = Vector2Normalize(dir);

            velocities[e].vel = Vector2Scale(dir, ais[e].speed * GetFrameTime());
        }
    }
}

void MovementSystem(void) {
    for (Entity e = 0; e < entityCount; e++) {
        transforms[e].pos = Vector2Add(transforms[e].pos, velocities[e].vel);
    }
}

void CombatSystem(Entity player) {
    if (!IsKeyPressed(KEY_SPACE)) return;

    for (Entity e = 0; e < entityCount; e++) {
        if (tags[e] & TAG_AI && stats[e].health > 0) {
            float dist = Vector2Distance(transforms[player].pos, transforms[e].pos);

            if (dist < 50) stats[e].health -= 10;
        }
    }
}

void RenderSystem(void) {
    for (Entity e = 0; e < entityCount; e++) {
        if (stats[e].health <= 0) continue;

        Color c = (tags[e] & TAG_PLAYER) ? RED : BLUE;
        DrawCircleV(transforms[e].pos, stats[e].radius, c);
    }
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Monster survival");
    SetTargetFPS(60);

    // player
    Entity player = CreateEntity(TAG_PLAYER);
    transforms[player].pos = (Vector2){ screenWidth/2, screenHeight/2 };
    stats[player] = (Stats){ 30, 100 };

    // hero
    Entity hero = CreateEntity(TAG_AI);
    transforms[hero].pos = (Vector2){ 100, 100 };
    stats[hero] = (Stats){ 15, 30 };
    ais[hero].speed = 120;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        PlayerInputSystem(dt);
        AISystem(player);
        CombatSystem(player);
        MovementSystem();

        BeginDrawing();
        ClearBackground(DARKGRAY);

        RenderSystem();

        DrawHealthText("Player Health: ", stats[player].health, 10, 30);
        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
