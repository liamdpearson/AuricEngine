#include <glm/glm.hpp>                  // vec3, mat4, basic types
#include <xatlas/xatlas.h>
#include <ufbx/ufbx.h>
#include <vector>
#include <string>
#include <iostream>

#include "include/scene/object.h"



namespace Wayback3D::Assets {
    std::pair<std::unique_ptr<MeshComponent>, std::unique_ptr<RigComponent>> makeMesh(const char* objSrc,
                                                                                               const char* texSrc,
                                                                                               bool pixelated);
    std::unique_ptr<MeshComponent> makeCamera(float FOV);

    std::unique_ptr<RigComponent> makeRig(const char* path);

    std::unique_ptr<LightMapComponent> makeLightMap();

    std::unique_ptr<ColliderComponent> makeCollider();

    std::unique_ptr<CapsuleColliderComponent> makeCapsuleCollider(float radius, float height);

    std::unique_ptr<AudioSourceComponent> makeAudioSource(const char* path, float volume,
                                                          float maxDistance, float falloff, bool loop);

    std::unique_ptr<ScriptComponent> makeScript(const std::string path);
}

std::unique_ptr<StaticMesh> makeStaticMesh(const Transform& transform, const char* objPath,
                          const char* texPath, bool pixelated, bool collides)
{   
    auto obj = std::make_unique<StaticMesh>();

    std::vector<float> verts;
    std::vector<unsigned int> idx;

    const char* file = strrchr(objPath, '.');
    if (file) {
        if (strcmp(file, ".obj") == 0) {
            loadObj(objPath, verts, idx);
        }
        else if (strcmp(file, ".fbx") == 0) {
            loadFbxUnanimated(objPath, verts, idx);
        }
        else {
            std::cout << "File not recognized: " << objPath << '\n';
        }
    } else {
        std::cout << "No file type: " << objPath << '\n';
    }
    
    xatlas::Atlas* atlas = nullptr;
    float ogArea = getMeshArea(verts, idx);
    float scaledArea = (transform.scaleX * transform.scaleY +
                        transform.scaleX * transform.scaleZ +
                        transform.scaleY * transform.scaleZ) * ogArea;
    float resolution = std::sqrt(scaledArea * 64 * 64) * lightmapResScalar;
    if (!GenUV2(atlas, verts, idx, resolution)) atlas = nullptr;
    

    obj->transform = transform;
    obj->setVertices(verts);
    obj->setIndices(idx);
    obj->setTexture(loadTexture(texPath, pixelated));
    obj->setIndexCount((GLsizei)idx.size());

    obj->setAtlas(atlas);
    obj->setCollides(collides);
    
    return obj;
}

std::unique_ptr<Mesh> makeMesh(const Transform& transform, const char* objPath,
              const char* texPath, bool pixelated)
{
    auto obj = std::make_unique<Mesh>();

    std::vector<float> verts;
    std::vector<unsigned int> idx;

    const char* file = strrchr(objPath, '.');
    if (file) {
        if (strcmp(file, ".obj") == 0) {
            loadObj(objPath, verts, idx);
        }
        else if (strcmp(file, ".fbx") == 0) {
            loadFbxUnanimated(objPath, verts, idx);
        }
        else {
            std::cout << "File not recognized: " << objPath << '\n';
        }
    } else {
        std::cout << "No file type: " << objPath << '\n';
    }

    obj->transform = transform;
    obj->setVertices(verts);
    obj->setIndices(idx);
    obj->setTexture(loadTexture(texPath, pixelated));
    obj->setIndexCount((GLsizei)idx.size());

    return obj;
}

std::unique_ptr<AnimatedMesh> makeAnimatedMesh(const Transform& transform, const char* objPath,
                              const char* texPath, bool pixelated)
{
    auto obj = std::make_unique<AnimatedMesh>();

    std::vector<float> verts;
    std::vector<unsigned int> idx;
    Skeleton skel;
    std::vector<Animation> anims;

    const char* file = strrchr(objPath, '.');
    if (file) {
        if (strcmp(file, ".fbx") == 0) {
            loadFbxAnimatedMesh(objPath, verts, idx, skel, anims);
        }
        else {
            std::cout << "File type not recognized: " << objPath << '\n';
        }
    } else {
        std::cout << "No file type: " << objPath << '\n';
    }
    
    Rig rig;

    obj->transform = transform;
    obj->setVertices(verts);
    obj->setIndices(idx);
    obj->setTexture(loadTexture(texPath, pixelated));
    obj->setIndexCount((GLsizei)idx.size());

    rig.skeleton = skel;
    rig.animations = anims;

    obj->rig = rig;

    return obj;
}

std::unique_ptr<AnimatedObj> makeAnimatedObj(const Transform& transform, const char* objPath)
{
    auto obj = std::make_unique<AnimatedObj>();

    Skeleton skel;
    std::vector<Animation> anims;

    const char* file = strrchr(objPath, '.');
    if (file) {
        if (strcmp(file, ".fbx") == 0) {
            loadFbxAnimatedObj(objPath, skel, anims);
        }
        else {
            std::cout << "File type not recognized: " << objPath << '\n';
        }
    } else {
        std::cout << "No file type: " << objPath << '\n';
    }
    
    Rig rig;
    obj->transform = transform;
    rig.skeleton = skel;
    rig.animations = anims;

    obj->rig = rig;

    return obj;
}

std::unique_ptr<AudioSource> makeAudioSource(const Transform& transform, const std::string& path,
                                             float volume, float maxDistance, float falloff, bool loop)
{
    auto obj = std::make_unique<AudioSource>();
    
    obj->transform = transform;
    obj->volume = volume;
    obj->maxDistance = maxDistance;
    obj->falloff = falloff;
    obj->setLoop(loop);
    obj->setPath(path);

    return obj;
}




// loads .fbx file unanimated
// void loadFbxUnanimated(const char* path, std::vector<float>& outVerts,
//                        std::vector<unsigned int>& outIndices)
// {
//     ufbx_load_opts opts = {}; // default options
//     opts.target_axes = ufbx_axes_right_handed_y_up;
//     opts.target_unit_meters = 1.0f;
//
//     ufbx_error error;
//     ufbx_scene* scene = ufbx_load_file(path, &opts, &error);
//     if (!scene)
//     {
//         std::cout << "Failed to load scene. Path: " << path 
//                   << " Description: " << error.description.data << '\n';
//         ufbx_free_scene(scene);
//         return;
//     }
//
//     for (size_t i = 0; i < scene->nodes.count; ++i)
//     {
//         ufbx_node* node = scene->nodes.data[i];
//         ufbx_mesh* mesh = node->mesh;
//         if (!mesh) continue;
//
//         ufbx_matrix geomToWorld = node->geometry_to_world;
//         ufbx_matrix geomToWorldNormals = ufbx_matrix_for_normals(&geomToWorld);
//         std::vector<unsigned int> tri(mesh->max_face_triangles * 3);
//
//         for (size_t fi = 0; fi < mesh->faces.count; ++fi)
//         {
//             ufbx_face face = mesh->faces[fi];
//             size_t numTris = ufbx_triangulate_face(tri.data(), tri.size(), mesh, face);
//
//             for (size_t i = 0; i < numTris * 3; ++i)
//             {
//                 unsigned int corner = tri[i];
//                 ufbx_vec3 p = ufbx_get_vertex_vec3(&mesh->vertex_position, corner);
//                 p = ufbx_transform_position(&geomToWorld, p);
//                 ufbx_vec2 uv = mesh->vertex_uv.exists
//                     ? ufbx_get_vertex_vec2(&mesh->vertex_uv, corner)
//                     : ufbx_vec2{0.0f, 0.0f};
//                 ufbx_vec3 n = mesh->vertex_normal.exists
//                     ? ufbx_get_vertex_vec3(&mesh->vertex_normal, corner)
//                     : ufbx_vec3{0.0f, 0.0f, 1.0f};
//                 n = ufbx_transform_direction(&geomToWorldNormals, n);
//
//                 outIndices.push_back((unsigned int)(outVerts.size() / VERTEX_FLOATS));
//                 outVerts.push_back((float)p.x);
//                 outVerts.push_back((float)p.y);
//                 outVerts.push_back((float)p.z);
//                 outVerts.push_back((float)uv.x);
//                 outVerts.push_back((float)uv.y);
//                 outVerts.push_back((float)n.x);
//                 outVerts.push_back((float)n.y);
//                 outVerts.push_back((float)n.z);
//                 outVerts.push_back(0.0f); // light map uv
//                 outVerts.push_back(0.0f);
//                 outVerts.push_back(0.0f); // empty anim data
//                 outVerts.push_back(0.0f);
//                 outVerts.push_back(0.0f);
//                 outVerts.push_back(0.0f);
//                 outVerts.push_back(0.0f);
//             }
//         }
//     }
//     ufbx_free_scene(scene);
// }
