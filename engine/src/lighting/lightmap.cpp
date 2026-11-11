#include "include/lighting/lightmap.h"

#include <xatlas/xatlas.h>

#include <vector>
#include <iostream>

#include "include/core/constants.h"
#include "include/lighting/lighting.h"
#include "include/scene/object.h"
#include "include/render/graphics.h"


namespace Auric::Lighting
{

float lightmapResScalar = 0.01f; // default value overriden in loadScene

// loops through mesh tris and adds up area
static float getMeshArea(const std::vector<float>& verts,
                         const std::vector<unsigned int>& idx)
{
    float area = 0.0f;

    for (size_t i = 0; i + 2 < idx.size(); i += 3)
    {
        glm::vec3 pos[3];
        for (int c = 0; c < 3; ++c)
        {
            size_t v = (size_t)idx[i + c] * Auric::Constants::VERTEX_FLOATS;
            pos[c] = glm::vec3(verts[v], verts[v + 1], verts[v + 2]);
        }
        area += 0.5f * glm::length(glm::cross(pos[2] - pos[0], pos[1] - pos[0]));
    }
    std::cout << "area: " << area << '\n';
    return area;
}

// uses xatlas to generate a second non overlapping 
// uv mapping for a static mesh's lightmap.
static bool GenUV2(xatlas::Atlas*& atlas, std::vector<float>& verts,
                   std::vector<unsigned int>& idx, float resolution)
{

    atlas = xatlas::Create();

    xatlas::MeshDecl meshDecl;
    meshDecl.vertexCount = static_cast<uint32_t>(verts.size() / Auric::Constants::VERTEX_FLOATS);
    meshDecl.vertexPositionData = verts.data();
    meshDecl.vertexPositionStride = sizeof(float) * Auric::Constants::VERTEX_FLOATS;

    meshDecl.indexCount = static_cast<uint32_t>(idx.size());
    meshDecl.indexData = idx.data();
    meshDecl.indexFormat = xatlas::IndexFormat::UInt32;


    xatlas::AddMeshError error = xatlas::AddMesh(atlas, meshDecl);
    if (error != xatlas::AddMeshError::Success)
    {
        std::cout << "Error adding mesh: " << xatlas::StringForEnum(error) << '\n';
        xatlas::Destroy(atlas);
        return false;
    }

    xatlas::ChartOptions chartOptions;
    xatlas::PackOptions packOptions;
    packOptions.resolution = resolution;
    packOptions.padding = 8;

    xatlas::Generate(atlas, chartOptions, packOptions);

    xatlas::Mesh& outputMesh = atlas->meshes[0];

    std::vector<float> finalVerts;
    std::vector<unsigned int> finalIdx;

    for (uint32_t i = 0; i < outputMesh.vertexCount; ++i)
    {
        const xatlas::Vertex& xatlasVert = outputMesh.vertexArray[i];

        const uint32_t ogVertIdx = xatlasVert.xref * Auric::Constants::VERTEX_FLOATS;
        //pos
        finalVerts.push_back(verts[ogVertIdx]);
        finalVerts.push_back(verts[ogVertIdx + 1]);
        finalVerts.push_back(verts[ogVertIdx + 2]);
        // uv1
        finalVerts.push_back(verts[ogVertIdx + 3]);
        finalVerts.push_back(verts[ogVertIdx + 4]);
        // norm
        finalVerts.push_back(verts[ogVertIdx + 5]);
        finalVerts.push_back(verts[ogVertIdx + 6]);
        finalVerts.push_back(verts[ogVertIdx + 7]);
        // uv2
        finalVerts.push_back(xatlasVert.uv[0] / static_cast<float>(atlas->width));
        finalVerts.push_back(xatlasVert.uv[1] / static_cast<float>(atlas->height));
        // anim data
        finalVerts.push_back(verts[ogVertIdx + 10]);
        finalVerts.push_back(verts[ogVertIdx + 11]);
        finalVerts.push_back(verts[ogVertIdx + 12]);
        finalVerts.push_back(verts[ogVertIdx + 13]);
        finalVerts.push_back(verts[ogVertIdx + 14]);
    }
    
    for (uint32_t i = 0; i < outputMesh.indexCount; ++i) {
        finalIdx.push_back(outputMesh.indexArray[i]);
    }

    verts = finalVerts;
    idx = finalIdx;

    std::cout << "Succesfully created second uv set. New vert count: "
              << finalVerts.size() / Auric::Constants::VERTEX_FLOATS << '\n'
              << "# of atlas islands: " << outputMesh.chartCount << '\n';

    return true;
}

static float cross2D(const glm::vec2& a, const glm::vec2& b)
{
    return a.x * b.y - a.y * b.x;
}

// calculates the light value for each texel in the static meshes
// light map. then it creates the light map texture and sets it as
// the static meshes light map. finally, it calls Object::BakeLighting.
static void bakeLightmapLighting(LightMapComponent* lm, MeshComponent* mesh)
{
    glm::mat4 world = lm->owner->getWorld();
    glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(world)));
    const std::vector<unsigned int>& indices = mesh->indices;
    const std::vector<float>& vertices = mesh->vertices;
    xatlas::Atlas* atlas = lm->atlas;
    const float wMargin = 1.0f / (float)atlas->width;
    const float wOffset = wMargin / 2.0f;
    const float hMargin = 1.0f / (float)atlas->height;
    const float hOffset = hMargin / 2.0f;

    std::vector<glm::vec3> pixels(atlas->width * atlas->height, glm::vec3(0.0f));
    std::vector<int> covMask(atlas->width * atlas->height, 0);

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        glm::vec3 pos[3];
        glm::vec3 norm[3];
        glm::vec2 uv[3];
        float uMin = 2.0f, uMax = -1.0f;
        float vMin = 2.0f, vMax = -1.0f;

        // gets triangle corners world pos, norm, and uv.
        // also creates uv triangle bounding box
        for (int c = 0; c < 3; ++c)
        {   
            size_t v = (size_t)indices[i + c] * Auric::Constants::VERTEX_FLOATS;
            pos[c]  = glm::vec3(vertices[v + 0], vertices[v + 1], vertices[v + 2]);
            norm[c] = glm::vec3(vertices[v + 5], vertices[v + 6], vertices[v + 7]);
            uv[c]   = glm::vec2(vertices[v + 8], vertices[v + 9]);

            uMin = std::min(uMin, uv[c].x);  uMax = std::max(uMax, uv[c].x);
            vMin = std::min(vMin, uv[c].y);  vMax = std::max(vMax, uv[c].y);
        }
        
        float area = cross2D(uv[1] - uv[0], uv[2] - uv[0]);
        if (fabsf(area) < 1e-8f) continue; // degenerate triangle

        // calculates world pos and world norm for each of triangles corners.
        glm::vec3 wp0 = glm::vec3(world * glm::vec4(pos[0], 1.0f));
        glm::vec3 wp1 = glm::vec3(world * glm::vec4(pos[1], 1.0f));
        glm::vec3 wp2 = glm::vec3(world * glm::vec4(pos[2], 1.0f));

        glm::vec3 wn0 = glm::normalize(normalMat * norm[0]);
        glm::vec3 wn1 = glm::normalize(normalMat * norm[1]);
        glm::vec3 wn2 = glm::normalize(normalMat * norm[2]);

        // finds the range of cols and rows inside bounding box
        int colMin = std::max(0, (int)std::floor(uMin * atlas->width  - 0.5f));
        int colMax = std::min((int)atlas->width  - 1, (int)std::ceil(uMax * atlas->width  - 0.5f));
        int rowMin = std::max(0, (int)std::floor(vMin * atlas->height - 0.5f));
        int rowMax = std::min((int)atlas->height - 1, (int)std::ceil(vMax * atlas->height - 0.5f));

        // loops through the texels inside the bounding box
        for (int row = rowMin; row <= rowMax; ++row)
        {
            for (int col = colMin; col <= colMax; ++col)
            {
                float texelu = col * wMargin + wOffset;
                float texelv = row * hMargin + hOffset;
                
                // calculates if texel is actually inside the triangle
                // and not just inside the bounding box.
                glm::vec2 p(texelu, texelv);
                float w0 = cross2D(uv[2] - uv[1], p - uv[1]) / area;
                float w1 = cross2D(uv[0] - uv[2], p - uv[2]) / area;
                float w2 = 1 - w0 - w1;

                if (w0 < -1e-5f || w1 < -1e-5f || w2 < -1e-5f)
                    continue;

                // uses black magic to get world pos and world norm for texel
                glm::vec3 worldPos = w0*wp0 + w1*wp1 + w2*wp2;
                glm::vec3 worldNorm = glm::normalize(w0*wn0 + w1*wn1 + w2*wn2);

                // calculate light for texel
                glm::vec3 lit(ambient);

                for (const Light* l: lights)
                {
                    if (const PointLight* light = dynamic_cast<const PointLight*>(l)) {
                        glm::vec3 toLight = light->pos - worldPos;
                        float dist = glm::length(toLight);
                        if (dist < 0.0001f || dist > light->radius) continue;

                        glm::vec3 dir = toLight / dist;
                        float lambert = glm::max(glm::dot(worldNorm, dir), 0.0f);
                        if (lambert <= 0) continue;

                        glm::vec3 origin = worldPos + worldNorm * 1e-3f; // shadow bias
                        if (rayOccluded(origin, dir, dist, rootNode)) continue;
                        
                        float a = 1/(1+(dist/light->falloff)*(dist/light->falloff));
                        float b = 1/(1+(light->radius/light->falloff)*(light->radius/light->falloff));
                        float denom = 1 - b;

                        float atten = light->intensity * (a - b) / denom;
                        atten = std::max(atten, 0.0f);  // this line kinda useless but better safe than sorry

                        lit += light->color * lambert * atten;
                    }
                    else if (const LineLight* light = dynamic_cast<const LineLight*>(l)) {
                        const glm::vec3 startToEnd= light->end - light->start;

                        for (int i = 0; i <= light->segments; i++) {
                            const glm::vec3 pos = light->start + ((float)i/(float)light->segments) * startToEnd;

                            glm::vec3 toLight = pos - worldPos;
                            float dist = glm::length(toLight);
                            if (dist < 0.0001f || dist > light->radius) continue;

                            glm::vec3 dir = toLight / dist;
                            float lambert = glm::max(glm::dot(worldNorm, dir), 0.0f);
                            if (lambert <= 0) continue;

                            glm::vec3 origin = worldPos + worldNorm * 1e-3f; // shadow bias
                            if (rayOccluded(origin, dir, dist, rootNode)) continue;
                            
                            float a = 1/(1+(dist/light->falloff)*(dist/light->falloff));
                            float b = 1/(1+(light->radius/light->falloff)*(light->radius/light->falloff));
                            float denom = 1 - b;

                            float atten = light->intensity/(float)light->segments * (a - b) / denom;
                            atten = std::max(atten, 0.0f);  // this line kinda useless but better safe than sorry

                            lit += light->color * lambert * atten;
                        }
                    }
                }
                // hue preservation clamp
                float peak = glm::max(lit.r, glm::max(lit.g, lit.b));
                if (peak > 1.0f) lit /= peak;

                pixels[row * atlas->width + col] = lit;
                covMask[row * atlas->width + col] = 1;
            }
        }
    }
    // loops through texels that are unlit and sets them as their
    // lit texel neighbors average. this is meant for the texels that
    // technically arent in the triangle but their corner bleeds into
    // the triangle. loops twice to be accurate which is safe because
    // the atlas has an island padding of 4 texels.
    for (int j = 0; j < 2; j++) {
        std::vector<int> copy = covMask;
        for (size_t i = 0; i < covMask.size(); ++i)
        {
            if (covMask[i] == 1) continue;

            glm::vec3 total(0.0f);
            int count = 0;
            bool onTopEdge = false;
            bool onBottomEdge = false;
            bool onLeftEdge = false;
            bool onRightEdge = false;

            if (i < atlas->width)                   onTopEdge = true;
            if (i + atlas->width >= pixels.size())  onBottomEdge = true;
            if (i % atlas->width == 0)              onLeftEdge = true;
            if (i % atlas->width == atlas->width-1) onRightEdge = true;

            if (!onTopEdge) {
                if (covMask[i - atlas->width] == 1) { total += pixels[i - atlas->width]; count++; }
            }
            if (!onBottomEdge) {
                if (covMask[i + atlas->width] == 1) { total += pixels[i + atlas->width]; count++; }
            }
            if (!onLeftEdge) {
                if (covMask[i - 1] == 1) { total += pixels[i - 1]; count++; }
            }
            if (!onRightEdge) {
                if (covMask[i + 1] == 1) { total += pixels[i + 1]; count++; }
            }
            if (count > 0) {
                pixels[i] = total / float(count);
                copy[i] = 1;
            }
        }
        covMask = copy;
    }
    std::cout << "Baked " << atlas->width << " by " << atlas->height << " texel light map\n";

    lm->lightMap = Auric::Render::loadLightMap(pixels, atlas->width, atlas->height);
}

void initializeLightmap(LightMapComponent* lm, MeshComponent* mesh)
{
    xatlas::Atlas* atlas = lm->atlas;
    std::vector<float>& verts = mesh->vertices;
    std::vector<unsigned int>& idx = mesh->indices;
    const Transform& transform = lm->owner->transform;

    float ogArea = getMeshArea(verts, idx);
    float scaledArea = (transform.scaleX * transform.scaleY +
                        transform.scaleX * transform.scaleZ +
                        transform.scaleY * transform.scaleZ) * ogArea;
    float resolution = std::sqrt(scaledArea * 64 * 64) * lightmapResScalar;
    if (!GenUV2(atlas, verts, idx, resolution)) atlas = nullptr;
}

}
