#include "/include/core/lighting.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>         // value_ptr (hand a matrix to OpenGL)
#include <xatlas/xatlas.h>

#include <vector>
#include <iostream>
#include <algorithm>
#include <utility>


std::vector<Light*> lights;
float ambient = 1.0f; // default value overriden in loadScene
float lightmapResScalar = 0.01f; // default value overriden in loadScene
std::vector<Tri> occluders;
BVHnode rootNode;
LightGrid lightGrid;


glm::vec3 sampleLightAt(const glm::vec3& p)
{
    glm::vec3 lit(ambient);

    for (const Light* l: lights)
    {
        // point lights
        if (const PointLight* light = dynamic_cast<const PointLight*>(l)) {
            glm::vec3 toLight = light->pos - p;
            float dist = glm::length(toLight);
            if (dist < 0.00001f || dist > light->radius) continue;
            glm::vec3 dir = glm::normalize(toLight);

            if (rayOccluded(p, dir, dist, rootNode)) continue;
            
            float a = 1/(1+(dist/light->falloff)*(dist/light->falloff));
            float b = 1/(1+(light->radius/light->falloff)*(light->radius/light->falloff));
            float denom = 1 - b;

            float atten = light->intensity * (a - b) / denom;
            atten = std::max(atten, 0.0f); // this line kinda useless(line 34) but better safe than sorry
            lit += light->color * atten;
        }
        else if (const LineLight* light = dynamic_cast<const LineLight*>(l)) {
            const glm::vec3 startToEnd= light->end - light->start;

            for (int i = 0; i <= light->segments; i++) {
                
                const glm::vec3 pos = light->start + ((float)i/(float)light->segments) * startToEnd;
                glm::vec3 toLight = pos - p;
                float dist = glm::length(toLight);
                if (dist > light->radius) continue;
                glm::vec3 dir = glm::normalize(toLight);

                if (rayOccluded(p, dir, dist, rootNode)) continue;
                
                float a = 1/(1+(dist/light->falloff)*(dist/light->falloff));
                float b = 1/(1+(light->radius/light->falloff)*(light->radius/light->falloff));
                float denom = 1 - b;

                float atten = light->intensity/(float)light->segments * (a - b) / denom;
                atten = std::max(atten, 0.0f); // this line kinda useless(line 34) but better safe than sorry
                lit += light->color * atten;
            }
        }
        
    }
    // if r, g, or b is over 1.0f then divide them all by the largest one. this
    // will force the values between 0 and 1 while still preserving the hue
    float peak = glm::max(lit.r, glm::max(lit.g, lit.b));
    if (peak > 1.0f) lit /= peak;

    return lit;
}

std::pair<glm::vec3, glm::vec3> sampleLightAndDir(const glm::vec3& p)
{
    glm::vec3 lit(ambient);
    glm::vec3 dir(0.0f);

    for (const Light* l: lights)
    {
        if (const PointLight* light = dynamic_cast<const PointLight*>(l)) {
            glm::vec3 toLight = light->pos - p;
            float dist = glm::length(toLight);
            if (dist < 0.00001f || dist > light->radius) continue;
            glm::vec3 d = glm::normalize(toLight);

            if (rayOccluded(p, d, dist, rootNode)) continue;
            
            float a = 1/(1+(dist/light->falloff)*(dist/light->falloff));
            float b = 1/(1+(light->radius/light->falloff)*(light->radius/light->falloff));
            float denom = 1 - b;

            float atten = light->intensity * (a - b) / denom;
            atten = std::max(atten, 0.0f); // this line kinda useless but better safe than sorry
            lit += light->color * atten;

            dir += d * atten;
        }
        else if (const LineLight* light = dynamic_cast<const LineLight*>(l)) {
            const glm::vec3 startToEnd= light->end - light->start;

            for (int i = 0; i <= light->segments; i++) {
                const glm::vec3 pos = light->start + ((float)i/(float)light->segments) * startToEnd;
                glm::vec3 toLight = pos - p;
                float dist = glm::length(toLight);
                if (dist < 0.00001f || dist > light->radius) continue;
                glm::vec3 d = glm::normalize(toLight);

                if (rayOccluded(p, d, dist, rootNode)) continue;
                
                float a = 1/(1+(dist/light->falloff)*(dist/light->falloff));
                float b = 1/(1+(light->radius/light->falloff)*(light->radius/light->falloff));
                float denom = 1 - b;

                float atten = light->intensity/(float)light->segments * (a - b) / denom;
                atten = std::max(atten, 0.0f); // this line kinda useless but better safe than sorry
                lit += light->color * atten;

                dir += d * atten;
            }
        }
    }
    // if r, g, or b is over 1.0f then divide them all by the largest one. this
    // will force the values between 0 and 1 while still preserving the hue
    float peak = glm::max(lit.r, glm::max(lit.g, lit.b));
    if (peak > 1.0f) lit /= peak;

    // grid points that no light reached have nothing to normalise, and
    // normalising a zero vector would hand back NaNs.
    if (glm::dot(dir, dir) > 0.0f) dir = glm::normalize(dir);
    std::pair<glm::vec3, glm::vec3> pair{lit, dir};

    return pair;
}

// CLAUDE GENERATED FUNCTION
// flattens a grid coordinate the same way bakeSceneLighting stored it,
// walking x on the outside and z on the inside.
static size_t gridIndex(int x, int y, int z, int ny, int nz)
{
    return ((size_t)x * ny + y) * nz + z;
}

// CLAUDE GENERATED FUNCTION
// takes in a point in space and returns the trilinearly interpolated
// color and dominant light direction of that point based on the 8 known
// values in the light grid that make up the cube around the point.
std::pair<glm::vec3, glm::vec3> gridLightAt(const glm::vec3& p)
{
    const std::vector<std::pair<glm::vec3, glm::vec3>>& values = lightGrid.values;
    const glm::vec3& min = lightGrid.min; const glm::vec3& max = lightGrid.max;

    if (values.empty()) {
        return std::pair<glm::vec3, glm::vec3>{glm::vec3{ambient}, glm::vec3{0.0f}};
    }

    // the grid holds one sample per unit, from min to max inclusive.
    int nx = (int)max.x - (int)min.x + 1;
    int ny = (int)max.y - (int)min.y + 1;
    int nz = (int)max.z - (int)min.z + 1;

    // nothing baked yet (or a grid that doesn't match the bounds box), so
    // fall back to flat ambient with no direction.
    if (nx < 1 || ny < 1 || nz < 1 || values.size() != (size_t)nx * ny * nz)
        return std::pair<glm::vec3, glm::vec3>{glm::vec3(ambient), glm::vec3(0.0f)};

    // position relative to the corner of the bounds box, clamped inside it
    // so points outside the scene just use the nearest samples.
    float fx = glm::clamp(p.x - min.x, 0.0f, (float)(nx - 1));
    float fy = glm::clamp(p.y - min.y, 0.0f, (float)(ny - 1));
    float fz = glm::clamp(p.z - min.z, 0.0f, (float)(nz - 1));

    // low corner of the cube around p. held one short of the last sample so
    // the high corner is always in range, and both collapse if an axis only
    // has a single sample.
    int x0 = std::min((int)fx, std::max(nx - 2, 0));
    int y0 = std::min((int)fy, std::max(ny - 2, 0));
    int z0 = std::min((int)fz, std::max(nz - 2, 0));
    int x1 = std::min(x0 + 1, nx - 1);
    int y1 = std::min(y0 + 1, ny - 1);
    int z1 = std::min(z0 + 1, nz - 1);

    // how far p sits between the two corners on each axis.
    float tx = fx - (float)x0;
    float ty = fy - (float)y0;
    float tz = fz - (float)z0;

    size_t i000 = gridIndex(x0, y0, z0, ny, nz);
    size_t i100 = gridIndex(x1, y0, z0, ny, nz);
    size_t i010 = gridIndex(x0, y1, z0, ny, nz);
    size_t i110 = gridIndex(x1, y1, z0, ny, nz);
    size_t i001 = gridIndex(x0, y0, z1, ny, nz);
    size_t i101 = gridIndex(x1, y0, z1, ny, nz);
    size_t i011 = gridIndex(x0, y1, z1, ny, nz);
    size_t i111 = gridIndex(x1, y1, z1, ny, nz);

    // blend the corner colors along x, then y, then z.
    glm::vec3 lit00 = glm::mix(values[i000].first, values[i100].first, tx);
    glm::vec3 lit10 = glm::mix(values[i010].first, values[i110].first, tx);
    glm::vec3 lit01 = glm::mix(values[i001].first, values[i101].first, tx);
    glm::vec3 lit11 = glm::mix(values[i011].first, values[i111].first, tx);
    glm::vec3 lit = glm::mix(glm::mix(lit00, lit10, ty), glm::mix(lit01, lit11, ty), tz);

    // same blend for the directions. each corner points at whatever lit it
    // most, so the weighted average of the 8 is the dominant direction here.
    glm::vec3 dir00 = glm::mix(values[i000].second, values[i100].second, tx);
    glm::vec3 dir10 = glm::mix(values[i010].second, values[i110].second, tx);
    glm::vec3 dir01 = glm::mix(values[i001].second, values[i101].second, tx);
    glm::vec3 dir11 = glm::mix(values[i011].second, values[i111].second, tx);
    glm::vec3 dir = glm::mix(glm::mix(dir00, dir10, ty), glm::mix(dir01, dir11, ty), tz);

    // averaging unit vectors doesn't give a unit vector, and corners that
    // cancel out or were never lit leave nothing to normalise.
    if (glm::dot(dir, dir) > 0.0f) dir = glm::normalize(dir);

    // std::cout << dir.x << ' ' << dir.y << ' ' << dir.z << '\n';
    return std::pair<glm::vec3, glm::vec3>{lit, dir};
}


// original bake lighting call
void bakeSceneLighting()
{
    double start = glfwGetTime();

    occluders.clear();
    lightGrid = LightGrid{};

    // collect light occluder triangles
    for (std::unique_ptr<Object>& obj : rootObjs)
        obj->CollectOccluders(glm::mat4(1.0f), occluders);
    
    std::cout << "baked " << occluders.size() << " occluder Tris"  << '\n';

    if (occluders.size() == 0) return;

    // set light grid dimentions
    glm::vec3& min = lightGrid.min;
    glm::vec3& max = lightGrid.max;

    for (const Tri& tri : occluders)
    {
        std::vector<glm::vec3> verts{tri.a, tri.b, tri.c};
        for (const glm::vec3& v : verts)
        {
            min.x = std::min(min.x, v.x); max.x = std::max(max.x, v.x);
            min.y = std::min(min.y, v.y); max.y = std::max(max.y, v.y);
            min.z = std::min(min.z, v.z); max.z = std::max(max.z, v.z);
        }
    }

    min.x = floor(min.x) - 1; min.y = floor(min.y) - 1; min.z = floor(min.z) - 1;
    max.x = floor(max.x) + 2; max.y = floor(max.y) + 2; max.z = floor(max.z) + 2;

    // create the BVH nodes for fast raycasting
    std::vector<Tri*> occluderRefs;
    for (Tri& t : occluders)
        occluderRefs.push_back(&t);

    rootNode = BVHnode{
        AABB{lightGrid.min, lightGrid.max},
        nullptr, nullptr,
        occluderRefs,
        false
    };
    splitBVHnode(&rootNode);

    std::cout << "Light Grid Dimensions(-X X -Y Y -Z Z):\n";
    std::cout << min.x << ' ' << min.y << ' ' << min.z << ' ' 
              << max.x << ' ' << max.y << ' ' << max.z << '\n';
    
    // build light grid
    for (int x = min.x; x <= max.x; ++x)
    {
        for (int y = min.y; y <= max.y; ++y)
        {
            for (int z = min.z; z <= max.z; ++z)
            {
                std::pair<glm::vec3, glm::vec3> litAndDir = sampleLightAndDir(glm::vec3(x, y, z));
                lightGrid.values.push_back(litAndDir);
            }
        }
    }

    // bake lighting
    for (std::unique_ptr<Object>& obj : rootObjs)
        obj->BakeLighting(glm::mat4(1.0f));


    std::cout << "bake took " << (glfwGetTime() - start) << "s\n";
}
