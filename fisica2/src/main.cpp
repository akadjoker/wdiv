// raylib_demo.cpp - Demonstração do Physics2D com Raylib EXPANDIDA
// Compile: g++ raylib_demo.cpp Physics2D.cpp -o demo -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c++17

#include "raylib.h"
#include "Physics2D.h"
#include <vector>
#include <cmath>

using namespace Physics2D;

// Constantes
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const float PIXELS_PER_METER = 1.0f;

// Converter coordenadas de física para renderização
Vector2 ToScreen(const Vec2 &physicsPos)
{
    return {physicsPos.x * PIXELS_PER_METER, physicsPos.y * PIXELS_PER_METER};
}

Vec2 FromScreen(const Vector2 &screenPos)
{
    return Vec2(screenPos.x / PIXELS_PER_METER, screenPos.y / PIXELS_PER_METER);
}

// Desenhar a Quadtree (recursivo)
void DrawQuadtreeNode(const Physics2D::QuadtreeNode *node, int depth = 0)
{
    if (!node)
        return;

    const Physics2D::AABB &bounds = node->getBounds();
    Vector2 min = ToScreen(bounds.min);
    Vector2 max = ToScreen(bounds.max);

    Color color;
    switch (depth % 4)
    {
    case 0: color = ColorAlpha(GREEN, 0.3f); break;
    case 1: color = ColorAlpha(BLUE, 0.3f); break;
    case 2: color = ColorAlpha(YELLOW, 0.3f); break;
    case 3: color = ColorAlpha(PURPLE, 0.3f); break;
    }

    DrawRectangleLines(min.x, min.y, max.x - min.x, max.y - min.y, color);

    if (node->isDivided())
    {
        for (int i = 0; i < 4; ++i)
        {
            DrawQuadtreeNode(node->getChild(i), depth + 1);
        }
    }
}

// Desenhar um corpo físico
void DrawPhysicsBody(const Body *body)
{
    if (!body)
        return;

    Color color = body->enabled ? BLUE : GRAY;
    if (body->inverseMass == 0.0f)
        color = DARKGRAY;
    if (body->isGrounded)
        color = GREEN;

    if (body->shape.type == ShapeType::Circle)
    {
        Vector2 pos = ToScreen(body->position);
        float radius = body->shape.radius * PIXELS_PER_METER;

        DrawCircleV(pos, radius, ColorAlpha(color, 0.7f));
        DrawCircleLines(pos.x, pos.y, radius, color);

        Vector2 direction = {
            std::cos(body->orient) * radius,
            std::sin(body->orient) * radius};
        DrawLineV(pos, {pos.x + direction.x, pos.y + direction.y}, RED);
    }
    else if (body->shape.type == ShapeType::Polygon)
    {
        const PolygonData &data = body->shape.vertexData;

        for (int i = 0; i < data.vertexCount; ++i)
        {
            Vec2 v1 = body->getVertex(i);
            Vec2 v2 = body->getVertex((i + 1) % data.vertexCount);

            Vector2 p1 = ToScreen(v1);
            Vector2 p2 = ToScreen(v2);

            DrawLineEx(p1, p2, 2.0f, color);
        }

        Vector2 center = ToScreen(body->position);
        for (int i = 0; i < data.vertexCount; ++i)
        {
            Vec2 v1 = body->getVertex(i);
            Vec2 v2 = body->getVertex((i + 1) % data.vertexCount);

            Vector2 p1 = ToScreen(v1);
            Vector2 p2 = ToScreen(v2);

            DrawTriangle(center, p1, p2, ColorAlpha(color, 0.5f));
        }
    }
}

void DrawVelocity(const Body *body)
{
    if (!body || body->velocity.lengthSq() < 0.1f)
        return;

    Vector2 pos = ToScreen(body->position);
    Vec2 vel = body->velocity * 5.0f;
    Vector2 end = {pos.x + vel.x, pos.y + vel.y};

    DrawLineEx(pos, end, 2.0f, YELLOW);
    DrawCircleV(end, 4.0f, YELLOW);
}

// Classe principal do demo
class PhysicsDemo
{
private:
    World world;
    Body *selectedBody;
    bool isPaused;
    bool showVelocity;
    bool showInfo;
    bool showCollisions;
    bool showQuadtree;
    int currentDemo;

public:
    PhysicsDemo() : selectedBody(nullptr), isPaused(false),
                    showVelocity(true), showInfo(true), showCollisions(false),
                    showQuadtree(false), currentDemo(0)
    {
        world.setGravity(0.0f, 9.8f);
    }

    void reset()
    {
        while (world.getBodyCount() > 0)
        {
            world.destroyBody(world.getBody(0));
        }
        selectedBody = nullptr;

        switch (currentDemo)
        {
        case 0: setupBasicDemo(); break;
        case 1: setupStackDemo(); break;
        case 2: setupDominoDemo(); break;
        case 3: setupPlaygroundDemo(); break;
        case 4: setupNewtonsCradleDemo(); break;
        case 5: setupPyramidDemo(); break;
        case 6: setupPinballDemo(); break;
        case 7: setupChainReactionDemo(); break;
        case 8: setupRotatingPlatformDemo(); break;
        case 9: setupWreckingBallDemo(); break;
        }
    }

    void setupBasicDemo()
    {
        // Chão
        Body *floor = world.createRectangle(Vec2(640, 650), 1200, 50, 100000.0f);
        floor->staticFriction = 0.6f;
        floor->dynamicFriction = 0.4f;

        // Paredes
        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);

        // Objetos variados
        Body *circle1 = world.createCircle(Vec2(300, 100), 40, 1.0f);
        circle1->restitution = 0.7f;
        circle1->addTorque(20.0f);

        Body *box1 = world.createRectangle(Vec2(500, 150), 60, 60, 1.0f);
        box1->restitution = 0.3f;
        box1->addTorque(-15.0f);

        Body *triangle = world.createPolygon(Vec2(700, 100), 50, 3, 1.0f);
        triangle->restitution = 0.5f;
        triangle->addForce(Vec2(30.0f, 0.0f));

        Body *hexagon = world.createPolygon(Vec2(900, 150), 45, 6, 1.0f);
        hexagon->restitution = 0.4f;
        hexagon->addTorque(25.0f);

        Body *penta = world.createPolygon(Vec2(400, 200), 35, 5, 1.0f);
        penta->restitution = 0.6f;
        penta->addTorque(30.0f);
    }

    void setupStackDemo()
    {
        Body *floor = world.createRectangle(Vec2(640, 650), 1200, 50, 100000.0f);
        floor->staticFriction = 0.6f;

        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);

        float boxSize = 50.0f;
        int layers = 10;

        for (int i = 0; i < layers; ++i)
        {
            float y = 600 - i * boxSize - boxSize / 2;
            Body *box = world.createRectangle(Vec2(640, y), boxSize, boxSize, 1.0f);
            box->restitution = 0.1f;
            box->staticFriction = 0.5f;
        }

        Body *ball = world.createCircle(Vec2(200, 300), 40, 2.0f);
        ball->restitution = 0.6f;
        ball->addForce(Vec2(500.0f, 0.0f));
    }

    void setupDominoDemo()
    {
        Body *floor = world.createRectangle(Vec2(640, 650), 1200, 50, 100000.0f);
        floor->staticFriction = 0.4f;

        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);

        int numDominos = 12;
        float spacing = 70.0f;

        for (int i = 0; i < numDominos; ++i)
        {
            float x = 250 + i * spacing;
            Body *domino = world.createRectangle(Vec2(x, 590), 15, 80, 1.0f);
            domino->restitution = 0.0f;
            domino->staticFriction = 0.4f;
        }

        Body *ball = world.createCircle(Vec2(150, 500), 30, 2.0f);
        ball->restitution = 0.0f;
        ball->velocity = Vec2(200.0f, 0.0f);
    }

    void setupPlaygroundDemo()
    {
        Body *floor = world.createRectangle(Vec2(640, 680), 1280, 80, 100000.0f);
        floor->staticFriction = 0.6f;

        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);

        Body *ramp = world.createRectangle(Vec2(300, 550), 400, 30, 100000.0f);
        ramp->setRotation(-0.3f);

        world.createRectangle(Vec2(800, 500), 200, 20, 100000.0f);
        world.createRectangle(Vec2(1000, 350), 200, 20, 100000.0f);

        for (int i = 0; i < 5; ++i)
        {
            float x = 150 + i * 50;
            Body *circle = world.createCircle(Vec2(x, 50 + i * 30), 25, 1.0f);
            circle->restitution = 0.6f;
        }

        for (int i = 0; i < 3; ++i)
        {
            Body *box = world.createRectangle(Vec2(600 + i * 80, 100), 40, 40, 1.0f);
            box->restitution = 0.3f;
        }
    }

    // ===== NOVA DEMO 5: Newton's Cradle =====
    void setupNewtonsCradleDemo()
    {
        Body *floor = world.createRectangle(Vec2(640, 650), 1200, 50, 100000.0f);
        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);

        // Suporte superior
        world.createRectangle(Vec2(640, 150), 500, 20, 100000.0f);

        // Criar 5 bolas suspensas
        int numBalls = 5;
        float spacing = 80.0f;
        float startX = 640 - (numBalls - 1) * spacing / 2.0f;
        float ballRadius = 35.0f;

        for (int i = 0; i < numBalls; ++i)
        {
            float x = startX + i * spacing;
            Body *ball = world.createCircle(Vec2(x, 400), ballRadius, 3.0f);
            ball->restitution = 0.95f; // Muito elástico
            ball->staticFriction = 0.1f;
            ball->dynamicFriction = 0.05f;
        }

        // Primeira bola começa deslocada
        Body *firstBall = world.getBody(3); // Índice 3 (após paredes e chão)
        firstBall->position = Vec2(startX - 100, 250);
        firstBall->velocity = Vec2(200.0f, 100.0f);
    }

    // ===== NOVA DEMO 6: Pyramid =====
    void setupPyramidDemo()
    {
        Body *floor = world.createRectangle(Vec2(640, 680), 1280, 80, 100000.0f);
        floor->staticFriction = 0.7f;

        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);

        // Construir pirâmide de caixas
        float boxSize = 45.0f;
        int layers = 8;
        float baseY = 630;

        for (int layer = 0; layer < layers; ++layer)
        {
            int boxesInLayer = layers - layer;
            float layerWidth = boxesInLayer * boxSize;
            float startX = 640 - layerWidth / 2.0f + boxSize / 2.0f;
            float y = baseY - layer * boxSize;

            for (int i = 0; i < boxesInLayer; ++i)
            {
                float x = startX + i * boxSize;
                Body *box = world.createRectangle(Vec2(x, y), boxSize * 0.95f, boxSize * 0.95f, 1.0f);
                box->restitution = 0.2f;
                box->staticFriction = 0.6f;
            }
        }

        // Bola gigante para destruir a pirâmide
        Body *wreckingBall = world.createCircle(Vec2(150, 300), 60, 5.0f);
        wreckingBall->restitution = 0.4f;
        wreckingBall->velocity = Vec2(300.0f, 0.0f);
    }

    // ===== NOVA DEMO 7: Pinball =====
    void setupPinballDemo()
    {
        // Paredes do pinball
        world.createRectangle(Vec2(640, 710), 1280, 20, 100000.0f); // Chão
        world.createRectangle(Vec2(20, 360), 40, 720, 100000.0f);  // Esquerda
        world.createRectangle(Vec2(1260, 360), 40, 720, 100000.0f); // Direita
        world.createRectangle(Vec2(640, 10), 1280, 20, 100000.0f);  // Teto

        // Flippers (simulados com retângulos angulados)
        Body *flipperLeft = world.createRectangle(Vec2(350, 620), 120, 20, 100000.0f);
        flipperLeft->setRotation(0.3f);

        Body *flipperRight = world.createRectangle(Vec2(930, 620), 120, 20, 100000.0f);
        flipperRight->setRotation(-0.3f);

        // Bumpers circulares (alta restituição)
        int bumperPositions[][2] = {{400, 300}, {640, 250}, {880, 300}, {520, 400}, {760, 400}};
        for (auto &pos : bumperPositions)
        {
            Body *bumper = world.createCircle(Vec2(pos[0], pos[1]), 40, 100000.0f);
            bumper->restitution = 1.5f; // Super bouncy!
        }

        // Obstáculos triangulares
        world.createPolygon(Vec2(300, 500), 40, 3, 100000.0f)->setRotation(0);
        world.createPolygon(Vec2(980, 500), 40, 3, 100000.0f)->setRotation(3.14f);

        // Bolas de pinball
        for (int i = 0; i < 3; ++i)
        {
            Body *ball = world.createCircle(Vec2(640 + i * 40, 100), 20, 1.0f);
            ball->restitution = 0.8f;
        }
    }

    // ===== NOVA DEMO 8: Chain Reaction =====
    void setupChainReactionDemo()
    {
        Body *floor = world.createRectangle(Vec2(640, 680), 1280, 80, 100000.0f);
        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);

        // Plataformas em escada
        for (int i = 0; i < 5; ++i)
        {
            float x = 250 + i * 200;
            float y = 600 - i * 80;
            world.createRectangle(Vec2(x, y), 150, 15, 100000.0f);

            // Caixas em cada plataforma
            Body *box = world.createRectangle(Vec2(x + 50, y - 40), 30, 30, 1.0f);
            box->restitution = 0.3f;
        }

        // Bola inicial no topo
        Body *initialBall = world.createCircle(Vec2(200, 100), 35, 2.0f);
        initialBall->restitution = 0.7f;
        initialBall->velocity = Vec2(100.0f, 0.0f);

        // Adicionar alguns polígonos extras
        for (int i = 0; i < 4; ++i)
        {
            Body *poly = world.createPolygon(Vec2(350 + i * 220, 200), 30, 5, 1.0f);
            poly->restitution = 0.6f;
        }
    }

    // ===== NOVA DEMO 9: Rotating Platform =====
    void setupRotatingPlatformDemo()
    {
        Body *floor = world.createRectangle(Vec2(640, 680), 1280, 80, 100000.0f);
        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);

        // Plataforma rotativa central (simular rotação com angularVelocity)
        Body *platform = world.createRectangle(Vec2(640, 400), 400, 30, 100000.0f);
        platform->angularVelocity = 0.5f; // Rotação constante

        // Plataformas fixas ao redor
        world.createRectangle(Vec2(300, 300), 150, 20, 100000.0f);
        world.createRectangle(Vec2(980, 300), 150, 20, 100000.0f);
        world.createRectangle(Vec2(300, 550), 150, 20, 100000.0f);
        world.createRectangle(Vec2(980, 550), 150, 20, 100000.0f);

        // Objetos que caem
        for (int i = 0; i < 8; ++i)
        {
            float x = 250 + i * 120;
            Body *obj = (i % 2 == 0) ? 
                world.createCircle(Vec2(x, 50), 25, 1.0f) :
                (Body*)world.createRectangle(Vec2(x, 50), 35, 35, 1.0f);
            obj->restitution = 0.5f;
        }
    }

    // ===== NOVA DEMO 10: Wrecking Ball =====
    void setupWreckingBallDemo()
    {
        Body *floor = world.createRectangle(Vec2(640, 680), 1280, 80, 100000.0f);
        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);

        // Construir parede para destruir
        float boxSize = 50.0f;
        int rows = 6;
        int cols = 8;
        float startX = 700;
        float startY = 630;

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                float x = startX + col * boxSize;
                float y = startY - row * boxSize;
                Body *brick = world.createRectangle(Vec2(x, y), boxSize * 0.95f, boxSize * 0.95f, 1.0f);
                brick->restitution = 0.1f;
                brick->staticFriction = 0.5f;
            }
        }

        // Wrecking ball (bola de demolição)
        Body *wreckingBall = world.createCircle(Vec2(300, 200), 70, 10.0f);
        wreckingBall->restitution = 0.3f;
        wreckingBall->velocity = Vec2(400.0f, 0.0f);

        // "Corrente" simulada com círculos menores
        for (int i = 1; i < 4; ++i)
        {
            Body *chain = world.createCircle(Vec2(300 - i * 50, 150), 15, 0.5f);
            chain->restitution = 0.2f;
        }
    }

    void update()
    {
        if (!isPaused)
        {
            world.step();
        }

        // Input
        if (IsKeyPressed(KEY_SPACE)) isPaused = !isPaused;
        if (IsKeyPressed(KEY_V)) showVelocity = !showVelocity;
        if (IsKeyPressed(KEY_I)) showInfo = !showInfo;
        if (IsKeyPressed(KEY_C)) showCollisions = !showCollisions;
        if (IsKeyPressed(KEY_Q)) showQuadtree = !showQuadtree;
        if (IsKeyPressed(KEY_T)) world.setUseQuadtree(!world.isUsingQuadtree());
        if (IsKeyPressed(KEY_R)) reset();

        // Mudar demos (1-9, 0 para demo 10)
        if (IsKeyPressed(KEY_ONE)) { currentDemo = 0; reset(); }
        if (IsKeyPressed(KEY_TWO)) { currentDemo = 1; reset(); }
        if (IsKeyPressed(KEY_THREE)) { currentDemo = 2; reset(); }
        if (IsKeyPressed(KEY_FOUR)) { currentDemo = 3; reset(); }
        if (IsKeyPressed(KEY_FIVE)) { currentDemo = 4; reset(); }
        if (IsKeyPressed(KEY_SIX)) { currentDemo = 5; reset(); }
        if (IsKeyPressed(KEY_SEVEN)) { currentDemo = 6; reset(); }
        if (IsKeyPressed(KEY_EIGHT)) { currentDemo = 7; reset(); }
        if (IsKeyPressed(KEY_NINE)) { currentDemo = 8; reset(); }
        if (IsKeyPressed(KEY_ZERO)) { currentDemo = 9; reset(); }

        // Criar objetos com mouse
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            Vector2 mousePos = GetMousePosition();
            Vec2 pos = FromScreen(mousePos);

            if (IsKeyDown(KEY_LEFT_SHIFT))
            {
                Body *box = world.createRectangle(pos, 50, 50, 1.0f);
                box->restitution = 0.4f;
            }
            else
            {
                Body *circle = world.createCircle(pos, 25, 1.0f);
                circle->restitution = 0.6f;
            }
        }

        // Aplicar força com botão direito
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        {
            Vector2 mousePos = GetMousePosition();
            Vec2 clickPos = FromScreen(mousePos);

            Body *nearest = nullptr;
            float minDist = 100.0f;

            for (int i = 0; i < world.getBodyCount(); ++i)
            {
                Body *body = world.getBody(i);
                if (body && body->inverseMass > 0)
                {
                    Vec2 diff = body->position - clickPos;
                    float dist = diff.length();
                    if (dist < minDist)
                    {
                        minDist = dist;
                        nearest = body;
                    }
                }
            }

            if (nearest)
            {
                Vec2 direction = clickPos - nearest->position;
                direction.normalize();
                nearest->addForce(direction * -500.0f);
            }
        }
    }

    void draw()
    {
        ClearBackground(BLACK);

        if (showQuadtree && world.isUsingQuadtree())
        {
            DrawQuadtreeNode(world.getQuadtree()->getRoot());
        }

        for (int i = 0; i < world.getBodyCount(); ++i)
        {
            Body *body = world.getBody(i);
            DrawPhysicsBody(body);

            if (showVelocity)
            {
                DrawVelocity(body);
            }
        }

        // UI
        if (showInfo)
        {
            DrawRectangle(10, 10, 420, 240, ColorAlpha(BLACK, 0.7f));
            DrawText("PHYSICS2D + RAYLIB - 10 DEMOS", 20, 20, 20, RAYWHITE);
            DrawText(TextFormat("Bodies: %d", world.getBodyCount()), 20, 50, 15, YELLOW);
            DrawText(TextFormat("Collisions: %d", world.getCollisionChecks()), 20, 70, 15, ORANGE);
            DrawText(TextFormat("FPS: %d", GetFPS()), 20, 90, 15, GREEN);
            DrawText(isPaused ? "PAUSED" : "RUNNING", 20, 110, 15, isPaused ? RED : GREEN);
            DrawText(TextFormat("Quadtree: %s", world.isUsingQuadtree() ? "ON" : "OFF"),
                     20, 130, 15, world.isUsingQuadtree() ? GREEN : RED);

            DrawText("Controls:", 20, 160, 15, SKYBLUE);
            DrawText("SPACE-Pause | R-Reset | V-Velocity", 20, 180, 12, WHITE);
            DrawText("Q-Quadtree | T-Toggle Quadtree", 20, 195, 12, WHITE);
            DrawText("1-9,0 - Change Demo", 20, 210, 12, WHITE);
            DrawText("Left Click-Circle | Shift+Click-Box", 20, 225, 12, WHITE);
        }

        const char *demoNames[] = {"Basic", "Stack", "Domino", "Playground", 
                                   "Newton's Cradle", "Pyramid", "Pinball", 
                                   "Chain Reaction", "Rotating Platform", "Wrecking Ball"};
        DrawText(TextFormat("Demo %d: %s", currentDemo + 1, demoNames[currentDemo]), 
                 10, SCREEN_HEIGHT - 50, 16, YELLOW);
        DrawText("Right Click - Apply Force to nearest object", 10, SCREEN_HEIGHT - 30, 12, LIGHTGRAY);
    }

    World &getWorld() { return world; }
};

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Physics2D + Raylib - 10 Demos");
    SetTargetFPS(60);

    PhysicsDemo demo;
    demo.reset();

    while (!WindowShouldClose())
    {
        demo.update();

        BeginDrawing();
        demo.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
