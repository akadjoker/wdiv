// features_demo.cpp - Demonstração das novas features do Physics2D
#include "raylib.h"
#include "Physics2D.h"
#include <iostream>
#include <sstream>

using namespace Physics2D;

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

Vector2 ToScreen(const Vec2 &v)
{
    return {v.x, v.y};
}

Vec2 FromScreen(const Vector2 &v)
{
    return Vec2(v.x, v.y);
}

void DrawBody(const Body *body)
{
    Color color = body->enabled ? BLUE : GRAY;
    if (body->inverseMass == 0.0f)
        color = DARKGRAY;

    Vector2 pos = ToScreen(body->position);

    if (body->shape.type == ShapeType::Circle)
    {
        DrawCircleV(pos, body->shape.radius, ColorAlpha(color, 0.6f));
        DrawCircleLines(pos.x, pos.y, body->shape.radius, color);
    }
    else
    {
        for (int i = 0; i < body->shape.vertexData.vertexCount; ++i)
        {
            Vec2 v1 = body->getVertex(i);
            Vec2 v2 = body->getVertex((i + 1) % body->shape.vertexData.vertexCount);
            DrawLineEx(ToScreen(v1), ToScreen(v2), 2.0f, color);
        }
    }
}

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Physics2D - Features Demo");
    SetTargetFPS(60);

    World world;
    world.setGravity(0.0f, 9.8f);

    // Criar chão
    Body *floor = world.createRectangle(Vec2(640, 680), 1200, 40, 100000.0f);
    floor->setLayer(0);

    // Criar diferentes layers
    // Layer 0 = Estruturas estáticas
    // Layer 1 = Objetos normais
    // Layer 2 = Projéteis (não colidem entre si)

    Body *leftWall = world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
    leftWall->setLayer(0);

    Body *rightWall = world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);
    rightWall->setLayer(0);

    // Criar alguns objetos
    Body *box1 = world.createRectangle(Vec2(300, 300), 50, 50, 1.0f);
    box1->setLayer(1);
    box1->linearDamping = 0.95f;

    Body *box2 = world.createRectangle(Vec2(400, 200), 60, 60, 1.5f);
    box2->setLayer(1);
    box2->restitution = 0.5f;

    Body *circle1 = world.createCircle(Vec2(500, 100), 30, 1.0f);
    circle1->setLayer(1);
    circle1->restitution = 0.8f;

    // Modo atual
    enum Mode
    {
        NORMAL,
        RAYCAST,
        EXPLOSION,
        QUERY_CIRCLE,
        QUERY_AABB
    };

    Mode currentMode = NORMAL;
    Vec2 mouseWorldPos;
    Vec2 raycastStart;
    bool raycastActive = false;

    std::string modeText = "NORMAL";

    while (!WindowShouldClose())
    {
        // Input
        Vector2 mousePos = GetMousePosition();
        mouseWorldPos = FromScreen(mousePos);

        // Mudar modo
        if (IsKeyPressed(KEY_ONE))
        {
            currentMode = NORMAL;
            modeText = "NORMAL";
        }
        if (IsKeyPressed(KEY_TWO))
        {
            currentMode = RAYCAST;
            modeText = "RAYCAST";
        }
        if (IsKeyPressed(KEY_THREE))
        {
            currentMode = EXPLOSION;
            modeText = "EXPLOSION";
        }
        if (IsKeyPressed(KEY_FOUR))
        {
            currentMode = QUERY_CIRCLE;
            modeText = "QUERY CIRCLE";
        }
        if (IsKeyPressed(KEY_FIVE))
        {
            currentMode = QUERY_AABB;
            modeText = "QUERY AABB";
        }

        if (IsKeyPressed(KEY_SPACE))
        {
            world.step();
        }
        else
        {
            world.step(); // Auto step
        }

        // Ações por modo
        switch (currentMode)
        {
        case NORMAL:
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Body *newBody = world.createCircle(mouseWorldPos, 25, 1.0f);
                newBody->setLayer(1);
                newBody->restitution = 0.6f;
            }
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
            {
                Body *newBody = world.createRectangle(mouseWorldPos, 40, 40, 1.0f);
                newBody->setLayer(1);
            }
            break;

        case RAYCAST:
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                raycastStart = mouseWorldPos;
                raycastActive = true;
            }
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && raycastActive)
            {
                // Desenhar preview do raio
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && raycastActive)
            {
                Vec2 direction = mouseWorldPos - raycastStart;
                float distance = direction.length();

                auto hit = world.raycast(raycastStart, direction, distance);

                if (hit.hit && hit.body)
                {
                    // Aplicar impulso no ponto de impacto
                    Vec2 impulse = direction.normalized() * 100.0f;
                    hit.body->applyImpulse(impulse, hit.point - hit.body->position);
                }

                raycastActive = false;
            }
            break;

        case EXPLOSION:
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            {
                world.applyExplosion(mouseWorldPos, 200000.0f, 250.0f);
            }
            break;

        case QUERY_CIRCLE:
        case QUERY_AABB:
            // Queries são desenhadas no render
            break;
        }

        // Render
        BeginDrawing();
        ClearBackground(BLACK);

        // Desenhar todos os corpos
        for (int i = 0; i < world.getBodyCount(); ++i)
        {
            Body *body = world.getBody(i);
            DrawBody(body);
        }

        // Desenhar baseado no modo
        switch (currentMode)
        {
        case RAYCAST:
            if (raycastActive)
            {
                DrawLineEx(ToScreen(raycastStart), mousePos, 2.0f, YELLOW);
                DrawCircleV(ToScreen(raycastStart), 5, YELLOW);
            }
            break;

        case EXPLOSION:
            DrawCircleLines(mousePos.x, mousePos.y, 200, ColorAlpha(RED, 0.5f));
            DrawCircleV(mousePos, 5, RED);
            break;

        case QUERY_CIRCLE:
        {
            float radius = 100.0f;
            auto bodies = world.queryCircle(mouseWorldPos, radius);
            DrawCircleLines(mousePos.x, mousePos.y, radius, ColorAlpha(GREEN, 0.8f));

            for (Body *body : bodies)
            {
                Vector2 pos = ToScreen(body->position);
                DrawCircleV(pos, 8, GREEN);
            }
            DrawText(TextFormat("Found: %d", bodies.size()), 10, 120, 20, GREEN);
            break;
        }

        case QUERY_AABB:
        {
            float size = 150.0f;
            AABB aabb(Vec2(mouseWorldPos.x - size / 2, mouseWorldPos.y - size / 2),
                      Vec2(mouseWorldPos.x + size / 2, mouseWorldPos.y + size / 2));

            auto bodies = world.queryAABB(aabb);
            DrawRectangleLines(mousePos.x - size / 2, mousePos.y - size / 2,
                               size, size, ColorAlpha(PURPLE, 0.8f));

            for (Body *body : bodies)
            {
                Vector2 pos = ToScreen(body->position);
                DrawCircleV(pos, 8, PURPLE);
            }
            DrawText(TextFormat("Found: %d", bodies.size()), 10, 120, 20, PURPLE);
            break;
        }

        default:
            break;
        }

        // UI
        DrawRectangle(10, 10, 300, 200, ColorAlpha(BLACK, 0.7f));
        DrawText("PHYSICS2D - FEATURES DEMO", 20, 20, 15, RAYWHITE);
        DrawText(TextFormat("Mode: %s", modeText.c_str()), 20, 45, 15, YELLOW);
        DrawText(TextFormat("Bodies: %d", world.getBodyCount()), 20, 65, 15, WHITE);
        DrawText(TextFormat("Collisions: %d", world.getCollisionChecks()), 20, 85, 15, WHITE);
        DrawText(TextFormat("FPS: %d", GetFPS()), 20, 105, 15, GREEN);

        // Energia cinética
        float energy = world.getTotalKineticEnergy();
        DrawText(TextFormat("Kinetic Energy: %.1f", energy), 20, 125, 12, ORANGE);

        // Centro de massa
        Vec2 com = world.getCenterOfMass();
        DrawCircleV(ToScreen(com), 8, ColorAlpha(YELLOW, 0.6f));
        DrawCircleLines(ToScreen(com).x, ToScreen(com).y, 10, YELLOW);

        DrawText("Controls:", 20, 155, 12, SKYBLUE);
        DrawText("1-Normal 2-Raycast 3-Explosion 4-QueryCircle 5-QueryAABB", 20, 170, 10, WHITE);
        DrawText("Left Click - Action | Right Click - Create Box", 20, 185, 10, WHITE);

        // Instruções específicas do modo
        DrawText(TextFormat("Mouse: (%.0f, %.0f)", mouseWorldPos.x, mouseWorldPos.y),
                 10, SCREEN_HEIGHT - 30, 12, GRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}