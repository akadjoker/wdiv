// joints_demo.cpp - Demonstração de Joints (Juntas)
#include "raylib.h"
#include "Physics2D.hpp"
#include <vector>

using namespace Physics2D;

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

Vector2 ToScreen(const Vec2& v) { return {v.x, v.y}; }
Vec2 FromScreen(const Vector2& v) { return Vec2(v.x, v.y); }

void DrawBody(const Body* body) {
    Color color = body->enabled ? BLUE : GRAY;
    if (body->inverseMass == 0.0f) color = DARKGRAY;
    
    Vector2 pos = ToScreen(body->position);
    
    if (body->shape.type == ShapeType::Circle) {
        DrawCircleV(pos, body->shape.radius, ColorAlpha(color, 0.6f));
        DrawCircleLines(pos.x, pos.y, body->shape.radius, color);
    } else {
        for (int i = 0; i < body->shape.vertexData.vertexCount; ++i) {
            Vec2 v1 = body->getVertex(i);
            Vec2 v2 = body->getVertex((i + 1) % body->shape.vertexData.vertexCount);
            DrawLineEx(ToScreen(v1), ToScreen(v2), 2.0f, color);
        }
    }
}

void DrawJoint(const Joint* joint) {
    if (!joint) return;
    
    Vec2 anchorA = joint->getWorldAnchorA();
    Vec2 anchorB = joint->getWorldAnchorB();
    
    Color color = YELLOW;
    
    switch (joint->getType()) {
        case JointType::Distance:
            color = GREEN;
            break;
        case JointType::Revolute:
            color = RED;
            break;
        case JointType::Prismatic:
            color = BLUE;
            break;
        case JointType::Weld:
            color = ORANGE;
            break;
    }
    
    // Linha conectando os dois corpos
    DrawLineEx(ToScreen(anchorA), ToScreen(anchorB), 2.0f, color);
    
    // Círculos nas âncoras
    DrawCircleV(ToScreen(anchorA), 5, color);
    DrawCircleV(ToScreen(anchorB), 5, color);
}

void CreateChain(World& world, Vec2 start, int segments, float length) {
    Body* prev = nullptr;
    
    for (int i = 0; i < segments; ++i) {
        Vec2 pos = start + Vec2(0, i * length);
        Body* box = world.createRectangle(pos, 20, length - 5, 1.0f);
        box->restitution = 0.1f;
        
        if (i == 0) {
            // Primeiro segmento fixo ao teto
            Body* anchor = world.createCircle(start, 5, 100000.0f);
            anchor->inverseMass = 0.0f;  // Força estático
            anchor->inverseInertia = 0.0f;
            world.createRevoluteJoint(anchor, box, start);
        } else {
            // Conectar ao segmento anterior
            Vec2 jointPos = start + Vec2(0, i * length - length/2);
            world.createRevoluteJoint(prev, box, jointPos);
        }
        
        prev = box;
    }
}

void CreateRagdoll(World& world, Vec2 center) {
    // Corpo
    Body* torso = world.createRectangle(center, 40, 80, 2.0f);
    
    // Cabeça
    Body* head = world.createCircle(center + Vec2(0, -60), 25, 1.0f);
    world.createRevoluteJoint(torso, head, center + Vec2(0, -40));
    
    // Braço esquerdo
    Body* leftArm = world.createRectangle(center + Vec2(-40, -20), 15, 50, 0.5f);
    auto leftShoulder = world.createRevoluteJoint(torso, leftArm, center + Vec2(-20, -20));
    leftShoulder->setLimits(-PI/2, PI/2);
    
    // Braço direito
    Body* rightArm = world.createRectangle(center + Vec2(40, -20), 15, 50, 0.5f);
    auto rightShoulder = world.createRevoluteJoint(torso, rightArm, center + Vec2(20, -20));
    rightShoulder->setLimits(-PI/2, PI/2);
    
    // Perna esquerda
    Body* leftLeg = world.createRectangle(center + Vec2(-15, 70), 15, 60, 0.8f);
    auto leftHip = world.createRevoluteJoint(torso, leftLeg, center + Vec2(-15, 40));
    leftHip->setLimits(-PI/4, PI/4);
    
    // Perna direita
    Body* rightLeg = world.createRectangle(center + Vec2(15, 70), 15, 60, 0.8f);
    auto rightHip = world.createRevoluteJoint(torso, rightLeg, center + Vec2(15, 40));
    rightHip->setLimits(-PI/4, PI/4);
}

void CreateBridge(World& world, Vec2 start, Vec2 end, int segments) {
    float totalDist = (end - start).length();
    float segmentDist = totalDist / segments;
    Vec2 direction = (end - start).normalized();
    
    Body* prev = world.createCircle(start, 5, 100000.0f);
    prev->inverseMass = 0.0f;  // Força estático
    prev->inverseInertia = 0.0f;
    
    for (int i = 0; i < segments; ++i) {
        Vec2 pos = start + direction * (i + 0.5f) * segmentDist;
        Body* plank = world.createRectangle(pos, segmentDist - 5, 15, 1.0f);
        
        Vec2 jointPos = start + direction * i * segmentDist;
        world.createRevoluteJoint(prev, plank, jointPos);
        
        prev = plank;
    }
    
    // Âncora final
    Body* endAnchor = world.createCircle(end, 5, 100000.0f);
    endAnchor->inverseMass = 0.0f;  // Força estático
    endAnchor->inverseInertia = 0.0f;
    world.createRevoluteJoint(prev, endAnchor, end);
}

void CreateCar(World& world, Vec2 pos) {
    // Chassis
    Body* chassis = world.createRectangle(pos, 120, 30, 2.0f);
    
    // Rodas
    Body* wheelL = world.createCircle(pos + Vec2(-40, 30), 20, 1.0f);
    wheelL->dynamicFriction = 0.9f;
    wheelL->staticFriction = 0.9f;
    wheelL->restitution = 0.2f;
    
    Body* wheelR = world.createCircle(pos + Vec2(40, 30), 20, 1.0f);
    wheelR->dynamicFriction = 0.9f;
    wheelR->staticFriction = 0.9f;
    wheelR->restitution = 0.2f;
    
    // Juntas das rodas (revolute para rotação livre)
    world.createRevoluteJoint(chassis, wheelL, pos + Vec2(-40, 15));
    world.createRevoluteJoint(chassis, wheelR, pos + Vec2(40, 15));
}

void CreateElevator(World& world, Vec2 start) {
    // Trilho (corpo estático)
    Body* rail = world.createRectangle(start, 10, 300, 100000.0f);
    rail->inverseMass = 0.0f;  // Força estático
    rail->inverseInertia = 0.0f;
    
    // Plataforma
    Body* platform = world.createRectangle(start, 100, 20, 2.0f);
    
    // Joint prismático (slider vertical)
    auto slider = world.createPrismaticJoint(rail, platform, start, Vec2(0, 1));
    slider->setLimits(-140, 140);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Physics2D - Joints Demo");
    SetTargetFPS(60);
    
    World world;
    world.setGravity(0.0f, 9.8f);
    
    // Chão
    Body* floor = world.createRectangle(Vec2(640, 680), 1200, 40, 100000.0f);
    floor->inverseMass = 0.0f;  // Força estático
    floor->inverseInertia = 0.0f;
    
    int currentDemo = 0;
    bool paused = false;
    
    // Demo inicial
    CreateChain(world, Vec2(200, 50), 8, 40);

    MouseJoint* mouseJoint = nullptr;
    Body* selectedBody = nullptr;
    
    while (!WindowShouldClose()) {
        // Input
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        
        if (IsKeyPressed(KEY_R)) {
            // Resetar
            while (world.getBodyCount() > 0) {
                world.destroyBody(world.getBody(0));
            }
            while (world.getJointCount() > 0) {
                world.destroyJoint(world.getJoint(0));
            }
            
            // Recriar chão
            floor = world.createRectangle(Vec2(640, 680), 1200, 40, 100000.0f);
            floor->inverseMass = 0.0f;  // Força estático
            floor->inverseInertia = 0.0f;
        }
        
        // Demos
        if (IsKeyPressed(KEY_ONE)) {
            CreateChain(world, Vec2(200 + currentDemo * 50, 50), 8, 40);
            currentDemo++;
        }
        
        if (IsKeyPressed(KEY_TWO)) {
            CreateRagdoll(world, Vec2(300 + currentDemo * 100, 200));
            currentDemo++;
        }
        
        if (IsKeyPressed(KEY_THREE)) {
            CreateBridge(world, Vec2(100, 300), Vec2(600, 300), 12);
        }
        
        if (IsKeyPressed(KEY_FOUR)) {
            CreateCar(world, Vec2(300 + currentDemo * 150, 100));
            currentDemo++;
        }
        
        if (IsKeyPressed(KEY_FIVE)) {
            CreateElevator(world, Vec2(900, 400));
        }
        
        // Adicionar força nos carros
        if (IsKeyDown(KEY_LEFT)) {
            for (int i = 0; i < world.getBodyCount(); ++i) {
                Body* body = world.getBody(i);
                if (body && body->shape.type == ShapeType::Circle && body->shape.radius == 20) {
                    body->addTorque(-50.0f);
                }
            }
        }
        if (IsKeyDown(KEY_RIGHT)) {
            for (int i = 0; i < world.getBodyCount(); ++i) {
                Body* body = world.getBody(i);
                if (body && body->shape.type == ShapeType::Circle && body->shape.radius == 20) {
                    body->addTorque(50.0f);
                }
            }
        }

         Vector2 mousePos = GetMousePosition();
        Vec2 mousePosWorld = FromScreen(mousePos);
        
        // Criar mouse joint ao clicar
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Body* body = world.getBodyAtPoint(mousePosWorld);
            if (body && body->inverseMass > 0.0f) {
                selectedBody = body;
                mouseJoint = world.createMouseJoint(body, mousePosWorld);
                mouseJoint->maxForce = 2000.0f;
                mouseJoint->stiffness = 0.8f;
                mouseJoint->damping = 0.9f;
            }
        }
        
        // Atualizar posição do mouse joint
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && mouseJoint) {
            mouseJoint->setTarget(mousePosWorld);
        }
        
        // Destruir mouse joint ao soltar
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && mouseJoint) {
            world.destroyJoint(mouseJoint);
            mouseJoint = nullptr;
            selectedBody = nullptr;
        }
        
        // Update
        if (!paused) {
            world.step();
        }
        
        // Render
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Desenhar corpos
        for (int i = 0; i < world.getBodyCount(); ++i) {
            DrawBody(world.getBody(i));
        }
        
       // Desenhar joints (exceto mouse joint)
        for (int i = 0; i < world.getJointCount(); ++i) {
            Joint* joint = world.getJoint(i);
            if (joint->getType() != JointType::Distance || joint != mouseJoint) {
                DrawJoint(joint);
            }
        }
        
        // Desenhar mouse joint se activo
        if (mouseJoint) {
            Vec2 anchor = mouseJoint->getWorldAnchorA();
            DrawLineEx(ToScreen(anchor), mousePos, 2.0f, MAGENTA);
            DrawCircleV(ToScreen(anchor), 8, MAGENTA);
            DrawCircleV(mousePos, 8, MAGENTA);
        }
        
        // UI
        DrawRectangle(10, 10, 400, 220, ColorAlpha(BLACK, 0.7f));
        DrawText("JOINTS DEMO", 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("Bodies: %d | Joints: %d", world.getBodyCount(), world.getJointCount()), 
                 20, 50, 15, YELLOW);
        DrawText(TextFormat("FPS: %d", GetFPS()), 20, 70, 15, GREEN);
        DrawText(paused ? "PAUSED" : "RUNNING", 20, 90, 15, paused ? RED : GREEN);
        
        DrawText("Create Demos:", 20, 120, 15, SKYBLUE);
        DrawText("1 - Chain (Corrente)", 20, 140, 12, WHITE);
        DrawText("2 - Ragdoll (Boneco)", 20, 155, 12, WHITE);
        DrawText("3 - Bridge (Ponte)", 20, 170, 12, WHITE);
        DrawText("4 - Car (Carro)", 20, 185, 12, WHITE);
        DrawText("5 - Elevator (Elevador)", 20, 200, 12, WHITE);
        
        DrawText("Controls:", 20, SCREEN_HEIGHT - 70, 12, ORANGE);
        DrawText("SPACE - Pause | R - Reset", 20, SCREEN_HEIGHT - 55, 10, WHITE);
        DrawText("LEFT/RIGHT - Drive Cars", 20, SCREEN_HEIGHT - 40, 10, WHITE);
        
        // Legenda dos joints
        DrawText("Joints:", SCREEN_WIDTH - 180, 20, 12, SKYBLUE);
        DrawCircleV({SCREEN_WIDTH - 160, 45}, 5, GREEN);
        DrawText("Distance", SCREEN_WIDTH - 145, 40, 10, WHITE);
        DrawCircleV({SCREEN_WIDTH - 160, 60}, 5, RED);
        DrawText("Revolute", SCREEN_WIDTH - 145, 55, 10, WHITE);
        DrawCircleV({SCREEN_WIDTH - 160, 75}, 5, BLUE);
        DrawText("Prismatic", SCREEN_WIDTH - 145, 70, 10, WHITE);
        DrawCircleV({SCREEN_WIDTH - 160, 90}, 5, ORANGE);
        DrawText("Weld", SCREEN_WIDTH - 145, 85, 10, WHITE);
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}