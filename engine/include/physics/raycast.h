#pragma once

#include "include/physics/types.h"
#include "include/scene/object.h"

#include <glm/glm.hpp>

#include <vector>
#include <memory>


struct BVHnode
{
    AABB aabb;
    std::unique_ptr<BVHnode> left;
    std::unique_ptr<BVHnode> right;
    std::vector<Tri*> tris;
    bool isLeaf = false;
};

namespace Auric::Physics
{

bool rayOccluded(const glm::vec3& origin, const glm::vec3& dir,
                        float maxDist, const BVHnode& node);

bool rayOccluded(const glm::vec3 &origin, const glm::vec3 &destination, const BVHnode& node);

float rayHitCapsule(const glm::vec3& origin, const glm::vec3& dir, float maxDist,
                   CapsuleColliderComponent* cap);

void splitBVHnode(BVHnode* node);

}
