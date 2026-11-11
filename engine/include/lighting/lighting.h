#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <utility>
#include <string>

#include "include/physics/phystypes.h"
#include "include/physics/raycast.h"


// scene light used only in lighting bake
struct Light
{
    std::string name;
    std::string tag;
    glm::vec3 color;
    float intensity;
    float radius;
    float falloff;

    virtual ~Light() = default;
};

struct PointLight : Light
{
    glm::vec3 pos;
};

struct LineLight : Light 
{
    glm::vec3 start;
    glm::vec3 end;
    int segments;
};

struct LightGrid
{
    std::vector<std::pair<glm::vec3, glm::vec3>> values;
    glm::vec3 min{INFINITY}, max{-INFINITY};
};

namespace Auric::Lighting
{

extern std::vector<Light*> lights;
extern float ambient;
extern std::vector<Tri> occluders;
extern Auric::Physics::BVHnode rootNode;
extern LightGrid lightGrid;

glm::vec3 sampleLightAt(const glm::vec3& p);

std::pair<glm::vec3, glm::vec3> gridLightAt(const glm::vec3& p);

void bakeSceneLighting();

}
