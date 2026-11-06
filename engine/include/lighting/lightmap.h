#pragma once

#include <glm/glm.hpp> // vec3, mat4, basic types
#include <xatlas/xatlas.h>

#include <vector>

#include "include/scene/object.h"


extern float lightmapResScalar;

// loops through mesh tris and adds up area
float getMeshArea(const std::vector<float>& verts,
                         const std::vector<unsigned int>& idx);

void bakeLightmapLighting(LightMapComponent* lm, const MeshComponent* mesh);

void initializeLightmap(LightMapComponent* lm, const MeshComponent* mesh, Object* obj);

