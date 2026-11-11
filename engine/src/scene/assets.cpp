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

#include "include/core/constants.h"
#include "include/core/scripts.h"
#include "include/render/graphics.h"
#include "include/scene/object.h"
#include "include/scene/mesh.h"


using namespace Auric;

namespace Auric::Assets {

std::pair<std::unique_ptr<MeshComponent>, std::unique_ptr<RigComponent>> makeMesh(const char* objSrc, 
                                                                                  const char* texSrc,
                                                                                  bool pixelated, bool draw)
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
        else if (strcmp(file, ".obj") == 0) {
            loadObj(objSrc, verts, idx);
        }
        else {
            std::cout << "File type not recognized: " << objSrc << '\n';
        }
    } else {
        std::cout << "No file type: " << objSrc << '\n';
    }

    mesh->cid = Auric::Constants::MESHID;
    mesh->vertices = verts;
    mesh->indices  = idx;
    mesh->texture  = Render::loadTexture(texSrc, pixelated);
    mesh->indexCount = (GLsizei)idx.size();
    mesh->draw = draw;

    if (skel.inverseBind.size() > 0) {
        auto rig  = std::make_unique<RigComponent>();

        Rig rigObj;
        rigObj.skeleton = skel;
        rigObj.animations = anims;

        rig->cid = Auric::Constants::RIGID;
        rig->rig = rigObj;

        return std::make_pair(std::move(mesh), std::move(rig));
    } else {
        return std::make_pair(std::move(mesh), nullptr);
    }
}

std::unique_ptr<CameraComponent> makeCamera(float FOV)
{
    auto cam = std::make_unique<CameraComponent>();

    cam->cid = Auric::Constants::CAMERAID;
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

    rig->cid = Auric::Constants::RIGID;
    rigObj.skeleton = skel;
    rigObj.animations = anims;

    rig->rig = rigObj;

    return rig;
}

std::unique_ptr<LightMapComponent> makeLightMap()
{
    auto lm = std::make_unique<LightMapComponent>();

    lm->cid = Auric::Constants::LIGHTMAPID;

    return lm;
}

std::unique_ptr<ColliderComponent> makeCollider()
{
    auto collider = std::make_unique<ColliderComponent>();

    collider->cid = Auric::Constants::COLLIDERID;

    return collider;
}

std::unique_ptr<CapsuleColliderComponent> makeCapsuleCollider(float radius = 0.3f, float height = 1.8f)
{
    auto capsuleCol = std::make_unique<CapsuleColliderComponent>();

    capsuleCol->cid = Auric::Constants::CAPSULECOLLIDERID;
    capsuleCol->height = height;
    capsuleCol->radius = radius;

    return capsuleCol;
}

std::unique_ptr<AudioSourceComponent> makeAudioSource(const char* path, float volume,
                                                      float maxDistance, float falloff, bool loop)
{
    auto audioSource = std::make_unique<AudioSourceComponent>();

    audioSource->cid = Auric::Constants::AUDIOSOURCEID;
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

    script->cid = Auric::Constants::SCRIPTID;
    script->path = path;
    
    return script;
}

}
