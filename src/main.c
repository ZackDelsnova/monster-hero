#include "raylib.h"
#include "raymath.h"

#include <stdio.h>
#include <stdlib.h>

char* healthText(const char *str, int health) {
    int r = snprintf(NULL, 0, "%s%d", str, health);
    char *result = malloc(r + 1);
    if (!result) return NULL;

    snprintf(result, r + 1, "%s%d", str, health);
    return result;
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Monster survival");
    SetTargetFPS(60);

    // player
    Vector2 playerPos = { (float)screenWidth/2, (float)screenHeight/2 };
    float playerRadius = 30;
    int playerHealth = 100;

    // hero
    Vector2 heroPos = { 100, 100 };
    float heroRadius = 15;
    int heroHealth = 30;
    int heroAlive = 1; // 1 - true, 0 false

    while (!WindowShouldClose()) {
        
        if (IsKeyDown(KEY_W)) playerPos.y -= 5;
        if (IsKeyDown(KEY_S)) playerPos.y += 5;
        if (IsKeyDown(KEY_A)) playerPos.x -= 5;
        if (IsKeyDown(KEY_D)) playerPos.x += 5;

        Vector2 dir = { playerPos.x - heroPos.x, playerPos.y - heroPos.y };
        float length = sqrtf(dir.x*dir.x + dir.y*dir.y);
        if (length > 0) {
            dir.x /= length;
            dir.y /= length;
            heroPos.x += dir.x * 2;
            heroPos.y += dir.y * 2;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            float dist = sqrtf(powf(playerPos.x - heroPos.x, 2) + powf(playerPos.y - heroPos.y, 2));
            if (dist < 50 && heroAlive) {
                heroHealth -= 10;
                if (heroHealth <= 0) {
                    heroAlive = 0;
                    heroHealth = 0;
                }
            }
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);

        DrawCircleV(playerPos, playerRadius, RED);

        if (heroAlive) DrawCircleV(heroPos, heroRadius, BLUE);

        DrawText(healthText("Player Health: " ,playerHealth), 10, 30, 20, WHITE);
        DrawText(healthText("Hero Health: " ,heroHealth), 10, 50, 20, WHITE);
        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
