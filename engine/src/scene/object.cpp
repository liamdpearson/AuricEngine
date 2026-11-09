#include "include/scene/object.h"

#include "include/core/audio.h"


using namespace Auric;

// define scene variables
std::vector<std::unique_ptr<Object>> rootObjs;

Object* findObject(const std::string& name,
                   const std::vector<std::unique_ptr<Object>>& objs)
{
    for (const std::unique_ptr<Object>& obj : objs)
    {
        if (obj->getName() == name) return obj.get();
        Object* found = findObject(name, obj->children);
        if (found) return found;
    }
    return nullptr;
}

// deletes VAO, VBO, EBO, and texture.
MeshComponent::~MeshComponent()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);
}

LightMapComponent::~LightMapComponent()
{
    glDeleteTextures(1, &lightMap);
    xatlas::Destroy(atlas);
}

AudioSourceComponent::~AudioSourceComponent()
{
    Audio::uninitializeAudioSource(this);
}
