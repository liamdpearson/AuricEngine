#include "include/scene/assets.h"

#include <cassert>
#include <glm/glm.hpp>                  // vec3, mat4, basic types
#include <memory>
#include <utility>
#include <xatlas/xatlas.h>
#include <ufbx/ufbx.h>
#include <vector>
#include <string>
#include <iostream>

#include "include/render/graphics.h"
#include "include/scene/object.h"
#include "include/scene/mesh.h"
#include "include/types/animation.h"


using namespace Auric;

namespace Auric::Assets {

std::pair<std::unique_ptr<MeshComponent>, std::unique_ptr<RigComponent>> makeMesh(const char* objSrc,
                                                                                           const char* texSrc,
                                                                                           bool pixelated)
{
    auto mesh = std::make_unique<MeshComponent>();

    std::vector<float> verts;
    std::vector<unsigned int> idx;
    Skeleton skel;
    std::vector<Animation> anims;

    const char* file = strrchr(objSrc, '.');
    if (file) {
        if (strcmp(file, ".fbx") == 0) {
            loadFbx(objSrc, verts, idx, skel, anims);
        }
        else {
            std::cout << "File type not recognized: " << objSrc << '\n';
        }
    } else {
        std::cout << "No file type: " << objSrc << '\n';
    }

    mesh->vertices = verts;
    mesh->indices  = idx;
    mesh->texture  = Render::loadTexture(texSrc, pixelated);
    mesh->indexCount = (GLsizei)idx.size();

    if (skel.inverseBind.size() > 0) {
        auto rig  = std::make_unique<RigComponent>();

        Rig rigObj;
        rigObj.skeleton = skel;
        rigObj.animations = anims;

        rig->rig = rigObj;

        return std::make_pair(std::move(mesh), std::move(rig));
    } else {
        return std::make_pair(std::move(mesh), nullptr);
    }
}

std::unique_ptr<CameraComponent> makeCamera(float FOV)
{
    auto cam = std::make_unique<CameraComponent>();

    cam->FOV = FOV;

    return cam;
}

std::unique_ptr<RigComponent> makeRig(const char* path)
{
    auto rig = std::make_unique<RigComponent>();

    Skeleton skel;
    std::vector<Animation> anims;

    const char* file = strrchr(path, '.');
    if (file) {
        if (strcmp(file, ".fbx") == 0) {
            loadFbxRig(path, skel, anims);
        }
        else {
            std::cout << "File type not recognized: " << path << '\n';
        }
    } else {
        std::cout << "No file type: " << path << '\n';
    }

    Rig rigObj;

    rigObj.skeleton = skel;
    rigObj.animations = anims;

    rig->rig = rigObj;

    return rig;
}

std::unique_ptr<LightMapComponent> makeLightMap()
{
    auto lm = std::make_unique<LightMapComponent>();

    // std::vector<float> verts = mesh->vertices;
    // std::vector<unsigned int> idx = mesh->indices;
    
    // xatlas::Atlas* atlas = nullptr;
    // float ogArea = getMeshArea(verts, idx);
    // float scaledArea = (transform.scaleX * transform.scaleY +
    //                     transform.scaleX * transform.scaleZ +
    //                     transform.scaleY * transform.scaleZ) * ogArea;
    // float resolution = std::sqrt(scaledArea * 64 * 64) * lightmapResScalar;
    // if (!GenUV2(atlas, verts, idx, resolution)) atlas = nullptr;

    // lm->atlas = atlas;

    return lm;
}

std::unique_ptr<ColliderComponent> makeCollider()
{
    auto collider = std::make_unique<ColliderComponent>();

    return collider;
}

std::unique_ptr<CapsuleColliderComponent> makeCapsuleCollider(float radius = 0.3f, float height = 1.8f)
{
    auto capsuleCol = std::make_unique<CapsuleColliderComponent>();

    capsuleCol->height = height;
    capsuleCol->radius = radius;

    return capsuleCol;
}

std::unique_ptr<AudioSourceComponent> makeAudioSource(const char* path, float volume,
                                                      float maxDistance, float falloff, bool loop)
{
    auto audioSource = std::make_unique<AudioSourceComponent>();

    audioSource->path = path;
    audioSource->volume = volume;
    audioSource->maxDistance = maxDistance;
    audioSource->falloff = falloff;
    audioSource->loop = loop;

    return audioSource;
}

std::unique_ptr<ScriptComponent> makeScript(const std::string path)
{
    auto script = std::make_unique<ScriptComponent>();

    script->path = path;
    
    return script;
}

}
