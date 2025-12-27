// raylib_demo.cpp - Demonstração do Physics2D com Raylib
// Compile: g++ raylib_demo.cpp -o demo -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c++11

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
Vector2 ToScreen(const Vec2& physicsPos) {
    return {physicsPos.x * PIXELS_PER_METER, physicsPos.y * PIXELS_PER_METER};
}

Vec2 FromScreen(const Vector2& screenPos) {
    return Vec2(screenPos.x / PIXELS_PER_METER, screenPos.y / PIXELS_PER_METER);
}

// Desenhar um corpo físico
void DrawPhysicsBody(const Body* body) {
    if (!body) return;
    
    Color color = body->enabled ? BLUE : GRAY;
    if (body->inverseMass == 0.0f) color = DARKGRAY; // Estático
    if (body->isGrounded) color = GREEN;
    
    if (body->shape.type == ShapeType::Circle) {
        Vector2 pos = ToScreen(body->position);
        float radius = body->shape.radius * PIXELS_PER_METER;
        
        DrawCircleV(pos, radius, ColorAlpha(color, 0.7f));
        DrawCircleLines(pos.x, pos.y, radius, color);
        
        // Desenhar linha para mostrar rotação
        Vector2 direction = {
            std::cos(body->orient) * radius,
            std::sin(body->orient) * radius
        };
        DrawLineV(pos, {pos.x + direction.x, pos.y + direction.y}, RED);
        
    } else if (body->shape.type == ShapeType::Polygon) {
        const PolygonData& data = body->shape.vertexData;
        
        for (int i = 0; i < data.vertexCount; ++i) {
            Vec2 v1 = body->getVertex(i);
            Vec2 v2 = body->getVertex((i + 1) % data.vertexCount);
            
            Vector2 p1 = ToScreen(v1);
            Vector2 p2 = ToScreen(v2);
            
            DrawLineEx(p1, p2, 2.0f, color);
        }
        
        // Preencher polígono
        Vector2 center = ToScreen(body->position);
        for (int i = 0; i < data.vertexCount; ++i) {
            Vec2 v1 = body->getVertex(i);
            Vec2 v2 = body->getVertex((i + 1) % data.vertexCount);
            
            Vector2 p1 = ToScreen(v1);
            Vector2 p2 = ToScreen(v2);
            
            DrawTriangle(center, p1, p2, ColorAlpha(color, 0.5f));
        }
    }
}

// Desenhar vetores de velocidade
void DrawVelocity(const Body* body) {
    if (!body || body->velocity.lengthSq() < 0.1f) return;
    
    Vector2 pos = ToScreen(body->position);
    Vec2 vel = body->velocity * 5.0f; // Escalar para visualização
    Vector2 end = {pos.x + vel.x, pos.y + vel.y};
    
    DrawLineEx(pos, end, 2.0f, YELLOW);
    DrawCircleV(end, 4.0f, YELLOW);
}

// Classe principal do demo
class PhysicsDemo {
private:
    World world;
    Body* selectedBody;
    bool isPaused;
    bool showVelocity;
    bool showInfo;
    bool showCollisions;
    int currentDemo;
    
public:
    PhysicsDemo() : selectedBody(nullptr), isPaused(false), 
                    showVelocity(true), showInfo(true), showCollisions(false), currentDemo(0) {
        world.setGravity(0.0f, 9.8f);
    }
    
    void reset() {
        // Destruir todos os corpos
        while (world.getBodyCount() > 0) {
            world.destroyBody(world.getBody(0));
        }
        selectedBody = nullptr;
        
        // Criar nova demo baseada no modo atual
        switch (currentDemo) {
            case 0: setupBasicDemo(); break;
            case 1: setupStackDemo(); break;
            case 2: setupDominoDemo(); break;
            case 3: setupPlaygroundDemo(); break;
        }
    }
    
    void setupBasicDemo() {
        // Chão
        Body* floor = world.createRectangle(Vec2(640, 650), 1200, 50, 100000.0f);
        floor->staticFriction = 0.6f;
        floor->dynamicFriction = 0.4f;
        
        // Paredes
        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);
        
        // Objetos variados com torque para mostrar rotação
        Body* circle1 = world.createCircle(Vec2(300, 100), 40, 1.0f);
        circle1->restitution = 0.7f;
        circle1->addTorque(20.0f); // Adicionar torque inicial
        
        Body* box1 = world.createRectangle(Vec2(500, 150), 60, 60, 1.0f);
        box1->restitution = 0.3f;
        box1->addTorque(-15.0f); // Rodar no sentido contrário
        
        Body* triangle = world.createPolygon(Vec2(700, 100), 50, 3, 1.0f);
        triangle->restitution = 0.5f;
        triangle->addForce(Vec2(30.0f, 0.0f)); // Força lateral para criar rotação na colisão
        
        Body* hexagon = world.createPolygon(Vec2(900, 150), 45, 6, 1.0f);
        hexagon->restitution = 0.4f;
        hexagon->addTorque(25.0f);
        
        // Adicionar alguns pentágonos
        Body* penta = world.createPolygon(Vec2(400, 200), 35, 5, 1.0f);
        penta->restitution = 0.6f;
        penta->addTorque(30.0f);
    }
    
    void setupStackDemo() {
        // Chão
        Body* floor = world.createRectangle(Vec2(640, 650), 1200, 50, 100000.0f);
        floor->staticFriction = 0.6f;
        floor->dynamicFriction = 0.4f;
        
        // Paredes
        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);
        
        // Torre de caixas
        float boxSize = 50.0f;
        int layers = 10;
        
        for (int i = 0; i < layers; ++i) {
            float y = 600 - i * boxSize - boxSize/2;
            Body* box = world.createRectangle(Vec2(640, y), boxSize, boxSize, 1.0f);
            box->restitution = 0.1f;
            box->staticFriction = 0.5f;
        }
        
        // Bola para derrubar a torre
        Body* ball = world.createCircle(Vec2(200, 300), 40, 2.0f);
        ball->restitution = 0.6f;
        ball->addForce(Vec2(500.0f, 0.0f));
    }
    
    void setupDominoDemo() {
        // Chão
        Body* floor = world.createRectangle(Vec2(640, 650), 1200, 50, 100000.0f);
        floor->staticFriction = 0.4f;
        floor->dynamicFriction = 0.2f;
        
        // Paredes
        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);
        
        // Dominós
        int numDominos = 12;
        float spacing = 70.0f;
        
        for (int i = 0; i < numDominos; ++i) {
            float x = 250 + i * spacing;
            Body* domino = world.createRectangle(Vec2(x, 590), 15, 80, 1.0f);
            domino->restitution = 0.0f;
            domino->staticFriction = 0.4f;
            domino->dynamicFriction = 0.2f;
        }
        
        // Bola para derrubar
        Body* ball = world.createCircle(Vec2(150, 500), 30, 2.0f);
        ball->restitution = 0.0f;
        ball->velocity = Vec2(200.0f, 0.0f);
    }
    
    void setupPlaygroundDemo() {
        // Chão
        Body* floor = world.createRectangle(Vec2(640, 680), 1280, 80, 100000.0f);
        floor->staticFriction = 0.6f;
        floor->dynamicFriction = 0.4f;
        
        // Paredes
        world.createRectangle(Vec2(50, 360), 50, 720, 100000.0f);
        world.createRectangle(Vec2(1230, 360), 50, 720, 100000.0f);
        
        // Rampa
        Body* ramp = world.createRectangle(Vec2(300, 550), 400, 30, 100000.0f);
        ramp->setRotation(-0.3f);
        
        // Plataformas
        world.createRectangle(Vec2(800, 500), 200, 20, 100000.0f);
        world.createRectangle(Vec2(1000, 350), 200, 20, 100000.0f);
        
        // Objetos diversos
        for (int i = 0; i < 5; ++i) {
            float x = 150 + i * 50;
            Body* circle = world.createCircle(Vec2(x, 50 + i * 30), 25, 1.0f);
            circle->restitution = 0.6f;
        }
        
        for (int i = 0; i < 3; ++i) {
            Body* box = world.createRectangle(Vec2(600 + i * 80, 100), 40, 40, 1.0f);
            box->restitution = 0.3f;
        }
    }
    
    void update() {
        if (!isPaused) {
            world.step();
        }
        
        // Input do teclado
        if (IsKeyPressed(KEY_SPACE)) isPaused = !isPaused;
        if (IsKeyPressed(KEY_V)) showVelocity = !showVelocity;
        if (IsKeyPressed(KEY_I)) showInfo = !showInfo;
        if (IsKeyPressed(KEY_C)) showCollisions = !showCollisions;
        if (IsKeyPressed(KEY_R)) reset();
        
        // Mudar de demo
        if (IsKeyPressed(KEY_ONE)) { currentDemo = 0; reset(); }
        if (IsKeyPressed(KEY_TWO)) { currentDemo = 1; reset(); }
        if (IsKeyPressed(KEY_THREE)) { currentDemo = 2; reset(); }
        if (IsKeyPressed(KEY_FOUR)) { currentDemo = 3; reset(); }
        
        // Criar objetos com o mouse
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            Vec2 pos = FromScreen(mousePos);
            
            if (IsKeyDown(KEY_LEFT_SHIFT)) {
                Body* box = world.createRectangle(pos, 50, 50, 1.0f);
                box->restitution = 0.4f;
            } else {
                Body* circle = world.createCircle(pos, 25, 1.0f);
                circle->restitution = 0.6f;
            }
        }
        
        // Adicionar força com botão direito
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            Vec2 clickPos = FromScreen(mousePos);
            
            // Encontrar corpo mais próximo
            Body* nearest = nullptr;
            float minDist = 100.0f;
            
            for (int i = 0; i < world.getBodyCount(); ++i) {
                Body* body = world.getBody(i);
                if (body && body->inverseMass > 0) {
                    Vec2 diff = body->position - clickPos;
                    float dist = diff.length();
                    if (dist < minDist) {
                        minDist = dist;
                        nearest = body;
                    }
                }
            }
            
            if (nearest) {
                Vec2 direction = clickPos - nearest->position;
                direction.normalize();
                nearest->addForce(direction * -500.0f);
            }
        }
    }
    
    void draw() {
        ClearBackground(BLACK);
        
        // Desenhar todos os corpos
        for (int i = 0; i < world.getBodyCount(); ++i) {
            Body* body = world.getBody(i);
            DrawPhysicsBody(body);
            
            if (showVelocity) {
                DrawVelocity(body);
            }
        }
        
        // UI
        if (showInfo) {
            DrawRectangle(10, 10, 350, 180, ColorAlpha(BLACK, 0.7f));
            DrawText("PHYSICS2D + RAYLIB DEMO", 20, 20, 20, RAYWHITE);
            DrawText(TextFormat("Bodies: %d", world.getBodyCount()), 20, 50, 15, YELLOW);
            DrawText(TextFormat("FPS: %d", GetFPS()), 20, 70, 15, GREEN);
            DrawText(isPaused ? "PAUSED" : "RUNNING", 20, 90, 15, isPaused ? RED : GREEN);
            
            DrawText("Controls:", 20, 120, 15, SKYBLUE);
            DrawText("SPACE - Pause/Resume", 20, 140, 12, WHITE);
            DrawText("R - Reset | V - Toggle Velocity | C - Collisions", 20, 155, 12, WHITE);
            DrawText("1-4 - Change Demo", 20, 170, 12, WHITE);
        }
        
        // Instruções inferiores
        const char* demoNames[] = {"Basic", "Stack", "Domino", "Playground"};
        DrawText(TextFormat("Demo: %s", demoNames[currentDemo]), 10, SCREEN_HEIGHT - 50, 15, YELLOW);
        DrawText("Left Click - Create Circle | Shift+Click - Create Box", 10, SCREEN_HEIGHT - 30, 12, WHITE);
        DrawText("Right Click - Apply Force", 450, SCREEN_HEIGHT - 30, 12, WHITE);
    }
    
    World& getWorld() { return world; }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Physics2D + Raylib Demo");
    SetTargetFPS(60);
    
    PhysicsDemo demo;
    demo.reset(); // Inicializar primeira demo
    
    while (!WindowShouldClose()) {
        demo.update();
        
        BeginDrawing();
        demo.draw();
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}