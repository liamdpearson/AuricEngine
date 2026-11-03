#pragma once

#include "raycast.h"
#include "include/scene/object.h"


extern std::vector<TriAABB> colliders;
extern BVHnode rootNode;

void resolveCapsuleCollisions(CapsuleColliderComponent* cap, float deltaTime);

void collectSceneColliders();
