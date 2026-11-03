#pragma once

#include "include/scene/object.h"
#include <memory>
#include <string>
#include <xatlas/xatlas.h>


namespace Auric::Assets {

std::pair<std::unique_ptr<MeshComponent>, std::unique_ptr<RigComponent>> makeMesh(const char* objSrc,
                                                                                           const char* texSrc,
                                                                                           bool pixelated);
std::unique_ptr<CameraComponent> makeCamera(float FOV);

std::unique_ptr<RigComponent> makeRig(const char* path);

std::unique_ptr<LightMapComponent> makeLightMap(MeshComponent* mesh);

std::unique_ptr<ColliderComponent> makeCollider();

std::unique_ptr<CapsuleColliderComponent> makeCapsuleCollider(float radius, float height);

std::unique_ptr<AudioSourceComponent> makeAudioSource(const char* path, float volume,
                                                      float maxDistance, float falloff, bool loop);

std::unique_ptr<ScriptComponent> makeScript(const std::string path);

}
