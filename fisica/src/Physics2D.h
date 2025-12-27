#ifndef PHYSICS2D_H
#define PHYSICS2D_H

#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace Physics2D {

// Constantes
 
constexpr float EPSILON = 0.000001f;
constexpr float FLT_MAX = 3.402823466e+38f;
constexpr int MAX_VERTICES = 24;
constexpr int CIRCLE_VERTICES = 24;
constexpr int COLLISION_ITERATIONS = 20;
constexpr float PENETRATION_ALLOWANCE = 0.05f;
constexpr float PENETRATION_CORRECTION = 0.4f;

// ===== Vector2 =====
struct Vec2 {
    float x, y;
    
    Vec2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
    
    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 operator/(float s) const { return Vec2(x / s, y / s); }
    
    Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
    Vec2& operator-=(const Vec2& v) { x -= v.x; y -= v.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    
    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSq() const { return x * x + y * y; }
    
    void normalize() {
        float len = length();
        if (len > EPSILON) {
            x /= len;
            y /= len;
        }
    }
    
    Vec2 normalized() const {
        Vec2 result = *this;
        result.normalize();
        return result;
    }
    
    float dot(const Vec2& v) const { return x * v.x + y * v.y; }
    float cross(const Vec2& v) const { return x * v.y - y * v.x; }
    
    static Vec2 cross(float s, const Vec2& v) { return Vec2(-s * v.y, s * v.x); }
};

// ===== Matrix 2x2 =====
struct Mat2 {
    float m00, m01;
    float m10, m11;
    
    Mat2() : m00(1), m01(0), m10(0), m11(1) {}
    Mat2(float radians) { set(radians); }
    
    void set(float radians) {
        float c = std::cos(radians);
        float s = std::sin(radians);
        m00 = c; m01 = -s;
        m10 = s; m11 = c;
    }
    
    Mat2 transpose() const {
        return Mat2{m00, m10, m01, m11};
    }
    
    Vec2 multiply(const Vec2& v) const {
        return Vec2(m00 * v.x + m01 * v.y, m10 * v.x + m11 * v.y);
    }
    
private:
    Mat2(float a, float b, float c, float d) : m00(a), m01(b), m10(c), m11(d) {}
};

// ===== Shape Type =====
enum class ShapeType {
    Circle,
    Polygon
};

// ===== Polygon Data =====
struct PolygonData {
    int vertexCount;
    Vec2 positions[MAX_VERTICES];
    Vec2 normals[MAX_VERTICES];
    
    PolygonData() : vertexCount(0) {}
};

// Forward declaration
class Body;

// ===== Shape =====
class Shape {
public:
    ShapeType type;
    Body* body;
    float radius;
    Mat2 transform;
    PolygonData vertexData;
    
    Shape() : type(ShapeType::Circle), body(nullptr), radius(0.0f) {}
};

// ===== Physics Body =====
class Body {
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
    
    Body(unsigned int id) : 
        id(id), enabled(true), position(0, 0), velocity(0, 0), force(0, 0),
        angularVelocity(0), torque(0), orient(0), inertia(0), inverseInertia(0),
        mass(0), inverseMass(0), staticFriction(0.4f), dynamicFriction(0.2f),
        restitution(0.0f), useGravity(true), isGrounded(false), freezeOrient(false) {
        shape.body = this;
    }
    
    void addForce(const Vec2& f) {
        force += f;
    }
    
    void addTorque(float t) {
        torque += t;
    }
    
    void setRotation(float radians) {
        orient = radians;
        if (shape.type == ShapeType::Polygon) {
            shape.transform.set(radians);
        }
    }
    
    Vec2 getVertex(int index) const {
        if (shape.type == ShapeType::Circle) {
            float angle = 360.0f / CIRCLE_VERTICES * index * DEG2RAD;
            return Vec2(
                position.x + std::cos(angle) * shape.radius,
                position.y + std::sin(angle) * shape.radius
            );
        } else {
            return position + shape.transform.multiply(shape.vertexData.positions[index]);
        }
    }
};

// ===== Collision Manifold =====
struct Manifold {
    Body* bodyA;
    Body* bodyB;
    float penetration;
    Vec2 normal;
    Vec2 contacts[2];
    int contactsCount;
    float restitution;
    float dynamicFriction;
    float staticFriction;
    
    Manifold() : bodyA(nullptr), bodyB(nullptr), penetration(0),
                 contactsCount(0), restitution(0), dynamicFriction(0), staticFriction(0) {}
};

// ===== Physics World =====
class World {
private:
    std::vector<Body*> bodies;
    std::vector<Manifold> manifolds;
    Vec2 gravity;
    double deltaTime;
    double accumulator;
    unsigned int stepCount;
    unsigned int nextBodyId;
    
public:
    World() : gravity(0, 9.81f), deltaTime(1.0 / 60.0), 
              accumulator(0), stepCount(0), nextBodyId(0) {}
    
    ~World() {
        for (Body* body : bodies) {
            delete body;
        }
    }
    
    void setGravity(const Vec2& g) { gravity = g; }
    void setGravity(float x, float y) { gravity = Vec2(x, y); }
    void setTimeStep(double dt) { deltaTime = dt / 1000.0; }
    
    Body* createCircle(const Vec2& pos, float radius, float density) {
        Body* body = new Body(nextBodyId++);
        body->position = pos;
        body->shape.type = ShapeType::Circle;
        body->shape.radius = radius;
        
        body->mass = PI * radius * radius * density;
        body->inverseMass = (body->mass != 0.0f) ? 1.0f / body->mass : 0.0f;
        body->inertia = body->mass * radius * radius;
        body->inverseInertia = (body->inertia != 0.0f) ? 1.0f / body->inertia : 0.0f;
        
        bodies.push_back(body);
        return body;
    }
    
    Body* createRectangle(const Vec2& pos, float width, float height, float density) {
        Body* body = new Body(nextBodyId++);
        body->position = pos;
        body->shape.type = ShapeType::Polygon;
        body->shape.vertexData = createRectanglePolygon(pos, Vec2(width, height));
        
        calculateMassAndInertia(body, density);
        bodies.push_back(body);
        return body;
    }
    
    Body* createPolygon(const Vec2& pos, float radius, int sides, float density) {
        Body* body = new Body(nextBodyId++);
        body->position = pos;
        body->shape.type = ShapeType::Polygon;
        body->shape.vertexData = createRegularPolygon(radius, sides);
        
        calculateMassAndInertia(body, density);
        bodies.push_back(body);
        return body;
    }
    
    void destroyBody(Body* body) {
        auto it = std::find(bodies.begin(), bodies.end(), body);
        if (it != bodies.end()) {
            bodies.erase(it);
            delete body;
        }
    }
    
    void step() {
        // Limpar colisões anteriores
        manifolds.clear();
        
        // Reset grounded state
        for (Body* body : bodies) {
            body->isGrounded = false;
        }
        
        // Detectar colisões
        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                Body* a = bodies[i];
                Body* b = bodies[j];
                
                if (a->inverseMass == 0 && b->inverseMass == 0) continue;
                
                Manifold m;
                m.bodyA = a;
                m.bodyB = b;
                solveCollision(m);
                
                if (m.contactsCount > 0) {
                    manifolds.push_back(m);
                }
            }
        }
        
        // Integrar forças
        for (Body* body : bodies) {
            integrateForces(body);
        }
        
        // Inicializar manifolds
        for (Manifold& m : manifolds) {
            initializeManifold(m);
        }
        
        // Resolver colisões
        for (int i = 0; i < COLLISION_ITERATIONS; ++i) {
            for (Manifold& m : manifolds) {
                applyImpulse(m);
            }
        }
        
        // Integrar velocidades
        for (Body* body : bodies) {
            integrateVelocity(body);
        }
        
        // Corrigir posições
        for (Manifold& m : manifolds) {
            correctPositions(m);
        }
        
        // Limpar forças
        for (Body* body : bodies) {
            body->force = Vec2(0, 0);
            body->torque = 0;
        }
        
        stepCount++;
    }
    
    int getBodyCount() const { return bodies.size(); }
    Body* getBody(int index) {
        if (index >= 0 && index < (int)bodies.size()) {
            return bodies[index];
        }
        return nullptr;
    }
    
private:
 
    
    PolygonData createRectanglePolygon(const Vec2& pos, const Vec2& size) {
        PolygonData data;
        data.vertexCount = 4;
        
        // Criar retângulo centrado na origem (será transladado depois)
        float halfW = size.x / 2.0f;
        float halfH = size.y / 2.0f;
        
        data.positions[0] = Vec2(halfW, -halfH);
        data.positions[1] = Vec2(halfW, halfH);
        data.positions[2] = Vec2(-halfW, halfH);
        data.positions[3] = Vec2(-halfW, -halfH);
        
        calculateNormals(data);
        return data;
    }
    
    PolygonData createRegularPolygon(float radius, int sides) {
        PolygonData data;
        data.vertexCount = sides;
        
        for (int i = 0; i < sides; ++i) {
            float angle = 360.0f / sides * i * DEG2RAD;
            data.positions[i] = Vec2(
                std::cos(angle) * radius,
                std::sin(angle) * radius
            );
        }
        
        calculateNormals(data);
        return data;
    }
    
    void calculateNormals(PolygonData& data) {
        for (int i = 0; i < data.vertexCount; ++i) {
            int next = (i + 1) % data.vertexCount;
            Vec2 face = data.positions[next] - data.positions[i];
            data.normals[i] = Vec2(face.y, -face.x).normalized();
        }
    }
    
    void calculateMassAndInertia(Body* body, float density) {
        Vec2 center(0, 0);
        float area = 0;
        float inertia = 0;
        
        PolygonData& vd = body->shape.vertexData;
        
        for (int i = 0; i < vd.vertexCount; ++i) {
            Vec2 p1 = vd.positions[i];
            Vec2 p2 = vd.positions[(i + 1) % vd.vertexCount];
            
            float D = p1.cross(p2);
            float triangleArea = D / 2.0f;
            
            area += triangleArea;
            center += Vec2(p1.x + p2.x, p1.y + p2.y) * triangleArea / 3.0f;
            
            float intx2 = p1.x*p1.x + p2.x*p1.x + p2.x*p2.x;
            float inty2 = p1.y*p1.y + p2.y*p1.y + p2.y*p2.y;
            inertia += (0.25f / 3.0f * D) * (intx2 + inty2);
        }
        
        center = center / area;
        
        // Transladar vértices para o centroide
        for (int i = 0; i < vd.vertexCount; ++i) {
            vd.positions[i] -= center;
        }
        
        body->mass = density * std::abs(area);
        body->inverseMass = (body->mass != 0.0f) ? 1.0f / body->mass : 0.0f;
        body->inertia = density * std::abs(inertia);
        body->inverseInertia = (body->inertia != 0.0f) ? 1.0f / body->inertia : 0.0f;
    }
    
    void solveCollision(Manifold& m) {
        if (m.bodyA->shape.type == ShapeType::Circle && m.bodyB->shape.type == ShapeType::Circle) {
            solveCircleToCircle(m);
        } else if (m.bodyA->shape.type == ShapeType::Polygon && m.bodyB->shape.type == ShapeType::Polygon) {
            solvePolygonToPolygon(m);
        } else {
            if (m.bodyA->shape.type == ShapeType::Circle) {
                solveCircleToPolygon(m);
            } else {
                solvePolygonToCircle(m);
            }
        }
    }
    
    void solveCircleToCircle(Manifold& m) {
        Vec2 normal = m.bodyB->position - m.bodyA->position;
        float distSq = normal.lengthSq();
        float radius = m.bodyA->shape.radius + m.bodyB->shape.radius;
        
        if (distSq >= radius * radius) {
            m.contactsCount = 0;
            return;
        }
        
        float dist = std::sqrt(distSq);
        m.contactsCount = 1;
        
        if (dist < EPSILON) {
            m.penetration = m.bodyA->shape.radius;
            m.normal = Vec2(1, 0);
            m.contacts[0] = m.bodyA->position;
        } else {
            m.penetration = radius - dist;
            m.normal = normal / dist;
            m.contacts[0] = m.bodyA->position + m.normal * m.bodyA->shape.radius;
        }
        
        if (!m.bodyA->isGrounded) {
            m.bodyA->isGrounded = (m.normal.y < 0);
        }
    }
    
    void solveCircleToPolygon(Manifold& m) {
        solveDifferentShapes(m, m.bodyA, m.bodyB);
    }
    
    void solvePolygonToCircle(Manifold& m) {
        solveDifferentShapes(m, m.bodyB, m.bodyA);
        m.normal.x *= -1.0f;
        m.normal.y *= -1.0f;
    }
    
    void solveDifferentShapes(Manifold& m, Body* circle, Body* polygon) {
        m.contactsCount = 0;
        
        // Transformar centro do círculo para o espaço do polígono
        Vec2 center = circle->position;
        center = polygon->shape.transform.transpose().multiply(center - polygon->position);
        
        // Encontrar a aresta com menor penetração
        float separation = -FLT_MAX;
        int faceNormal = 0;
        PolygonData& vertexData = polygon->shape.vertexData;
        
        for (int i = 0; i < vertexData.vertexCount; ++i) {
            float s = vertexData.normals[i].dot(center - vertexData.positions[i]);
            
            if (s > circle->shape.radius) return;
            
            if (s > separation) {
                separation = s;
                faceNormal = i;
            }
        }
        
        // Pegar vértices da face
        Vec2 v1 = vertexData.positions[faceNormal];
        int nextIndex = (faceNormal + 1) % vertexData.vertexCount;
        Vec2 v2 = vertexData.positions[nextIndex];
        
        // Verificar se o centro está dentro do polígono
        if (separation < EPSILON) {
            m.contactsCount = 1;
            Vec2 normal = polygon->shape.transform.multiply(vertexData.normals[faceNormal]);
            m.normal = Vec2(-normal.x, -normal.y);
            m.contacts[0] = circle->position + m.normal * circle->shape.radius;
            m.penetration = circle->shape.radius;
            return;
        }
        
        // Determinar região de Voronoi
        float dot1 = (center - v1).dot(v2 - v1);
        float dot2 = (center - v2).dot(v1 - v2);
        m.penetration = circle->shape.radius - separation;
        
        if (dot1 <= 0.0f) { // Mais próximo de v1
            if ((center - v1).lengthSq() > circle->shape.radius * circle->shape.radius)
                return;
            
            m.contactsCount = 1;
            Vec2 normal = v1 - center;
            normal = polygon->shape.transform.multiply(normal);
            normal.normalize();
            m.normal = normal;
            v1 = polygon->shape.transform.multiply(v1);
            v1 = v1 + polygon->position;
            m.contacts[0] = v1;
        }
        else if (dot2 <= 0.0f) { // Mais próximo de v2
            if ((center - v2).lengthSq() > circle->shape.radius * circle->shape.radius)
                return;
            
            m.contactsCount = 1;
            Vec2 normal = v2 - center;
            v2 = polygon->shape.transform.multiply(v2);
            v2 = v2 + polygon->position;
            m.contacts[0] = v2;
            normal = polygon->shape.transform.multiply(normal);
            normal.normalize();
            m.normal = normal;
        }
        else { // Mais próximo da face
            Vec2 normal = vertexData.normals[faceNormal];
            
            if ((center - v1).dot(normal) > circle->shape.radius)
                return;
            
            normal = polygon->shape.transform.multiply(normal);
            m.normal = Vec2(-normal.x, -normal.y);
            m.contacts[0] = circle->position + m.normal * circle->shape.radius;
            m.contactsCount = 1;
        }
    }
    
    void solvePolygonToPolygon(Manifold& m) {
        Shape& shapeA = m.bodyA->shape;
        Shape& shapeB = m.bodyB->shape;
        m.contactsCount = 0;
        
        // Verificar eixo de separação com faces de A
        int faceA = 0;
        float penetrationA = findAxisLeastPenetration(faceA, shapeA, shapeB);
        if (penetrationA >= 0.0f) return;
        
        // Verificar eixo de separação com faces de B
        int faceB = 0;
        float penetrationB = findAxisLeastPenetration(faceB, shapeB, shapeA);
        if (penetrationB >= 0.0f) return;
        
        int referenceIndex = 0;
        bool flip = false;
        
        Shape* refPoly;
        Shape* incPoly;
        
        // Determinar qual shape contém a face de referência (bias para estabilidade)
        const float k_relativeTol = 0.95f;
        const float k_absoluteTol = 0.01f;
        
        if (penetrationA >= penetrationB * k_relativeTol + penetrationA * k_absoluteTol) {
            refPoly = &shapeA;
            incPoly = &shapeB;
            referenceIndex = faceA;
            flip = false;
        } else {
            refPoly = &shapeB;
            incPoly = &shapeA;
            referenceIndex = faceB;
            flip = true;
        }
        
        // Face incidente no espaço mundial
        Vec2 incidentFace[2];
        findIncidentFace(incidentFace[0], incidentFace[1], *refPoly, *incPoly, referenceIndex);
        
        // Vértices da face de referência
        PolygonData& refData = refPoly->vertexData;
        Vec2 v1 = refData.positions[referenceIndex];
        referenceIndex = (referenceIndex + 1) % refData.vertexCount;
        Vec2 v2 = refData.positions[referenceIndex];
        
        // Transformar para espaço mundial
        v1 = refPoly->transform.multiply(v1) + refPoly->body->position;
        v2 = refPoly->transform.multiply(v2) + refPoly->body->position;
        
        // Calcular normal lateral da face de referência
        Vec2 sidePlaneNormal = (v2 - v1).normalized();
        
        // Ortogonalizar
        Vec2 refFaceNormal(sidePlaneNormal.y, -sidePlaneNormal.x);
        float refC = refFaceNormal.dot(v1);
        float negSide = -sidePlaneNormal.dot(v1);
        float posSide = sidePlaneNormal.dot(v2);
        
        // Clipar face incidente aos planos laterais da face de referência
        if (clip(Vec2(-sidePlaneNormal.x, -sidePlaneNormal.y), negSide, incidentFace[0], incidentFace[1]) < 2)
            return;
        
        if (clip(sidePlaneNormal, posSide, incidentFace[0], incidentFace[1]) < 2)
            return;
        
        // Inverter normal se necessário
        m.normal = flip ? Vec2(-refFaceNormal.x, -refFaceNormal.y) : refFaceNormal;
        
        // Manter pontos atrás da face de referência
        int currentPoint = 0;
        float separation = refFaceNormal.dot(incidentFace[0]) - refC;
        
        if (separation <= 0.0f) {
            m.contacts[currentPoint] = incidentFace[0];
            m.penetration = -separation;
            currentPoint++;
        } else {
            m.penetration = 0.0f;
        }
        
        separation = refFaceNormal.dot(incidentFace[1]) - refC;
        
        if (separation <= 0.0f) {
            m.contacts[currentPoint] = incidentFace[1];
            m.penetration += -separation;
            currentPoint++;
            
            if (currentPoint > 0)
                m.penetration /= currentPoint;
        }
        
        m.contactsCount = currentPoint;
        
        // Atualizar grounded state
        if (!m.bodyB->isGrounded && m.contactsCount > 0) {
            m.bodyB->isGrounded = (m.normal.y < 0);
        }
    }
    
    Vec2 getSupport(Shape& shape, const Vec2& dir) {
        float bestProjection = -FLT_MAX;
        Vec2 bestVertex(0, 0);
        PolygonData& data = shape.vertexData;
        
        for (int i = 0; i < data.vertexCount; ++i) {
            Vec2 vertex = data.positions[i];
            float projection = vertex.dot(dir);
            
            if (projection > bestProjection) {
                bestVertex = vertex;
                bestProjection = projection;
            }
        }
        
        return bestVertex;
    }
    
    float findAxisLeastPenetration(int& faceIndex, Shape& shapeA, Shape& shapeB) {
        float bestDistance = -FLT_MAX;
        int bestIndex = 0;
        
        PolygonData& dataA = shapeA.vertexData;
        
        for (int i = 0; i < dataA.vertexCount; ++i) {
            Vec2 normal = dataA.normals[i];
            Vec2 transNormal = shapeA.transform.multiply(normal);
            
            Mat2 buT = shapeB.transform.transpose();
            normal = buT.multiply(transNormal);
            
            Vec2 support = getSupport(shapeB, Vec2(-normal.x, -normal.y));
            
            Vec2 vertex = dataA.positions[i];
            vertex = shapeA.transform.multiply(vertex);
            vertex = vertex + shapeA.body->position;
            vertex = vertex - shapeB.body->position;
            vertex = buT.multiply(vertex);
            
            float distance = normal.dot(support - vertex);
            
            if (distance > bestDistance) {
                bestDistance = distance;
                bestIndex = i;
            }
        }
        
        faceIndex = bestIndex;
        return bestDistance;
    }
    
    void findIncidentFace(Vec2& v0, Vec2& v1, Shape& ref, Shape& inc, int index) {
        PolygonData& refData = ref.vertexData;
        PolygonData& incData = inc.vertexData;
        
        Vec2 referenceNormal = refData.normals[index];
        
        referenceNormal = ref.transform.multiply(referenceNormal);
        referenceNormal = inc.transform.transpose().multiply(referenceNormal);
        
        int incidentFace = 0;
        float minDot = FLT_MAX;
        
        for (int i = 0; i < incData.vertexCount; ++i) {
            float dot = referenceNormal.dot(incData.normals[i]);
            
            if (dot < minDot) {
                minDot = dot;
                incidentFace = i;
            }
        }
        
        v0 = inc.transform.multiply(incData.positions[incidentFace]);
        v0 = v0 + inc.body->position;
        incidentFace = (incidentFace + 1) % incData.vertexCount;
        v1 = inc.transform.multiply(incData.positions[incidentFace]);
        v1 = v1 + inc.body->position;
    }
    
    int clip(const Vec2& normal, float c, Vec2& faceA, Vec2& faceB) {
        int sp = 0;
        Vec2 out[2] = { faceA, faceB };
        
        float distanceA = normal.dot(faceA) - c;
        float distanceB = normal.dot(faceB) - c;
        
        if (distanceA <= 0.0f) out[sp++] = faceA;
        if (distanceB <= 0.0f) out[sp++] = faceB;
        
        if (distanceA * distanceB < 0.0f) {
            float alpha = distanceA / (distanceA - distanceB);
            out[sp] = faceA;
            Vec2 delta = faceB - faceA;
            delta = delta * alpha;
            out[sp] = out[sp] + delta;
            sp++;
        }
        
        faceA = out[0];
        faceB = out[1];
        
        return sp;
    }
    
    void integrateForces(Body* body) {
        if (body->inverseMass == 0 || !body->enabled) return;
        
        body->velocity += (body->force * body->inverseMass) * (deltaTime / 2.0f);
        
        if (body->useGravity) {
            body->velocity += gravity * (deltaTime / 2.0f);
        }
        
        if (!body->freezeOrient) {
            body->angularVelocity += body->torque * body->inverseInertia * (deltaTime / 2.0f);
        }
    }
    
    void integrateVelocity(Body* body) {
        if (!body->enabled) return;
        
        body->position += body->velocity * deltaTime;
        
        if (!body->freezeOrient) {
            body->orient += body->angularVelocity * deltaTime;
        }
        
        body->shape.transform.set(body->orient);
        integrateForces(body);
    }
    
    void initializeManifold(Manifold& m) {
        m.restitution = std::sqrt(m.bodyA->restitution * m.bodyB->restitution);
        m.staticFriction = std::sqrt(m.bodyA->staticFriction * m.bodyB->staticFriction);
        m.dynamicFriction = std::sqrt(m.bodyA->dynamicFriction * m.bodyB->dynamicFriction);
        
        for (int i = 0; i < m.contactsCount; ++i) {
            Vec2 ra = m.contacts[i] - m.bodyA->position;
            Vec2 rb = m.contacts[i] - m.bodyB->position;
            
            Vec2 rv = m.bodyB->velocity + Vec2::cross(m.bodyB->angularVelocity, rb) -
                      m.bodyA->velocity - Vec2::cross(m.bodyA->angularVelocity, ra);
            
            // Se o movimento é causado apenas pela gravidade, sem restituição
            if (rv.lengthSq() < (gravity * deltaTime).lengthSq() + EPSILON) {
                m.restitution = 0.0f;
            }
        }
    }
    
    void applyImpulse(Manifold& m) {
        Body* a = m.bodyA;
        Body* b = m.bodyB;
        
        if (std::abs(a->inverseMass + b->inverseMass) <= EPSILON) {
            a->velocity = Vec2(0, 0);
            b->velocity = Vec2(0, 0);
            return;
        }
        
        for (int i = 0; i < m.contactsCount; ++i) {
            Vec2 ra = m.contacts[i] - a->position;
            Vec2 rb = m.contacts[i] - b->position;
            
            Vec2 rv = b->velocity + Vec2::cross(b->angularVelocity, rb) - 
                      a->velocity - Vec2::cross(a->angularVelocity, ra);
            
            float contactVel = rv.dot(m.normal);
            if (contactVel > 0) return;
            
            float raCrossN = ra.cross(m.normal);
            float rbCrossN = rb.cross(m.normal);
            
            float invMassSum = a->inverseMass + b->inverseMass + 
                              (raCrossN * raCrossN) * a->inverseInertia + 
                              (rbCrossN * rbCrossN) * b->inverseInertia;
            
            float j = -(1.0f + m.restitution) * contactVel;
            j /= invMassSum;
            j /= m.contactsCount;
            
            Vec2 impulse = m.normal * j;
            
            if (a->enabled) {
                a->velocity -= impulse * a->inverseMass;
                if (!a->freezeOrient) {
                    a->angularVelocity -= a->inverseInertia * ra.cross(impulse);
                }
            }
            
            if (b->enabled) {
                b->velocity += impulse * b->inverseMass;
                if (!b->freezeOrient) {
                    b->angularVelocity += b->inverseInertia * rb.cross(impulse);
                }
            }
            
            // Aplicar impulso de fricção
            rv = b->velocity + Vec2::cross(b->angularVelocity, rb) - 
                 a->velocity - Vec2::cross(a->angularVelocity, ra);
            
            Vec2 tangent = rv - m.normal * rv.dot(m.normal);
            tangent.normalize();
            
            float jt = -rv.dot(tangent);
            jt /= invMassSum;
            jt /= m.contactsCount;
            
            if (std::abs(jt) < EPSILON) continue;
            
            Vec2 tangentImpulse;
            if (std::abs(jt) < j * m.staticFriction) {
                tangentImpulse = tangent * jt;
            } else {
                tangentImpulse = tangent * (-j * m.dynamicFriction);
            }
            
            if (a->enabled) {
                a->velocity -= tangentImpulse * a->inverseMass;
                if (!a->freezeOrient) {
                    a->angularVelocity -= a->inverseInertia * ra.cross(tangentImpulse);
                }
            }
            
            if (b->enabled) {
                b->velocity += tangentImpulse * b->inverseMass;
                if (!b->freezeOrient) {
                    b->angularVelocity += b->inverseInertia * rb.cross(tangentImpulse);
                }
            }
        }
    }
    
    void correctPositions(Manifold& m) {
        Body* a = m.bodyA;
        Body* b = m.bodyB;
        
        float correction = std::max(m.penetration - PENETRATION_ALLOWANCE, 0.0f) / 
                          (a->inverseMass + b->inverseMass) * PENETRATION_CORRECTION;
        
        Vec2 correctionVec = m.normal * correction;
        
        if (a->enabled) {
            a->position -= correctionVec * a->inverseMass;
        }
        
        if (b->enabled) {
            b->position += correctionVec * b->inverseMass;
        }
    }
};

} // namespace Physics2D

#endif // PHYSICS2D_H