#include "include/physics/raycast.h"
#include "include/scene/object.h"

#include <glm/geometric.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <algorithm>
#include <memory>


namespace Auric::Physics
{

static float rayHitTri(const glm::vec3& origin, const glm::vec3& dir,
                      float maxDist, const Tri& t)
{
    const glm::vec3 e1 = t.b - t.a;
    const glm::vec3 e2 = t.c - t.a;
    const glm::vec3 h = glm::cross(dir, e2);
    const float a = glm::dot(e1, h);
    if (a > -0.0001f && a < 0.0001f) return -1; // parallel
    const float f = 1 / a;
    const glm::vec3 s = origin - t.a;
    const float u = f * glm::dot(s, h);
    if (u < 0 || u > 1) return -1;
    const glm::vec3 q = glm::cross(s, e1);
    const float v = f * glm::dot(dir, q);
    if (v < 0 || u + v > 1) return -1;
    const float dist = f * glm::dot(e2, q);
    if (dist > 0.0001 && dist < maxDist) return dist;
    return -1;
}

static bool rayHitAABB(const glm::vec3& origin, const glm::vec3& dir,
                       float maxDist, const AABB& aabb)
{  
    float t1x;
    float t2x;

    // make sure no div by 0
    if (dir.x == 0) {
        // if x dir is 0 and origin.x isnt inside the aabb's x bounds it wont hit
        if (!(origin.x >= aabb.min.x && origin.x <= aabb.max.x)) return false;
        t1x = -INFINITY;
        t2x = INFINITY;
    } else {
        t1x = (aabb.min.x - origin.x) / dir.x;
        t2x = (aabb.max.x - origin.x) / dir.x;
        if (t1x > t2x) std::swap(t1x, t2x);
    }
    
    float t1y;
    float t2y;

    // make sure no div by 0
    if (dir.y == 0) {
        // if y dir is 0 and origin.y isnt inside the aabb's y bounds it wont hit
        if (!(origin.y >= aabb.min.y && origin.y <= aabb.max.y)) return false;
        t1y = -INFINITY;
        t2y = INFINITY;
    } else {
        t1y = (aabb.min.y - origin.y) / dir.y;
        t2y = (aabb.max.y - origin.y) / dir.y;
        if (t1y > t2y) std::swap(t1y, t2y);
    }

    float t1z;
    float t2z;

    // make sure no div by 0
    if (dir.z == 0) {
        // if z dir is 0 and origin.z isnt inside the aabb's z bounds it wont hit
        if (!(origin.z >= aabb.min.z && origin.z <= aabb.max.z)) return false;
        t1z = -INFINITY;
        t2z = INFINITY;
    } else {
        t1z = (aabb.min.z - origin.z) / dir.z;
        t2z = (aabb.max.z - origin.z) / dir.z;
        if (t1z > t2z) std::swap(t1z, t2z);
    }
    float t1Max = std::max({t1x, t1y, t1z});
    float t2Min = std::min({t2x, t2y, t2z});

    return (t1Max <= t2Min) &&
            t2Min >= 0.0f &&
            t1Max <= maxDist;
}

// CLAUDE GENERATED FUNCTION
// ray vs capsule (segment pa->pb, radius r). rd must be normalized. returns t or -1
static float rayCapsule(glm::vec3 ro, glm::vec3 rd, glm::vec3 pa, glm::vec3 pb, float r)
{
    glm::vec3 ba = pb - pa, oa = ro - pa;
    float baba = glm::dot(ba, ba), bard = glm::dot(ba, rd), baoa = glm::dot(ba, oa);
    float rdoa = glm::dot(rd, oa), oaoa = glm::dot(oa, oa);
    float a = baba - bard * bard;
    float b = baba * rdoa - baoa * bard;
    float c = baba * oaoa - baoa * baoa - r * r * baba;
    float h = b * b - a * c;
    if (h >= 0.0f && a > 1e-6f) {
        float t = (-b - std::sqrt(h)) / a;
        float y = baoa + t * bard;
        if (y > 0.0f && y < baba) return t;              // hit the body
        glm::vec3 oc = (y <= 0.0f) ? oa : ro - pb;       // hit an end cap
        b = glm::dot(rd, oc);
        c = glm::dot(oc, oc) - r * r;
        h = b * b - c;
        if (h > 0.0f) return -b - std::sqrt(h);
    }
    return -1.0f;
}

float rayHitCapsule(const glm::vec3& origin, const glm::vec3& dir, float maxDist,
                   CapsuleColliderComponent* cap)
{
    const Transform& t = cap->owner->transform;
    glm::vec3 feet{t.x, t.y, t.z};
    glm::vec3 a = feet + glm::vec3{0, cap->radius, 0};
    glm::vec3 b = feet + glm::vec3{0, cap->height - cap->radius, 0};
    float d = rayCapsule(origin, glm::normalize(dir), a, b, cap->radius);
    if (d > 0.0f && d < maxDist) return d;
    return -1;
}

bool rayOccluded(const glm::vec3& origin, const glm::vec3& dir,
                        float maxDist, const BVHnode& node)
{
    if (!rayHitAABB(origin, dir, maxDist, node.aabb)) return false;
    if (node.isLeaf) {
        for (Tri* t : node.tris) {
            if (rayHitTri(origin, dir, maxDist, *t) > 0.0f) return true;
        }
    } else {
        return rayOccluded(origin, dir, maxDist, *(node.left.get())) ||
               rayOccluded(origin, dir, maxDist, *(node.right.get()));
    }
    return false;
}

bool rayOccluded(const glm::vec3 &origin, const glm::vec3 &destination, const BVHnode& node)
{
    glm::vec3 dir = origin - destination;
    float maxDist = glm::length(dir);
    dir = glm::normalize(dir); 
    return rayOccluded(origin, dir, maxDist, node);
}

// old raycast method for reference
// bool rayOccluded(const glm::vec3& origin, const glm::vec3& dir,
//                         float maxDist, const BVHnode& node)
// {
//     for (Tri* t : node.tris)
//     {
//         if (rayHitTri(origin, dir, maxDist, *t) > 0.0f) return true;
//     }

//     return false;

// }


static glm::vec3 centroid(const Tri& tri)
{
    return (tri.a + tri.b + tri.c) / 3.0f;
}

void splitBVHnode(BVHnode* node)
{
    if (node->tris.size() < 5) {
        node->isLeaf = true;
        return;
    }

    float xDiff = node->aabb.max.x - node->aabb.min.x;
    float yDiff = node->aabb.max.y - node->aabb.min.y;
    float zDiff = node->aabb.max.z - node->aabb.min.z;

    int axis = 0;
    if (yDiff > xDiff)
        axis = 1;
    if (zDiff > yDiff)
        axis = 2;

    size_t mid = node->tris.size() / 2;

    std::nth_element(
        node->tris.begin(),
        node->tris.begin() + mid,
        node->tris.end(),
        [axis](const Tri* a, const Tri* b) {
            return centroid(*a)[axis] < centroid(*b)[axis];
        }
    );

    std::vector<Tri*> leftTris(node->tris.begin(), node->tris.begin() + mid);
    AABB leftAABB{glm::vec3{INFINITY}, glm::vec3{-INFINITY}};

    for (Tri* t : leftTris)
    {
        leftAABB.min = glm::min(t->a, leftAABB.min);
        leftAABB.min = glm::min(t->b, leftAABB.min);
        leftAABB.min = glm::min(t->c, leftAABB.min);

        leftAABB.max = glm::max(t->a, leftAABB.max);
        leftAABB.max = glm::max(t->b, leftAABB.max);
        leftAABB.max = glm::max(t->c, leftAABB.max);
    }

    auto left = std::make_unique<BVHnode>(BVHnode{
        leftAABB, nullptr, nullptr, leftTris, false
    });
    node->left = std::move(left);

    std::vector<Tri*> rightTris(node->tris.begin() + mid, node->tris.end());
    AABB rightAABB{glm::vec3{INFINITY}, glm::vec3{-INFINITY}};

    for (Tri* t : rightTris)
    {
        rightAABB.min = glm::min(t->a, rightAABB.min);
        rightAABB.min = glm::min(t->b, rightAABB.min);
        rightAABB.min = glm::min(t->c, rightAABB.min);

        rightAABB.max = glm::max(t->a, rightAABB.max);
        rightAABB.max = glm::max(t->b, rightAABB.max);
        rightAABB.max = glm::max(t->c, rightAABB.max);
    }

    auto right = std::make_unique<BVHnode>(BVHnode{
        rightAABB, nullptr, nullptr, rightTris, false
    });
    node->right = std::move(right);

    splitBVHnode(node->left.get());
    splitBVHnode(node->right.get());
}

}
