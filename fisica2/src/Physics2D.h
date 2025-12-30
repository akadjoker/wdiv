#ifndef PHYSICS2D_HPP
#define PHYSICS2D_HPP

#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include "raylib.h"

namespace Physics2D
{
    // Constantes
    constexpr float EPSILON = 0.000001f;
//     constexpr float PI = 3.14159265358979323846f;
 //    constexpr float DEG2RAD = PI / 180.0f;
    //constexpr float FLOAT_MAX = std::numeric_limits<float>::max();
      constexpr float FLT_MAX = 3.402823466e+38f;
    constexpr int MAX_VERTICES = 24;
    constexpr int CIRCLE_VERTICES = 24;
    constexpr int COLLISION_ITERATIONS = 20;
    constexpr float PENETRATION_ALLOWANCE = 0.05f;
    constexpr float PENETRATION_CORRECTION = 0.4f;

    // Forward declarations
    class Body;
    class World;
    class QuadtreeNode;
    class Quadtree;

    // ===== Vector2 =====
    struct Vec2
    {
        float x, y;

        Vec2(float x = 0.0f, float y = 0.0f);

        Vec2 operator+(const Vec2 &v) const;
        Vec2 operator-(const Vec2 &v) const;
        Vec2 operator*(float s) const;
        Vec2 operator/(float s) const;

        Vec2 &operator+=(const Vec2 &v);
        Vec2 &operator-=(const Vec2 &v);
        Vec2 &operator*=(float s);

        float length() const;
        float lengthSq() const;

        void normalize();
        Vec2 normalized() const;

        float dot(const Vec2 &v) const;
        float cross(const Vec2 &v) const;

        static Vec2 cross(float s, const Vec2 &v);
    };

    // ===== Matrix 2x2 =====
    struct Mat2
    {
        float m00, m01;
        float m10, m11;

        Mat2();
        Mat2(float radians);

        void set(float radians);
        Mat2 transpose() const;
        Vec2 multiply(const Vec2 &v) const;

    private:
        Mat2(float a, float b, float c, float d);
    };

    // ===== Shape Type =====
    enum class ShapeType
    {
        Circle,
        Polygon
    };

    // ===== Polygon Data =====
    struct PolygonData
    {
        int vertexCount;
        Vec2 positions[MAX_VERTICES];
        Vec2 normals[MAX_VERTICES];

        PolygonData();
    };

    // ===== AABB =====
    struct AABB
    {
        Vec2 min;
        Vec2 max;

        AABB();
        AABB(const Vec2 &min, const Vec2 &max);

        bool contains(const Vec2 &point) const;
        bool intersects(const AABB &other) const;
        Vec2 center() const;
        Vec2 size() const;
    };

    // ===== Quadtree Node =====
    class QuadtreeNode
    {
    private:
        static constexpr int MAX_OBJECTS = 4;
        static constexpr int MAX_LEVELS = 8;

        int level;
        AABB bounds;
        std::vector<Body *> objects;
        QuadtreeNode *children[4];
        bool divided;

    public:
        QuadtreeNode(int level, const AABB &bounds);
        ~QuadtreeNode();

        void clear();
        void subdivide();
        bool insert(Body *body);
        void query(const AABB &range, std::vector<Body *> &found) const;
        void queryAll(std::vector<Body *> &found) const;

        static AABB getBodyAABB(Body *body);

        const AABB &getBounds() const { return bounds; }
        bool isDivided() const { return divided; }
        const QuadtreeNode *getChild(int index) const;
    };

    // ===== Quadtree =====
    class Quadtree
    {
    private:
        QuadtreeNode *root;
        AABB worldBounds;

    public:
        Quadtree(const AABB &bounds);
        ~Quadtree();

        void clear();
        void insert(Body *body);
        void query(const AABB &range, std::vector<Body *> &found) const;
        void queryAll(std::vector<Body *> &found) const;
        std::vector<Body *> getPotentialCollisions(Body *body) const;

        const QuadtreeNode *getRoot() const { return root; }
    };

    // ===== Shape =====
    class Shape
    {
    public:
        ShapeType type;
        Body *body;
        float radius;
        Mat2 transform;
        PolygonData vertexData;

        Shape();
    };

    // ===== Physics Body =====
    class Body
    {
    public:
        unsigned int id;
        bool enabled;
        Vec2 position;
        Vec2 velocity;
        Vec2 force;
        float angularVelocity;
        float torque;
        float orient;
        float inertia;
        float inverseInertia;
        float mass;
        float inverseMass;
        float staticFriction;
        float dynamicFriction;
        float restitution;
        bool useGravity;
        bool isGrounded;
        bool freezeOrient;
        Shape shape;

        Body(unsigned int id);

        void addForce(const Vec2 &f);
        void addTorque(float t);
        void setRotation(float radians);
        Vec2 getVertex(int index) const;
    };

    // ===== Collision Manifold =====
    struct Manifold
    {
        Body *bodyA;
        Body *bodyB;
        float penetration;
        Vec2 normal;
        Vec2 contacts[2];
        int contactsCount;
        float restitution;
        float dynamicFriction;
        float staticFriction;

        Manifold();
    };

    // ===== Physics World =====
    class World
    {
    private:
        std::vector<Body *> bodies;
        std::vector<Manifold> manifolds;
        Vec2 gravity;
        double deltaTime;
        double accumulator;
        unsigned int stepCount;
        unsigned int nextBodyId;
        Quadtree *quadtree;
        bool useQuadtree;

    public:
        World();
        ~World();

        void setUseQuadtree(bool use);
        bool isUsingQuadtree() const;

        void setGravity(const Vec2 &g);
        void setGravity(float x, float y);
        void setTimeStep(double dt);

        Body *createCircle(const Vec2 &pos, float radius, float density);
        Body *createRectangle(const Vec2 &pos, float width, float height, float density);
        Body *createPolygon(const Vec2 &pos, float radius, int sides, float density);

        void destroyBody(Body *body);
        void step();

        int getBodyCount() const;
        Body *getBody(int index);

        const Quadtree *getQuadtree() const;
        int getCollisionChecks() const;

    private:
        // Collision detection helpers
        void solveDifferentShapes(Manifold &m, Body *circle, Body *polygon);
        Vec2 getSupport(Shape &shape, const Vec2 &dir);
        float findAxisLeastPenetration(int &faceIndex, Shape &shapeA, Shape &shapeB);
        void findIncidentFace(Vec2 &v0, Vec2 &v1, Shape &ref, Shape &inc, int index);
        int clip(const Vec2 &normal, float c, Vec2 &faceA, Vec2 &faceB);

        // Shape creation helpers
        PolygonData createRectanglePolygon(const Vec2 &pos, const Vec2 &size);
        PolygonData createRegularPolygon(float radius, int sides);
        void calculateNormals(PolygonData &data);

        // Collision solvers
        void solveCollision(Manifold &m);
        void solveCircleToCircle(Manifold &m);
        void solveCircleToPolygon(Manifold &m);
        void solvePolygonToCircle(Manifold &m);
        void solvePolygonToPolygon(Manifold &m);

        // Physics integration
        void integrateForces(Body *body);
        void integrateVelocity(Body *body);
        void initializeManifold(Manifold &m);
        void applyImpulse(Manifold &m);
        void correctPositions(Manifold &m);
    };

} // namespace Physics2D

#endif // PHYSICS2D_HPP