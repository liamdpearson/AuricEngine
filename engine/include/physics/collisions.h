#pragma once

#include "include/physics/raycast.h"
#include "include/scene/object.h"


namespace Auric::Physics
{

extern std::vector<TriAABB> colliders;
extern BVHnode rootNode;

void resolveCapsuleCollisions(CapsuleColliderComponent* cap, float deltaTime);

void collectSceneColliders();

}
