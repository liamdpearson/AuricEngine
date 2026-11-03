#pragma once

#include <glm/glm.hpp> // vec3, mat4, basic types
#include <xatlas/xatlas.h>

#include <vector>


// loops through mesh tris and adds up area
float getMeshArea(const std::vector<float>& verts,
                         const std::vector<unsigned int>& idx);

bool GenUV2(xatlas::Atlas*& atlas, std::vector<float>& verts,
                   std::vector<unsigned int>& idx, float resolution);
