
#include "raylib.h"
#include "Graph.h"
#include <iostream>
#include <sstream>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Physics2D - Features Demo");
    SetTargetFPS(30);

    GraphManager manager;

    // Simula texturas (100x80 para nave, 30x15 para arma)
    const int shipW = 100, shipH = 80;
    const int gunW = 30, gunH = 15;

    // Create ship graph
    Graph *ship = manager.create();
    ship->loadFile("assets/ship.png");
    // ship->sourceRect = {0, 0, (float)shipW, (float)shipH};
    ship->centerPoint = {(float)shipW / 2.0f, (float)shipH / 2.0f};
    ship->localPoints.clear();
    ship->localPoints.push(ship->centerPoint);                                    // Point 0: center
    ship->localPoints.push({ship->centerPoint.x, ship->centerPoint.y - 25});      // Point 1: top (weapon mount)
    ship->localPoints.push({ship->centerPoint.x, ship->centerPoint.y + 25}); // Point 2: left engine
   // ship->localPoints.push({ship->centerPoint.x + 15, ship->centerPoint.y + 20}); // Point 3: right engine
    ship->setPosition(600, 400);
    ship->sizeX = 100;
    ship->sizeY = 100;

    Graph *gun = manager.create();
    gun->loadFile("assets/gun.png");
    // gun->sourceRect = {0, 0, (float)gunW, (float)gunH};
    gun->centerPoint = {(float)gunW / 2.0f, (float)gunH / 2.0f};
    gun->localPoints.clear();
    gun->localPoints.push(gun->centerPoint);                                    // Point 0: gun center
    gun->localPoints.push({gun->centerPoint.x + gunW / 2, gun->centerPoint.y}); // Point 1: muzzle
    gun->setParent(ship);
    gun->sizeX = 100;
    gun->sizeY = 100;

    ship->setTransform(600, 400, 0, 100, 100);
 
    gun->setTransform(0, -25, 0, 100, 100);

    while (!WindowShouldClose())
    {

        BeginDrawing();

        ClearBackground(BLACK);
        // UI
        DrawRectangle(10, 10, 300, 200, ColorAlpha(BLACK, 0.7f));

        if (IsKeyDown(KEY_LEFT))
            ship->angle += 2000;
        if (IsKeyDown(KEY_RIGHT))
            ship->angle -= 2000;
        if (IsKeyDown(KEY_UP))
            GraphMath::advance(ship->x, ship->y, ship->angle, 2.0f);
     

        ship->render();
        gun->render();

        EndDrawing();
    }

    manager.cleanup();

    CloseWindow();
    return 0;
}