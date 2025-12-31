#pragma once
#include "config.hpp"
#include "render.hpp"
#include <vector>
#include <raylib.h>
#include <cstring>
#include <cmath>
#include <algorithm>

enum LayerMode : uint8
{
    LAYER_MODE_TILEX = 1,    // 0001
    LAYER_MODE_TILEY = 2,    // 0010
    LAYER_MODE_STRETCHX = 4, // 0100
    LAYER_MODE_STRETCHY = 8, // 1000
    LAYER_MODE_FLIPX = 16,   // 10000
    LAYER_MODE_FLIPY = 32    // 100000
};

// Entity flags
#define MAX_LAYERS 6
#define MAXNAME 64
#define B_HMIRROR (1 << 0)
#define B_VMIRROR (1 << 1)
#define B_VISIBLE (1 << 2)
#define B_DEAD (1 << 3)
#define B_FROZEN (1 << 4)
#define B_STATIC (1 << 5)
#define B_COLLISION (1 << 6)

#define MAX_POINTS 8

enum ShapeType
{
    CIRCLE,
    RECTANGLE,
    POLYGON
};

class Scene;
struct Entity;

typedef void (*CollisionCallback)(Entity *a, Entity *b, void *userdata);

struct Shape
{
    uint8 type;
    virtual ~Shape() {}
    bool collide(Shape *other,
                 double x1, double y1, double a1, bool fx1, bool fy1,
                 double x2, double y2, double a2, bool fx2, bool fy2);

    virtual void draw(const Entity *entity, Color color) = 0;
};
struct CircleShape : public Shape
{
    float radius;
    Vector2 offset;

    CircleShape(float r = 1.0f) : radius(r), offset({0, 0}) { type = CIRCLE; }
    void draw(const Entity *entity, Color color) override;
};

struct PolygonShape : public Shape
{
    int num_points;
    Vector2 points[MAX_POINTS];
    Vector2 normals[MAX_POINTS];

    PolygonShape(int n = 0) : num_points(n) { type = POLYGON; }
    void calcNormals();
    void draw(const Entity *entity, Color color) override;
};

struct RectangleShape : public PolygonShape
{
    RectangleShape(int x, int y, int w, int h);
};

struct Graph
{
    std::vector<Vector2> points;
    int texture;        // id do opengl da textura , com raylib podmeos passar direito
    int width, height;  // obtemos da textura2d
    Rectangle clip;     // clip da textura (x, y, width, height)
    int id;             // id do graph
    char name[MAXNAME]; // nome do graph
};

struct CollisionInfo
{
    Entity *collider;
    Vector2 normal;
    bool hit;
    double depth;
};

struct Entity
{
    uint32 id;
    int graph; // referencia ao Graph via ID
    Shape *shape;
    uint8 layer;
    double x, y;
    double last_x, last_y;
    bool flip_x, flip_y;

    uint32 collision_layer; // Em que layer esta entity está
    uint32 collision_mask;  // Com que layers esta entity colide

    double center_x;
    double center_y;

    int64 flags;

    double size_x; // GRAPHSIZEX
    double size_y; // GRAPHSIZEY

    Rectangle bounds;
    bool bounds_dirty;

    void updateBounds(); // Recalcula AABB
    Rectangle getBounds();
    int64 resolution;
    int64 angle;
    Color color;
    bool on_floor = false;
    bool on_wall = false;
    bool on_ceiling = false;
    Vector2 floor_normal = {0, 0};

    Vector2 getPoint(int pointIdx) const;
    Vector2 getRealPoint(int pointIdx) const;

    void setCollisionLayer(uint32 layer) { collision_layer = (1 << layer); }
    void setCollisionMask(uint32 mask) { collision_mask = mask; }
    void addCollisionMask(uint32 layer) { collision_mask |= (1 << layer); }
    void removeCollisionMask(uint32 layer) { collision_mask &= ~(1 << layer); }
    bool canCollideWith(const Entity *other) const { return (collision_mask & other->collision_layer) != 0; }

    bool collide(Entity *other);
    bool intersects(const Entity *other) const;

    Vector2 getRealPointCustom(double px, double py) const;

    int64 getAngle(const Entity *other) const;

    double getDistance(const Entity *other) const;

    void advance(double distance);
    void xadvance(int64 angle_param, double distance);

    // Movement helpers

    void turnTowards(double targetX, double targetY, int64 maxTurn);
    bool moveTowards(double targetX, double targetY, double speed, double stopDistance = 0.0);

    Entity();
    ~Entity();

    void setFlag(uint32 flag) { flags |= flag; }
    void clearFlag(uint32 flag) { flags &= ~flag; }
    void toggleFlag(uint32 flag) { flags ^= flag; }
    bool hasFlag(uint32 flag) const { return (flags & flag) != 0; }

    // Shortcuts comuns
    void show() { setFlag(B_VISIBLE); }
    void hide() { clearFlag(B_VISIBLE); }
    void kill() { setFlag(B_DEAD); }
    void freeze() { setFlag(B_FROZEN); }
    void unfreeze() { clearFlag(B_FROZEN); }
    void enableCollision() { setFlag(B_COLLISION); }
    void disableCollision() { clearFlag(B_COLLISION); }
    void setStatic() { setFlag(B_STATIC); }

    bool isVisible() const { return hasFlag(B_VISIBLE); }
    bool isDead() const { return hasFlag(B_DEAD); }
    bool isFrozen() const { return hasFlag(B_FROZEN); }
    bool isStatic() const { return hasFlag(B_STATIC); }

    void render();

    bool place_free(double x, double y);
    Entity *place_meeting(double x, double y);
    bool move_and_slide(Vector2 &velocity, float delta, Vector2 up_direction = {0, -1});
    bool move_and_collide(double vel_x, double vel_y, CollisionInfo *result);
    void moveBy(double x, double y);
    bool snap_to_floor(float snap_len, Vector2 up_direction, Vector2 &velocity);

    void setRectangleShape(int x, int y, int w, int h);
    void setCircleShape(float radius);
    void setShape(Vector2 *points, int n);
};

struct QuadtreeNode
{
    Rectangle bounds;
    QuadtreeNode *children[4];
    std::vector<Entity *> items;
    int depth;

    static const int MAX_ITEMS = 8;
    static const int MAX_DEPTH = 8;

    QuadtreeNode(Rectangle b, int d) : bounds(b), depth(d)
    {
        children[0] = children[1] = children[2] = children[3] = nullptr;
    }

    ~QuadtreeNode()
    {
        for (int i = 0; i < 4; i++)
            if (children[i])
                delete children[i];
    }

    bool overlapsRect(Rectangle other);
    void insert(Entity *entity);
    void query(Rectangle area, std::vector<Entity *> &result);
    void split();
    void clear();
    void draw();
};

class Quadtree
{
    QuadtreeNode *root;

public:
    Quadtree(Rectangle world_bounds);

    ~Quadtree();

    void draw();

    void clear();
    void insert(Entity *entity);
    void query(Rectangle area, std::vector<Entity *> &result);
    void rebuild(Scene *scene);
};
 
struct Tile
{
    uint16 id; // 0 = empty
    bool solid;
    uint8 shape_type; // 0=none, 1=rect, 2=circle
    Rectangle rect;
    float radius;
};

 
class Tilemap
{
public:
    enum GridType
    {
        ORTHO = 0,
        HEXAGON = 1
    };

    // Constructor/Destructor [web:25]
    Tilemap();
    ~Tilemap();

    // Setup [web:25]
    void init(int w, int h, int size,int graph);
    void clear();
 

    // Tile access (inline getters) [web:25]
    inline Tile *getTile(int gx, int gy)
    {
        if (gx < 0 || gx >= width || gy < 0 || gy >= height)
            return nullptr;
        return &tiles[gy * width + gx];
    }

    void setTile(int gx, int gy, const Tile &t);

    // Coordinate conversion [web:26]
    Vector2 gridToWorld(int gx, int gy);
    void worldToGrid(Vector2 pos, int &gx, int &gy);

    // Paint operations [web:26]
    void paintRect(int cx, int cy, int radius, uint16 id);
    void paintCircle(int cx, int cy, float radius, uint16 id);
    void erase(int cx, int cy, int radius);
    void eraseCircle(int cx, int cy, float radius);
    void fill(int gx, int gy, uint16 id);

    // Collision (grid-based broadphase) [web:20][web:23]
    void getCollidingTiles(Rectangle bounds, std::vector<Rectangle> &out);
    void getCollidingSolids(Rectangle bounds, std::vector<Rectangle> &out);
    inline bool isSolid(int gx, int gy)
    {
        Tile *t = getTile(gx, gy);
        return t && t->solid;
    }

    // Render [web:26]
    void render(Vector2 scroll);
    void renderWithTexture(Vector2 scroll);
    void renderGrid(Vector2 scroll);

 
    bool save(const char *filename);
    bool load(const char *filename);

 
    int width, height;
    int tile_size;
    int tileset_id;
    int spacing, margin;
    int tileset_cols;
    int graph;
    GridType grid_type;

 
    int brush_id;
    float brush_radius;

private:
 
    Tile *tiles;
 
 
};

class TilemapEditor
{
public:
    Tilemap *tilemap;
    Vector2 scroll;

    TilemapEditor();

    void setTilemap(Tilemap *tm);
    void update(Vector2 mouse_pos);
    void handleInput();
    void render();
};

struct Layer
{
    std::vector<Entity *> nodes;
    int front;  // -1 se não tem
    int back;   // -1 se não tem
    uint8 mode; // tilex, tiley, stretch x, stretch y
    Rectangle size;
    // Parallax/scroll
    double scroll_factor_x; // 0.5 = parallax, 1.0 = normal
    double scroll_factor_y;
    double scroll_x; // offset de scroll
    double scroll_y;
    void destroy();
    void render();
    void render_parelax(Graph *g);
};

struct MainCamera
{
    double x, y;               // Posição atual da camera
    double target_x, target_y; // Alvo (normalmente o player)
    double smoothness;         // 0.0 = instantâneo, 0.9 = muito suave
    Rectangle bounds;          // Limites do mundo
    bool use_bounds;

    MainCamera()
    {
        x = y = 0;
        target_x = target_y = 0;
        smoothness = 0.1;
        bounds = {0, 0, 0, 0};
        use_bounds = false;
    }

    void setTarget(double tx, double ty);
    void update(double deltaTime);
    void setBounds(double minX, double minY, double maxX, double maxY);
};

struct Scene
{
    std::vector<Entity *> nodesToRemove;
    std::vector<Entity *> staticEntities;  // Cache de estáticas
    std::vector<Entity *> dynamicEntities; // Cache de dinâmicas
    Quadtree *staticTree;
    double scroll_x, scroll_y;
    int width, height;

    CollisionCallback onCollision;
    void *collisionUserData;

    Layer layers[6];
    Entity *addEntity(int graphId, int layer, double x, double y);
    void moveEntityToLayer(Entity *node, int layer);
    void removeEntity(Entity *node);
    void destroy();

    // Collision
    void initCollision(Rectangle worldBounds);
    void updateCollision();
    void checkCollisions();
    void setCollisionCallback(CollisionCallback callback, void *userdata = nullptr);

    Scene();
    ~Scene();
};

struct GraphLib
{
    std::vector<Graph> graphs;
    std::vector<Texture2D> textures; // grafico fica com o index da textura carregada
    Texture2D defaultTexture;

    int load(const char *name, const char *texturePath);
    int loadAtlas(const char *name, const char *texturePath, int count_x, int count_y);
    int addSubGraph(int id, const char *name, int x, int iy, int iw, int ih);
    Graph *getGraph(int id);
    void create();
    void destroy();
};

enum PathHeuristic
{
    PF_MANHATTAN = 0,
    PF_EUCLIDEAN,
    PF_OCTILE,
    PF_CHEBYSHEV
};

enum PathAlgorithm
{
    PATH_ASTAR,
    PATH_DIJKSTRA
};

struct GridNode
{
    int walkable;
    float f, g, h;
    int opened; // 0=closed, 1=open, 2=closed
    int parent_idx;
    GridNode *next;
    GridNode *prev;
};

class Mask
{
private:
    GridNode *grid;
    int width, height;

    float manhattan(int dx, int dy);
    float euclidean(int dx, int dy);
    float octile(int dx, int dy);
    float chebyshev(int dx, int dy);

    float calcHeuristic(int dx, int dy, PathHeuristic heur, int diag);

    GridNode *node_add(GridNode *list, GridNode *node);
    GridNode *node_remove(GridNode *list, GridNode *node);

public:
    Mask(int w, int h);
    ~Mask();

    void setOccupied(int x, int y);
    void setFree(int x, int y);
    bool isOccupied(int x, int y) const;
    bool isWalkable(int x, int y) const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void loadFromImage(const char *imagePath, int threshold = 128);

    std::vector<Vector2> findPath(int sx, int sy, int ex, int ey,
                                  int diag = 1,
                                  PathAlgorithm algo = PATH_ASTAR,
                                  PathHeuristic heur = PF_MANHATTAN);
};
void InitScene();
void DestroyScene();
void RenderScene();
void InitCollision(int x, int y, int width, int height, CollisionCallback onCollision);
void UpdateCollision();
void CheckCollisions();

Entity *CreateEntity(int graphId, int layer, double x, double y);
void RemoveEntity(Entity *node);
void MoveEntityToLayer(Entity *node, int layer);

void SetLayerMode(int layer, uint8 mode);
void SetLayerScroll(int layer, double x, double y);
void SetLayerScrollFactor(int layer, double x, double y);
void SetLayerSize(int layer, int x, int y, int width, int height);
void SetLayerBackGraph(int layer, int graph);
void SetLayerFrontGraph(int layer, int graph);
void SetScroll(double x, double y);

int LoadGraph(const char *name, const char *texturePath);
int LoadAtlas(const char *name, const char *texturePath, int count_x, int count_y);
int LoadSubGraph(int id, const char *name, int x, int iy, int iw, int ih);