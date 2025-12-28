#include "Physics2D.h"
#include <limits>

namespace Physics2D
{
    // ===== Vec2 Implementation =====
    Vec2::Vec2(float x, float y) : x(x), y(y) {}

    Vec2 Vec2::operator+(const Vec2 &v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 Vec2::operator-(const Vec2 &v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 Vec2::operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 Vec2::operator/(float s) const { return Vec2(x / s, y / s); }

    Vec2 &Vec2::operator+=(const Vec2 &v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }

    Vec2 &Vec2::operator-=(const Vec2 &v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    Vec2 &Vec2::operator*=(float s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    float Vec2::length() const { return std::sqrt(x * x + y * y); }
    float Vec2::lengthSq() const { return x * x + y * y; }

    void Vec2::normalize()
    {
        float len = length();
        if (len > EPSILON)
        {
            x /= len;
            y /= len;
        }
    }

    Vec2 Vec2::normalized() const
    {
        Vec2 result = *this;
        result.normalize();
        return result;
    }

    float Vec2::dot(const Vec2 &v) const { return x * v.x + y * v.y; }
    float Vec2::cross(const Vec2 &v) const { return x * v.y - y * v.x; }
    Vec2 Vec2::cross(float s, const Vec2 &v) { return Vec2(-s * v.y, s * v.x); }

    // ===== Mat2 Implementation =====
    Mat2::Mat2() : m00(1), m01(0), m10(0), m11(1) {}
    Mat2::Mat2(float radians) { set(radians); }
    Mat2::Mat2(float a, float b, float c, float d) : m00(a), m01(b), m10(c), m11(d) {}

    void Mat2::set(float radians)
    {
        float c = std::cos(radians);
        float s = std::sin(radians);
        m00 = c;
        m01 = -s;
        m10 = s;
        m11 = c;
    }

    Mat2 Mat2::transpose() const
    {
        return Mat2(m00, m10, m01, m11);
    }

    Vec2 Mat2::multiply(const Vec2 &v) const
    {
        return Vec2(m00 * v.x + m01 * v.y, m10 * v.x + m11 * v.y);
    }

    // ===== PolygonData Implementation =====
    PolygonData::PolygonData() : vertexCount(0) {}

    // ===== AABB Implementation =====
    AABB::AABB() : min(0, 0), max(0, 0) {}
    AABB::AABB(const Vec2 &min, const Vec2 &max) : min(min), max(max) {}

    bool AABB::contains(const Vec2 &point) const
    {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }

    bool AABB::intersects(const AABB &other) const
    {
        return !(max.x < other.min.x || min.x > other.max.x ||
                 max.y < other.min.y || min.y > other.max.y);
    }

    Vec2 AABB::center() const
    {
        return Vec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
    }

    Vec2 AABB::size() const
    {
        return Vec2(max.x - min.x, max.y - min.y);
    }

    // ===== QuadtreeNode Implementation =====
    QuadtreeNode::QuadtreeNode(int level, const AABB &bounds)
        : level(level), bounds(bounds), divided(false)
    {
        for (int i = 0; i < 4; ++i)
        {
            children[i] = nullptr;
        }
    }

    QuadtreeNode::~QuadtreeNode()
    {
        clear();
    }

    void QuadtreeNode::clear()
    {
        objects.clear();

        if (divided)
        {
            for (int i = 0; i < 4; ++i)
            {
                delete children[i];
                children[i] = nullptr;
            }
            divided = false;
        }
    }

    void QuadtreeNode::subdivide()
    {
        if (divided)
            return;

        Vec2 center = bounds.center();

        // Top-left
        children[0] = new QuadtreeNode(level + 1,
                                       AABB(bounds.min, center));

        // Top-right
        children[1] = new QuadtreeNode(level + 1,
                                       AABB(Vec2(center.x, bounds.min.y), Vec2(bounds.max.x, center.y)));

        // Bottom-left
        children[2] = new QuadtreeNode(level + 1,
                                       AABB(Vec2(bounds.min.x, center.y), Vec2(center.x, bounds.max.y)));

        // Bottom-right
        children[3] = new QuadtreeNode(level + 1,
                                       AABB(center, bounds.max));

        divided = true;
    }

    bool QuadtreeNode::insert(Body *body)
    {
        AABB bodyAABB = getBodyAABB(body);

        if (!bounds.intersects(bodyAABB))
        {
            return false;
        }

        if (objects.size() < MAX_OBJECTS || level >= MAX_LEVELS)
        {
            objects.push_back(body);
            return true;
        }

        if (!divided)
        {
            subdivide();
        }

        bool inserted = false;
        for (int i = 0; i < 4; ++i)
        {
            if (children[i]->insert(body))
            {
                inserted = true;
            }
        }

        if (!inserted)
        {
            objects.push_back(body);
        }

        return true;
    }

    void QuadtreeNode::query(const AABB &range, std::vector<Body *> &found) const
    {
        if (!bounds.intersects(range))
        {
            return;
        }

        for (Body *body : objects)
        {
            AABB bodyAABB = getBodyAABB(body);
            if (range.intersects(bodyAABB))
            {
                found.push_back(body);
            }
        }

        if (divided)
        {
            for (int i = 0; i < 4; ++i)
            {
                children[i]->query(range, found);
            }
        }
    }

    void QuadtreeNode::queryAll(std::vector<Body *> &found) const
    {
        for (Body *body : objects)
        {
            found.push_back(body);
        }

        if (divided)
        {
            for (int i = 0; i < 4; ++i)
            {
                children[i]->queryAll(found);
            }
        }
    }

    AABB QuadtreeNode::getBodyAABB(Body *body)
    {
        if (body->shape.type == ShapeType::Circle)
        {
            float r = body->shape.radius;
            return AABB(
                Vec2(body->position.x - r, body->position.y - r),
                Vec2(body->position.x + r, body->position.y + r));
        }
        else
        {
            Vec2 min(FLT_MAX, FLT_MAX);
            Vec2 max(-FLT_MAX, -FLT_MAX);

            for (int i = 0; i < body->shape.vertexData.vertexCount; ++i)
            {
                Vec2 v = body->getVertex(i);
                min.x = std::min(min.x, v.x);
                min.y = std::min(min.y, v.y);
                max.x = std::max(max.x, v.x);
                max.y = std::max(max.y, v.y);
            }

            return AABB(min, max);
        }
    }

    const QuadtreeNode *QuadtreeNode::getChild(int index) const
    {
        return (divided && index >= 0 && index < 4) ? children[index] : nullptr;
    }

    // ===== Quadtree Implementation =====
    Quadtree::Quadtree(const AABB &bounds) : worldBounds(bounds)
    {
        root = new QuadtreeNode(0, bounds);
    }

    Quadtree::~Quadtree()
    {
        delete root;
    }

    void Quadtree::clear()
    {
        delete root;
        root = new QuadtreeNode(0, worldBounds);
    }

    void Quadtree::insert(Body *body)
    {
        root->insert(body);
    }

    void Quadtree::query(const AABB &range, std::vector<Body *> &found) const
    {
        root->query(range, found);
    }

    void Quadtree::queryAll(std::vector<Body *> &found) const
    {
        root->queryAll(found);
    }

    std::vector<Body *> Quadtree::getPotentialCollisions(Body *body) const
    {
        std::vector<Body *> potentials;
        AABB bodyAABB = QuadtreeNode::getBodyAABB(body);

        float margin = 10.0f;
        AABB expandedAABB(
            Vec2(bodyAABB.min.x - margin, bodyAABB.min.y - margin),
            Vec2(bodyAABB.max.x + margin, bodyAABB.max.y + margin));

        query(expandedAABB, potentials);
        return potentials;
    }

    // ===== Shape Implementation =====
    Shape::Shape() : type(ShapeType::Circle), body(nullptr), radius(0.0f) {}

    // ===== Body Implementation =====
    Body::Body(unsigned int id)
        : id(id), enabled(true), position(0, 0), velocity(0, 0), force(0, 0),
          angularVelocity(0), torque(0), orient(0), inertia(0), inverseInertia(0),
          mass(0), inverseMass(0), staticFriction(0.4f), dynamicFriction(0.2f),
          restitution(0.0f), useGravity(true), isGrounded(false), freezeOrient(false),
          linearDamping(0.99f), angularDamping(0.98f), layer(0), collisionMask(0xFFFFFFFF),
          userData(nullptr)
    {
        shape.body = this;
    }

    void Body::addForce(const Vec2 &f)
    {
        force += f;
    }

    void Body::addTorque(float t)
    {
        torque += t;
    }

    void Body::setRotation(float radians)
    {
        orient = radians;
        if (shape.type == ShapeType::Polygon)
        {
            shape.transform.set(radians);
        }
    }

    Vec2 Body::getVertex(int index) const
    {
        if (shape.type == ShapeType::Circle)
        {
            float angle = 360.0f / CIRCLE_VERTICES * index * DEG2RAD;
            return Vec2(
                position.x + std::cos(angle) * shape.radius,
                position.y + std::sin(angle) * shape.radius);
        }
        else
        {
            return position + shape.transform.multiply(shape.vertexData.positions[index]);
        }
    }

    // ===== Manifold Implementation =====
    Manifold::Manifold()
        : bodyA(nullptr), bodyB(nullptr), penetration(0),
          contactsCount(0), restitution(0), dynamicFriction(0), staticFriction(0) {}

    // ===== World Implementation =====
    World::World()
        : gravity(0, 9.81f), deltaTime(1.0 / 60.0),
          accumulator(0), stepCount(0), nextBodyId(0), useQuadtree(true),
          collisionIterations(COLLISION_ITERATIONS),
          positionCorrectionPercent(PENETRATION_CORRECTION),
          penetrationAllowance(PENETRATION_ALLOWANCE)
    {
        quadtree = new Quadtree(AABB(Vec2(-100, -100), Vec2(1500, 900)));
    }

    World::~World()
    {
        for (Body *body : bodies)
        {
            delete body;
        }
        delete quadtree;
    }

    void World::setUseQuadtree(bool use) { useQuadtree = use; }
    bool World::isUsingQuadtree() const { return useQuadtree; }

    void World::setGravity(const Vec2 &g) { gravity = g; }
    void World::setGravity(float x, float y) { gravity = Vec2(x, y); }
    void World::setTimeStep(double dt) { deltaTime = dt / 1000.0; }

    Body *World::createCircle(const Vec2 &pos, float radius, float density)
    {
        Body *body = new Body(nextBodyId++);
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

    Body *World::createRectangle(const Vec2 &pos, float width, float height, float density)
    {
        Body *body = new Body(nextBodyId++);
        body->position = pos;
        body->shape.type = ShapeType::Polygon;
        body->shape.vertexData = createRectanglePolygon(Vec2(0, 0), Vec2(width, height));

        Vec2 center(0, 0);
        float area = 0.0f;
        float inertia = 0.0f;

        PolygonData &vertexData = body->shape.vertexData;

        for (int i = 0; i < vertexData.vertexCount; ++i)
        {
            Vec2 p1 = vertexData.positions[i];
            int nextIndex = ((i + 1) < vertexData.vertexCount) ? (i + 1) : 0;
            Vec2 p2 = vertexData.positions[nextIndex];

            float D = p1.cross(p2);
            float triangleArea = D / 2.0f;

            area += triangleArea;
            center.x += triangleArea * (1.0f / 3.0f) * (p1.x + p2.x);
            center.y += triangleArea * (1.0f / 3.0f) * (p1.y + p2.y);

            float intx2 = p1.x * p1.x + p2.x * p1.x + p2.x * p2.x;
            float inty2 = p1.y * p1.y + p2.y * p1.y + p2.y * p2.y;
            inertia += (0.25f * (1.0f / 3.0f) * D) * (intx2 + inty2);
        }

        center.x *= 1.0f / area;
        center.y *= 1.0f / area;

        for (int i = 0; i < vertexData.vertexCount; ++i)
        {
            vertexData.positions[i].x -= center.x;
            vertexData.positions[i].y -= center.y;
        }

        body->mass = density * area;
        body->inverseMass = (body->mass != 0.0f) ? 1.0f / body->mass : 0.0f;
        body->inertia = density * inertia;
        body->inverseInertia = (body->inertia != 0.0f) ? 1.0f / body->inertia : 0.0f;

        bodies.push_back(body);
        return body;
    }

    Body *World::createPolygon(const Vec2 &pos, float radius, int sides, float density)
    {
        Body *body = new Body(nextBodyId++);
        body->position = pos;
        body->shape.type = ShapeType::Polygon;
        body->shape.vertexData = createRegularPolygon(radius, sides);

        Vec2 center(0, 0);
        float area = 0.0f;
        float inertia = 0.0f;

        PolygonData &vertexData = body->shape.vertexData;

        for (int i = 0; i < vertexData.vertexCount; ++i)
        {
            Vec2 position1 = vertexData.positions[i];
            int nextIndex = ((i + 1) < vertexData.vertexCount) ? (i + 1) : 0;
            Vec2 position2 = vertexData.positions[nextIndex];

            float cross = position1.cross(position2);
            float triangleArea = cross / 2.0f;

            area += triangleArea;
            center.x += triangleArea * (1.0f / 3.0f) * (position1.x + position2.x);
            center.y += triangleArea * (1.0f / 3.0f) * (position1.y + position2.y);

            float intx2 = position1.x * position1.x + position2.x * position1.x + position2.x * position2.x;
            float inty2 = position1.y * position1.y + position2.y * position1.y + position2.y * position2.y;
            inertia += (0.25f * (1.0f / 3.0f) * cross) * (intx2 + inty2);
        }

        center.x *= 1.0f / area;
        center.y *= 1.0f / area;

        for (int i = 0; i < vertexData.vertexCount; ++i)
        {
            vertexData.positions[i].x -= center.x;
            vertexData.positions[i].y -= center.y;
        }

        body->mass = density * area;
        body->inverseMass = (body->mass != 0.0f) ? 1.0f / body->mass : 0.0f;
        body->inertia = density * inertia;
        body->inverseInertia = (body->inertia != 0.0f) ? 1.0f / body->inertia : 0.0f;

        bodies.push_back(body);
        return body;
    }

    void World::destroyBody(Body *body)
    {
        auto it = std::find(bodies.begin(), bodies.end(), body);
        if (it != bodies.end())
        {
            bodies.erase(it);
            delete body;
        }
    }

    void World::step()
    {
        manifolds.clear();

        if (useQuadtree)
        {
            quadtree->clear();
            for (Body *body : bodies)
            {
                quadtree->insert(body);
            }
        }

        for (Body *body : bodies)
        {
            body->isGrounded = false;
        }

        if (useQuadtree)
        {
            for (size_t i = 0; i < bodies.size(); ++i)
            {
                Body *a = bodies[i];
                if (a->inverseMass == 0)
                    continue;

                std::vector<Body *> potentials = quadtree->getPotentialCollisions(a);

                for (Body *b : potentials)
                {
                    if (a == b)
                        continue;
                    if (a->id >= b->id)
                        continue;
                    if (a->inverseMass == 0 && b->inverseMass == 0)
                        continue;

                    // Verificar collision mask
                    if (!a->canCollideWith(b))
                        continue;

                    Manifold m;
                    m.bodyA = a;
                    m.bodyB = b;
                    solveCollision(m);

                    if (m.contactsCount > 0)
                    {
                        manifolds.push_back(m);
                    }
                }
            }
        }
        else
        {
            for (size_t i = 0; i < bodies.size(); ++i)
            {
                for (size_t j = i + 1; j < bodies.size(); ++j)
                {
                    Body *a = bodies[i];
                    Body *b = bodies[j];

                    if (a->inverseMass == 0 && b->inverseMass == 0)
                        continue;

                    Manifold m;
                    m.bodyA = a;
                    m.bodyB = b;
                    solveCollision(m);

                    if (m.contactsCount > 0)
                    {
                        manifolds.push_back(m);
                    }
                }
            }
        }

        for (Body *body : bodies)
        {
            integrateForces(body);
        }

        for (Manifold &m : manifolds)
        {
            initializeManifold(m);
        }

        for (int i = 0; i < collisionIterations; ++i)
        {
            for (Manifold &m : manifolds)
            {
                applyImpulse(m);
            }
        }

        for (Body *body : bodies)
        {
            integrateVelocity(body);

            // Aplicar damping
            if (body->enabled && body->inverseMass > 0.0f)
            {
                body->velocity *= body->linearDamping;
                body->angularVelocity *= body->angularDamping;
            }
        }

        for (Manifold &m : manifolds)
        {
            correctPositions(m);
        }

        for (Body *body : bodies)
        {
            body->force = Vec2(0, 0);
            body->torque = 0;
        }

        stepCount++;
    }

    int World::getBodyCount() const { return bodies.size(); }

    Body *World::getBody(int index)
    {
        if (index >= 0 && index < (int)bodies.size())
        {
            return bodies[index];
        }
        return nullptr;
    }

    const Quadtree *World::getQuadtree() const { return quadtree; }
    int World::getCollisionChecks() const { return manifolds.size(); }

    // ===== Body - Novas Features =====

    void Body::applyImpulse(const Vec2 &impulse, const Vec2 &contactVector)
    {
        if (inverseMass == 0.0f)
            return;

        velocity += impulse * inverseMass;

        if (!freezeOrient)
        {
            angularVelocity += inverseInertia * contactVector.cross(impulse);
        }
    }

    void Body::applyLinearImpulse(const Vec2 &impulse)
    {
        if (inverseMass == 0.0f)
            return;
        velocity += impulse * inverseMass;
    }

    void Body::applyAngularImpulse(float impulse)
    {
        if (inverseInertia == 0.0f || freezeOrient)
            return;
        angularVelocity += impulse * inverseInertia;
    }

    Vec2 Body::getVelocityAtPoint(const Vec2 &point) const
    {
        Vec2 r = point - position;
        return velocity + Vec2::cross(angularVelocity, r);
    }

    float Body::getKineticEnergy() const
    {
        float linear = 0.5f * mass * velocity.lengthSq();
        float angular = 0.5f * inertia * angularVelocity * angularVelocity;
        return linear + angular;
    }

    void Body::setLayer(int newLayer, int mask)
    {
        layer = newLayer;
        collisionMask = (mask == -1) ? 0xFFFFFFFF : mask;
    }

    bool Body::canCollideWith(const Body *other) const
    {
        if (!other)
            return false;
        return (collisionMask & (1 << other->layer)) != 0;
    }

    // ===== World - Raycasting =====

    World::RaycastHit World::raycast(const Vec2 &origin, const Vec2 &direction, float maxDistance)
    {
        RaycastHit closestHit;
        closestHit.hit = false;
        closestHit.fraction = maxDistance;
        closestHit.body = nullptr;

        Vec2 dir = direction.normalized();

        for (Body *body : bodies)
        {
            RaycastHit hit;
            bool didHit = false;

            if (body->shape.type == ShapeType::Circle)
            {
                didHit = raycastCircle(body, origin, dir, hit);
            }
            else
            {
                didHit = raycastPolygon(body, origin, dir, hit);
            }

            if (didHit && hit.fraction < closestHit.fraction)
            {
                closestHit = hit;
                closestHit.body = body;
                closestHit.hit = true;
            }
        }

        return closestHit;
    }

    std::vector<World::RaycastHit> World::raycastAll(const Vec2 &origin, const Vec2 &direction, float maxDistance)
    {
        std::vector<RaycastHit> hits;
        Vec2 dir = direction.normalized();

        for (Body *body : bodies)
        {
            RaycastHit hit;
            bool didHit = false;

            if (body->shape.type == ShapeType::Circle)
            {
                didHit = raycastCircle(body, origin, dir, hit);
            }
            else
            {
                didHit = raycastPolygon(body, origin, dir, hit);
            }

            if (didHit && hit.fraction <= maxDistance)
            {
                hit.body = body;
                hit.hit = true;
                hits.push_back(hit);
            }
        }

        return hits;
    }

    bool World::raycastCircle(Body *body, const Vec2 &origin, const Vec2 &direction, RaycastHit &hit)
    {
        Vec2 m = origin - body->position;
        float b = m.dot(direction);
        float c = m.dot(m) - body->shape.radius * body->shape.radius;

        if (c > 0.0f && b > 0.0f)
            return false;

        float discr = b * b - c;
        if (discr < 0.0f)
            return false;

        float t = -b - std::sqrt(discr);
        if (t < 0.0f)
            t = 0.0f;

        hit.fraction = t;
        hit.point = origin + direction * t;
        hit.normal = (hit.point - body->position).normalized();

        return true;
    }

    bool World::raycastPolygon(Body *body, const Vec2 &origin, const Vec2 &direction, RaycastHit &hit)
    {
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();

        Vec2 p = body->position;
        Vec2 d = direction;

        PolygonData &vertexData = body->shape.vertexData;

        for (int i = 0; i < vertexData.vertexCount; ++i)
        {
            Vec2 v1 = body->getVertex(i);
            Vec2 v2 = body->getVertex((i + 1) % vertexData.vertexCount);

            Vec2 edge = v2 - v1;
            Vec2 normal = Vec2(edge.y, -edge.x).normalized();

            float numerator = normal.dot(v1 - origin);
            float denominator = normal.dot(d);

            if (std::abs(denominator) < EPSILON)
            {
                if (numerator < 0.0f)
                    return false;
                continue;
            }

            float t = numerator / denominator;

            if (denominator < 0.0f)
            {
                if (t > tMax)
                    return false;
                if (t > tMin)
                {
                    tMin = t;
                    hit.normal = normal;
                }
            }
            else
            {
                if (t < tMin)
                    return false;
                tMax = std::min(tMax, t);
            }
        }

        if (tMin > tMax)
            return false;

        hit.fraction = tMin;
        hit.point = origin + d * tMin;
        return true;
    }

    // ===== World - Query Shapes =====

    std::vector<Body *> World::queryAABB(const AABB &aabb)
    {
        std::vector<Body *> result;

        if (useQuadtree)
        {
            quadtree->query(aabb, result);
        }
        else
        {
            for (Body *body : bodies)
            {
                AABB bodyAABB = QuadtreeNode::getBodyAABB(body);
                if (aabb.intersects(bodyAABB))
                {
                    result.push_back(body);
                }
            }
        }

        return result;
    }

    std::vector<Body *> World::queryCircle(const Vec2 &center, float radius)
    {
        std::vector<Body *> result;
        AABB aabb(Vec2(center.x - radius, center.y - radius),
                  Vec2(center.x + radius, center.y + radius));

        std::vector<Body *> candidates = queryAABB(aabb);

        for (Body *body : candidates)
        {
            float distSq = (body->position - center).lengthSq();
            float radiusSum = radius + (body->shape.type == ShapeType::Circle ? body->shape.radius : 50.0f); // Aproximação para polígonos

            if (distSq <= radiusSum * radiusSum)
            {
                result.push_back(body);
            }
        }

        return result;
    }

    Body *World::queryPoint(const Vec2 &point)
    {
        for (Body *body : bodies)
        {
            if (body->shape.type == ShapeType::Circle)
            {
                float distSq = (point - body->position).lengthSq();
                if (distSq <= body->shape.radius * body->shape.radius)
                {
                    return body;
                }
            }
            else
            {
                // Algoritmo ponto-em-polígono (ray casting)
                int intersections = 0;
                PolygonData &vd = body->shape.vertexData;

                for (int i = 0; i < vd.vertexCount; ++i)
                {
                    Vec2 v1 = body->getVertex(i);
                    Vec2 v2 = body->getVertex((i + 1) % vd.vertexCount);

                    if ((v1.y > point.y) != (v2.y > point.y))
                    {
                        float xIntersect = (v2.x - v1.x) * (point.y - v1.y) / (v2.y - v1.y) + v1.x;
                        if (point.x < xIntersect)
                        {
                            intersections++;
                        }
                    }
                }

                if (intersections % 2 == 1)
                {
                    return body;
                }
            }
        }

        return nullptr;
    }

    // ===== World - Explosões =====
    void World::applyExplosion(const Vec2 &center, float force, float radius)
    {
        for (Body *body : bodies)
        {
            // Ignorar objetos estáticos
            if (body->inverseMass == 0.0f)
                continue;
            if (!body->enabled)
                continue;

            Vec2 direction = body->position - center;
            float distance = direction.length();
            // std::cout << "Body " << body->id << " - Distance: " << distance << " Radius: " << radius << std::endl;

            // Só afeta corpos dentro do raio
            if (distance > radius)
                continue;

            // Prevenir divisão por zero e normalizar
            if (distance < EPSILON)
            {
                // Se estiver exatamente no centro, empurrar em direção aleatória
                float randomAngle = (rand() % 360) * (3.14159f / 180.0f);
                direction = Vec2(std::cos(randomAngle), std::sin(randomAngle));
                distance = 1.0f;
            }
            else
            {
                direction = direction / distance; // Normalizar manualmente
            }

            // Falloff quadrático inverso (mais realista)
            float normalizedDist = distance / radius; // 0.0 (centro) a 1.0 (borda)
            float falloff = 1.0f - normalizedDist;    // 1.0 (centro) a 0.0 (borda)
            falloff = falloff * falloff;              // Quadrático

            // Aplicar impulso direto na velocidade (explosão instantânea)
            float impulseMagnitude = force * falloff * body->inverseMass;
            Vec2 impulse = direction * impulseMagnitude;

            body->velocity += impulse;

            // Adicionar torque aleatório para efeito visual
            float randomTorque = ((rand() % 200) - 100) * 0.01f * impulseMagnitude;
            body->angularVelocity += randomTorque * body->inverseInertia;
        }
    }

    // ===== World - Estatísticas =====

    float World::getTotalKineticEnergy() const
    {
        float total = 0.0f;
        for (const Body *body : bodies)
        {
            if (body)
            {
                total += body->getKineticEnergy();
            }
        }
        return total;
    }

    Vec2 World::getCenterOfMass() const
    {
        Vec2 com(0, 0);
        float totalMass = 0.0f;

        for (const Body *body : bodies)
        {
            if (body && body->mass > 0.0f)
            {
                com += body->position * body->mass;
                totalMass += body->mass;
            }
        }

        if (totalMass > 0.0f)
        {
            com = com / totalMass;
        }

        return com;
    }

    // ===== World - Configurações =====

    void World::setCollisionIterations(int iterations)
    {
        collisionIterations = std::max(1, iterations);
    }

    void World::setPositionCorrectionPercent(float percent)
    {
        positionCorrectionPercent = std::max(0.0f, std::min(1.0f, percent));
    }

    void World::setPenetrationAllowance(float allowance)
    {
        penetrationAllowance = std::max(0.0f, allowance);
    }
    void World::solveCollision(Manifold &m)
    {
        if (m.bodyA->shape.type == ShapeType::Circle && m.bodyB->shape.type == ShapeType::Circle)
        {
            solveCircleToCircle(m);
        }
        else if (m.bodyA->shape.type == ShapeType::Polygon && m.bodyB->shape.type == ShapeType::Polygon)
        {
            solvePolygonToPolygon(m);
        }
        else
        {
            if (m.bodyA->shape.type == ShapeType::Circle)
            {
                solveCircleToPolygon(m);
            }
            else
            {
                solvePolygonToCircle(m);
            }
        }
    }

    void World::solveCircleToCircle(Manifold &m)
    {
        Vec2 normal = m.bodyB->position - m.bodyA->position;
        float distSq = normal.lengthSq();
        float radius = m.bodyA->shape.radius + m.bodyB->shape.radius;

        if (distSq >= radius * radius)
        {
            m.contactsCount = 0;
            return;
        }

        float dist = std::sqrt(distSq);
        m.contactsCount = 1;

        if (dist < EPSILON)
        {
            m.penetration = m.bodyA->shape.radius;
            m.normal = Vec2(1, 0);
            m.contacts[0] = m.bodyA->position;
        }
        else
        {
            m.penetration = radius - dist;
            m.normal = normal / dist;
            m.contacts[0] = m.bodyA->position + m.normal * m.bodyA->shape.radius;
        }

        if (!m.bodyA->isGrounded)
        {
            m.bodyA->isGrounded = (m.normal.y < 0);
        }
    }

    void World::solveCircleToPolygon(Manifold &m)
    {
        solveDifferentShapes(m, m.bodyA, m.bodyB);
    }

    void World::solvePolygonToCircle(Manifold &m)
    {
        solveDifferentShapes(m, m.bodyB, m.bodyA);
        m.normal.x *= -1.0f;
        m.normal.y *= -1.0f;
    }

    void World::solveDifferentShapes(Manifold &m, Body *circle, Body *polygon)
    {
        m.contactsCount = 0;

        Vec2 center = circle->position;
        center = polygon->shape.transform.transpose().multiply(center - polygon->position);

        float separation = -FLT_MAX;
        int faceNormal = 0;
        PolygonData &vertexData = polygon->shape.vertexData;

        for (int i = 0; i < vertexData.vertexCount; ++i)
        {
            float s = vertexData.normals[i].dot(center - vertexData.positions[i]);

            if (s > circle->shape.radius)
                return;

            if (s > separation)
            {
                separation = s;
                faceNormal = i;
            }
        }

        Vec2 v1 = vertexData.positions[faceNormal];
        int nextIndex = (faceNormal + 1) % vertexData.vertexCount;
        Vec2 v2 = vertexData.positions[nextIndex];

        if (separation < EPSILON)
        {
            m.contactsCount = 1;
            Vec2 normal = polygon->shape.transform.multiply(vertexData.normals[faceNormal]);
            m.normal = Vec2(-normal.x, -normal.y);
            m.contacts[0] = circle->position + m.normal * circle->shape.radius;
            m.penetration = circle->shape.radius;
            return;
        }

        float dot1 = (center - v1).dot(v2 - v1);
        float dot2 = (center - v2).dot(v1 - v2);
        m.penetration = circle->shape.radius - separation;

        if (dot1 <= 0.0f)
        {
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
        else if (dot2 <= 0.0f)
        {
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
        else
        {
            Vec2 normal = vertexData.normals[faceNormal];

            if ((center - v1).dot(normal) > circle->shape.radius)
                return;

            normal = polygon->shape.transform.multiply(normal);
            m.normal = Vec2(-normal.x, -normal.y);
            m.contacts[0] = circle->position + m.normal * circle->shape.radius;
            m.contactsCount = 1;
        }
    }

    void World::solvePolygonToPolygon(Manifold &m)
    {
        Shape &shapeA = m.bodyA->shape;
        Shape &shapeB = m.bodyB->shape;
        m.contactsCount = 0;

        int faceA = 0;
        float penetrationA = findAxisLeastPenetration(faceA, shapeA, shapeB);
        if (penetrationA >= 0.0f)
            return;

        int faceB = 0;
        float penetrationB = findAxisLeastPenetration(faceB, shapeB, shapeA);
        if (penetrationB >= 0.0f)
            return;

        int referenceIndex = 0;
        bool flip = false;

        Shape *refPoly;
        Shape *incPoly;

        const float k_relativeTol = 0.95f;
        const float k_absoluteTol = 0.01f;

        if (penetrationA >= penetrationB * k_relativeTol + penetrationA * k_absoluteTol)
        {
            refPoly = &shapeA;
            incPoly = &shapeB;
            referenceIndex = faceA;
            flip = false;
        }
        else
        {
            refPoly = &shapeB;
            incPoly = &shapeA;
            referenceIndex = faceB;
            flip = true;
        }

        Vec2 incidentFace[2];
        findIncidentFace(incidentFace[0], incidentFace[1], *refPoly, *incPoly, referenceIndex);

        PolygonData &refData = refPoly->vertexData;
        Vec2 v1 = refData.positions[referenceIndex];
        referenceIndex = (referenceIndex + 1) % refData.vertexCount;
        Vec2 v2 = refData.positions[referenceIndex];

        v1 = refPoly->transform.multiply(v1) + refPoly->body->position;
        v2 = refPoly->transform.multiply(v2) + refPoly->body->position;

        Vec2 sidePlaneNormal = (v2 - v1).normalized();

        Vec2 refFaceNormal(sidePlaneNormal.y, -sidePlaneNormal.x);
        float refC = refFaceNormal.dot(v1);
        float negSide = -sidePlaneNormal.dot(v1);
        float posSide = sidePlaneNormal.dot(v2);

        if (clip(Vec2(-sidePlaneNormal.x, -sidePlaneNormal.y), negSide, incidentFace[0], incidentFace[1]) < 2)
            return;

        if (clip(sidePlaneNormal, posSide, incidentFace[0], incidentFace[1]) < 2)
            return;

        m.normal = flip ? Vec2(-refFaceNormal.x, -refFaceNormal.y) : refFaceNormal;

        int currentPoint = 0;
        float separation = refFaceNormal.dot(incidentFace[0]) - refC;

        if (separation <= 0.0f)
        {
            m.contacts[currentPoint] = incidentFace[0];
            m.penetration = -separation;
            currentPoint++;
        }
        else
        {
            m.penetration = 0.0f;
        }

        separation = refFaceNormal.dot(incidentFace[1]) - refC;

        if (separation <= 0.0f)
        {
            m.contacts[currentPoint] = incidentFace[1];
            m.penetration += -separation;
            currentPoint++;

            if (currentPoint > 0)
                m.penetration /= currentPoint;
        }

        m.contactsCount = currentPoint;

        if (!m.bodyB->isGrounded && m.contactsCount > 0)
        {
            m.bodyB->isGrounded = (m.normal.y < 0);
        }
    }

    Vec2 World::getSupport(Shape &shape, const Vec2 &dir)
    {
        float bestProjection = -FLT_MAX;
        Vec2 bestVertex(0, 0);
        PolygonData &data = shape.vertexData;

        for (int i = 0; i < data.vertexCount; ++i)
        {
            Vec2 vertex = data.positions[i];
            float projection = vertex.dot(dir);

            if (projection > bestProjection)
            {
                bestVertex = vertex;
                bestProjection = projection;
            }
        }

        return bestVertex;
    }

    float World::findAxisLeastPenetration(int &faceIndex, Shape &shapeA, Shape &shapeB)
    {
        float bestDistance = -FLT_MAX;
        int bestIndex = 0;

        PolygonData &dataA = shapeA.vertexData;

        for (int i = 0; i < dataA.vertexCount; ++i)
        {
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

            if (distance > bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        faceIndex = bestIndex;
        return bestDistance;
    }

    void World::findIncidentFace(Vec2 &v0, Vec2 &v1, Shape &ref, Shape &inc, int index)
    {
        PolygonData &refData = ref.vertexData;
        PolygonData &incData = inc.vertexData;

        Vec2 referenceNormal = refData.normals[index];

        referenceNormal = ref.transform.multiply(referenceNormal);
        referenceNormal = inc.transform.transpose().multiply(referenceNormal);

        int incidentFace = 0;
        float minDot = FLT_MAX;

        for (int i = 0; i < incData.vertexCount; ++i)
        {
            float dot = referenceNormal.dot(incData.normals[i]);

            if (dot < minDot)
            {
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
    int World::clip(const Vec2 &normal, float c, Vec2 &faceA, Vec2 &faceB)
    {
        int sp = 0;
        Vec2 out[2] = {faceA, faceB};

        float distanceA = normal.dot(faceA) - c;
        float distanceB = normal.dot(faceB) - c;

        if (distanceA <= 0.0f)
            out[sp++] = faceA;
        if (distanceB <= 0.0f)
            out[sp++] = faceB;

        if (distanceA * distanceB < 0.0f)
        {
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

    void World::integrateForces(Body *body)
    {
        if (body->inverseMass == 0 || !body->enabled)
            return;

        body->velocity += (body->force * body->inverseMass) * (deltaTime / 2.0f);

        if (body->useGravity)
        {
            body->velocity += gravity * (deltaTime / 2.0f);
        }

        if (!body->freezeOrient)
        {
            body->angularVelocity += body->torque * body->inverseInertia * (deltaTime / 2.0f);
        }
    }

    void World::integrateVelocity(Body *body)
    {
        if (!body->enabled)
            return;

        body->position += body->velocity * deltaTime;

        if (!body->freezeOrient)
        {
            body->orient += body->angularVelocity * deltaTime;
        }

        body->shape.transform.set(body->orient);
        integrateForces(body);
    }

    void World::initializeManifold(Manifold &m)
    {
        m.restitution = std::sqrt(m.bodyA->restitution * m.bodyB->restitution);
        m.staticFriction = std::sqrt(m.bodyA->staticFriction * m.bodyB->staticFriction);
        m.dynamicFriction = std::sqrt(m.bodyA->dynamicFriction * m.bodyB->dynamicFriction);

        for (int i = 0; i < m.contactsCount; ++i)
        {
            Vec2 ra = m.contacts[i] - m.bodyA->position;
            Vec2 rb = m.contacts[i] - m.bodyB->position;

            Vec2 rv = m.bodyB->velocity + Vec2::cross(m.bodyB->angularVelocity, rb) -
                      m.bodyA->velocity - Vec2::cross(m.bodyA->angularVelocity, ra);

            if (rv.lengthSq() < (gravity * deltaTime).lengthSq() + EPSILON)
            {
                m.restitution = 0.0f;
            }
        }
    }

    void World::applyImpulse(Manifold &m)
    {
        Body *a = m.bodyA;
        Body *b = m.bodyB;

        if (std::abs(a->inverseMass + b->inverseMass) <= EPSILON)
        {
            a->velocity = Vec2(0, 0);
            b->velocity = Vec2(0, 0);
            return;
        }

        for (int i = 0; i < m.contactsCount; ++i)
        {
            Vec2 ra = m.contacts[i] - a->position;
            Vec2 rb = m.contacts[i] - b->position;

            Vec2 rv = b->velocity + Vec2::cross(b->angularVelocity, rb) -
                      a->velocity - Vec2::cross(a->angularVelocity, ra);

            float contactVel = rv.dot(m.normal);
            if (contactVel > 0)
                return;

            float raCrossN = ra.cross(m.normal);
            float rbCrossN = rb.cross(m.normal);

            float invMassSum = a->inverseMass + b->inverseMass +
                               (raCrossN * raCrossN) * a->inverseInertia +
                               (rbCrossN * rbCrossN) * b->inverseInertia;

            float j = -(1.0f + m.restitution) * contactVel;
            j /= invMassSum;
            j /= m.contactsCount;

            Vec2 impulse = m.normal * j;

            if (a->enabled)
            {
                a->velocity -= impulse * a->inverseMass;
                if (!a->freezeOrient)
                {
                    a->angularVelocity -= a->inverseInertia * ra.cross(impulse);
                }
            }

            if (b->enabled)
            {
                b->velocity += impulse * b->inverseMass;
                if (!b->freezeOrient)
                {
                    b->angularVelocity += b->inverseInertia * rb.cross(impulse);
                }
            }

            rv = b->velocity + Vec2::cross(b->angularVelocity, rb) -
                 a->velocity - Vec2::cross(a->angularVelocity, ra);

            Vec2 tangent = rv - m.normal * rv.dot(m.normal);
            tangent.normalize();

            float jt = -rv.dot(tangent);
            jt /= invMassSum;
            jt /= m.contactsCount;

            if (std::abs(jt) < EPSILON)
                continue;

            Vec2 tangentImpulse;
            if (std::abs(jt) < j * m.staticFriction)
            {
                tangentImpulse = tangent * jt;
            }
            else
            {
                tangentImpulse = tangent * (-j * m.dynamicFriction);
            }

            if (a->enabled)
            {
                a->velocity -= tangentImpulse * a->inverseMass;
                if (!a->freezeOrient)
                {
                    a->angularVelocity -= a->inverseInertia * ra.cross(tangentImpulse);
                }
            }

            if (b->enabled)
            {
                b->velocity += tangentImpulse * b->inverseMass;
                if (!b->freezeOrient)
                {
                    b->angularVelocity += b->inverseInertia * rb.cross(tangentImpulse);
                }
            }
        }
    }

    PolygonData World::createRectanglePolygon(const Vec2 &pos, const Vec2 &size)
    {
        PolygonData data;
        data.vertexCount = 4;

        float halfW = size.x / 2.0f;
        float halfH = size.y / 2.0f;

        data.positions[0] = Vec2(halfW, -halfH);
        data.positions[1] = Vec2(halfW, halfH);
        data.positions[2] = Vec2(-halfW, halfH);
        data.positions[3] = Vec2(-halfW, -halfH);

        calculateNormals(data);
        return data;
    }
    PolygonData World::createRegularPolygon(float radius, int sides)
    {
        PolygonData data;
        data.vertexCount = sides;

        for (int i = 0; i < sides; ++i)
        {
            float angle = 360.0f / sides * i * DEG2RAD;
            data.positions[i] = Vec2(
                std::cos(angle) * radius,
                std::sin(angle) * radius);
        }

        calculateNormals(data);
        return data;
    }

    void World::calculateNormals(PolygonData &data)
    {
        for (int i = 0; i < data.vertexCount; ++i)
        {
            int next = (i + 1) % data.vertexCount;
            Vec2 face = data.positions[next] - data.positions[i];
            data.normals[i] = Vec2(face.y, -face.x).normalized();
        }
    }
    void World::correctPositions(Manifold &m)
    {
        Body *a = m.bodyA;
        Body *b = m.bodyB;

        float correction = std::max(m.penetration - PENETRATION_ALLOWANCE, 0.0f) /
                           (a->inverseMass + b->inverseMass) * PENETRATION_CORRECTION;

        Vec2 correctionVec = m.normal * correction;

        if (a->enabled)
        {
            a->position -= correctionVec * a->inverseMass;
        }

        if (b->enabled)
        {
            b->position += correctionVec * b->inverseMass;
        }
    }

} // namespace Physics2D