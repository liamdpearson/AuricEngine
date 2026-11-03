#include "include/assets/lightmap.h"

#include <vector>
#include <iostream>

#include "include/core/constants.h"



// loops through mesh tris and adds up area
float getMeshArea(const std::vector<float>& verts,
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
bool GenUV2(xatlas::Atlas*& atlas, std::vector<float>& verts,
                   std::vector<unsigned int>& idx, float resolution)
{

    atlas = xatlas::Create();

    xatlas::MeshDecl meshDecl;
    meshDecl.vertexCount = static_cast<uint32_t>(verts.size() / Constants::VERTEX_FLOATS);
    meshDecl.vertexPositionData = verts.data();
    meshDecl.vertexPositionStride = sizeof(float) * Constants::VERTEX_FLOATS;

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

        const uint32_t ogVertIdx = xatlasVert.xref * Constants::VERTEX_FLOATS;
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
              << finalVerts.size() / Constants::VERTEX_FLOATS << '\n'
              << "# of atlas islands: " << outputMesh.chartCount << '\n';

    return true;
}
