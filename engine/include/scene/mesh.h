#pragma once

#include <glm/glm.hpp> // vec3, mat4, basic types

#include <vector>

#include "include/scene/animtypes.h"


// loads .obj file
void loadObj(const char* path, std::vector<float>& outVerts,
             std::vector<unsigned int>& outIndices);

// loads .fbx file
void loadFbx(const char* path, std::vector<float>& outVerts,
             std::vector<unsigned int>& outIndices,
             Skeleton& outSkel,
             std::vector<Animation>& outAnims);

// loads a rig from a .fbx file
void loadFbxRig(const char* path,
                       Skeleton& outSkel,
                       std::vector<Animation>& outAnims);
