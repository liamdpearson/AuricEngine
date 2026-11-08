#pragma once

#include <glm/glm.hpp> // vec3, mat4, basic types
#include <xatlas/xatlas.h>

#include "include/scene/object.h"


namespace Auric::Lighting
{

extern float lightmapResScalar;

void initializeLightmap(LightMapComponent* lm, const MeshComponent* mesh);

}

