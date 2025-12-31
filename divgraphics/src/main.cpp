
#include "raylib.h"
#include "engine.hpp"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

void handleCollision(Entity *a, Entity *b, void *userdata)
{
    if (!a || !b)
        return;
    printf("Collision between %d and %d\n", a->graph, b->graph);

    if (a->graph == 1)
    {
        a->kill();
    }
    else if (b->graph == 1)
    {
        b->kill();
    }
}

static inline Vector2 Slide(Vector2 v, Vector2 n)
{
    float dot = v.x * n.x + v.y * n.y;
    return {
        v.x - n.x * dot,
        v.y - n.y * dot
    };
}

static inline Vector2 Reflect(Vector2 v, Vector2 n)
{
    float dot = v.x * n.x + v.y * n.y;
    return {
        v.x - n.x * dot * 2,
        v.y - n.y * dot * 2
    };
}

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Physics2D - Features Demo");
    SetTargetFPS(30);

    InitScene();

    int nave = LoadGraph("nave", "assets/ship.png");
    int gr = LoadGraph("orb", "assets/Orb.png");
    int bk1 = LoadGraph("bck1", "assets/0.png");
    int bk2 = LoadGraph("bck2", "assets/1.png");
    int bk3 = LoadGraph("bck3", "assets/2.png");
    int bk4 = LoadGraph("bck4", "assets/3.png");
    int bk5 = LoadGraph("bck5", "assets/4.png");

    MainCamera camera;
    camera.smoothness = 0.15; // Ajusta suavidade (0.0-1.0)
    camera.setBounds(0, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT);

    for (int i = 0; i < 20; i++)
    {
        int x = GetRandomValue(50, SCREEN_WIDTH * 2);
        int y = GetRandomValue(50, SCREEN_HEIGHT - 50);
        Entity *e = CreateEntity(gr, 4, x, y);
        e->setStatic();
        e->setCircleShape(20);
        e->setCollisionLayer(1);                  // Enemy layer
        e->setCollisionMask((1 << 0) | (1 << 2)); // Colide com Player e Bullets
    }

    for (int i = 0; i < 20; i++)
    {
        int x = GetRandomValue(0, SCREEN_WIDTH * 2);
        int y = GetRandomValue(50, SCREEN_HEIGHT - 50);
        Entity *e = CreateEntity(gr, 5, x, y);
        e->setStatic();
        e->setCircleShape(20);
        e->setCollisionLayer(1);                  // Enemy layer
        e->setCollisionMask((1 << 0) | (1 << 2)); // Colide com Player e Bullets
    }

    Entity *player = CreateEntity(nave, 4, 200, 200);
    player->setRectangleShape(-40, -40, 60, 80);
    //player->setCircleShape(20);

    // player->setCircleShape(20);
    player->setCollisionLayer(0);       // Player layer
    player->setCollisionMask((1 << 1)); // Só colide com Enemies

    SetLayerMode(0, LAYER_MODE_STRETCHX | LAYER_MODE_STRETCHY);
    SetLayerMode(1, LAYER_MODE_TILEX);
    SetLayerMode(2, LAYER_MODE_TILEX);
    SetLayerMode(3, LAYER_MODE_TILEX);
    SetLayerMode(4, LAYER_MODE_TILEX);

    SetLayerBackGraph(0, bk5);
    SetLayerBackGraph(1, bk4);
    SetLayerBackGraph(2, bk3);
    SetLayerBackGraph(3, bk2);
    SetLayerBackGraph(4, bk1);

    SetLayerSize(-1, 0, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT);

    SetLayerScrollFactor(1, 0.1, 0.0);
    SetLayerScrollFactor(2, 0.3, 0.0);
    SetLayerScrollFactor(3, 0.5, 0.0);
    SetLayerScrollFactor(4, 0.7, 0.0);

    InitCollision(0, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT, handleCollision);

    Vector2 velocity = {0, 0};

    while (!WindowShouldClose())
    {

        float dt = GetFrameTime();
     

        SetScroll(player->x - SCREEN_WIDTH / 2, player->y - SCREEN_HEIGHT / 2);

        BeginDrawing();

        ClearBackground(BLACK);
         

        RenderScene();

        Vector2 velocity = {0, 0};
        if (IsKeyDown(KEY_RIGHT)) velocity.x = 300;
        if (IsKeyDown(KEY_LEFT))  velocity.x = -300;
        if (IsKeyDown(KEY_DOWN))  velocity.y = 300;
        if (IsKeyDown(KEY_UP))    velocity.y = -300;
        
        // CollisionInfo collision;
        // if (player->move_and_collide(velocity.x * GetFrameTime(),
        //                              velocity.y * GetFrameTime(),
        //                              &collision))
        // {

        //      velocity = Slide(velocity, collision.normal);
        //     // Slide
        //    //double dot = velocity.x * collision.normal.x + velocity.y * collision.normal.y;
        //   //  velocity.x -= dot * collision.normal.x;
        //   //  velocity.y -= dot * collision.normal.y;
        //   DrawText(TextFormat("%f, %f , %f", collision.normal.x, collision.normal.y,collision.depth), 10, 10, 20, RED);
        // }


       player->move_and_slide(velocity, dt, {0, -1});
       player->snap_to_floor(6.0f, {0, -1}, velocity);  


    // Move and slide
    // Vector2 up = {0, -1};  // Up direction (para floating não importa)
    // player->move_and_slide(velocity, dt, up);

        // UI

        EndDrawing();
    }

    DestroyScene();

    CloseWindow();
    return 0;
}
