#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp> // vec3, mat4, basic types

#include <xatlas/xatlas.h>
                       
#include "include/core/constants.h"
#include "include/types/primatives.h"
#include "include/types/animation.h"

#include <vector>
#include <string>
#include <memory>


class Object;

struct Component
{
    Object* owner = nullptr;
    virtual ~Component() = default;
    static inline const unsigned int CID = 0;
};

// A node in the scene hierarchy
class Object
{
    private:
        glm::mat4 world = glm::mat4(1.0f);
        int boneIndex = -1; // -1 unless object is a child of a bone in an armature
        std::string name, tag;
        std::vector<std::unique_ptr<Component>> components;

    public:
        bool draw = true;
        Transform transform;
        std::vector<std::unique_ptr<Object>> children;
        Object* parent = nullptr; // nullptr if root, points too parent object
        
        Object() = default;
        Object(const Transform& t)
            : transform(t) {}

        void addComponent(std::unique_ptr<Component> comp) {
            comp->owner = this;
            this->components.push_back(std::move(comp));
        }

        template <typename T>
        T* getComponent() const {
            for (auto& c : components)
                if (T::CID == c->CID) {
                    T* t = static_cast<T*>(c.get());
                    return t;
                }
            return nullptr;
        }

        void setWorld(const glm::mat4& world) { this->world = world; }
        glm::mat4 getWorld() const  { return this->world; }

        void setBoneIndex(int bi) { this->boneIndex = bi; }
        int getBoneIndex() const { return this->boneIndex; }

        void setName(std::string n) { this->name = n; }
        std::string getName() const { return this->name; }

        void setTag(std::string t) { this->tag = t; }
        std::string getTag() const { return this->tag; }

        void addChild(std::unique_ptr<Object> child, int bi = -1) {
            if (bi > -1)
                child->boneIndex = bi;
            child->parent = this;
            this->children.push_back(std::move(child));
        }

        ~Object() = default;
};

struct MeshComponent : Component 
{
    static inline const unsigned int CID = Auric::Constants::MESHID;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int texture = 0;

    GLsizei indexCount = 0;
    unsigned int VAO = 0, VBO = 0, EBO = 0;

    ~MeshComponent() override;
};

struct CameraComponent : Component
{
    static inline const unsigned int CID = Auric::Constants::CAMERAID;
    
    glm::vec3 pos{0.0f, 0.0f, 0.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};

    float FOV;
};

struct RigComponent : Component
{
    static inline const unsigned int CID = Auric::Constants::RIGID;
    
    Rig rig;
};

struct LightMapComponent : Component 
{
    static inline const unsigned int CID = Auric::Constants::LIGHTMAPID;

    unsigned int lightMap = 0;
    xatlas::Atlas* atlas = nullptr;

    ~LightMapComponent() override;
};

// just a boolean to tell collision code
// to add the objects mesh to the colliders
struct ColliderComponent : Component {
    static inline const unsigned int CID = Auric::Constants::COLLIDERID;
};

struct CapsuleColliderComponent : Component
{
    static inline const unsigned int CID = Auric::Constants::CAPSULECOLLIDERID;

    float height = 1.8f; // capsule height
    float radius = 0.3f; // capsule width
    glm::vec3 velocity{0.0f};
};

struct AudioSourceComponent : Component
{
    static inline const unsigned int CID = Auric::Constants::AUDIOSOURCEID;

    std::string path;
    int index = -1;

    bool loop = false;
    float volume = 1.0f;
    float maxDistance = 50.0f;
    float falloff = 1.0f;

    ~AudioSourceComponent() override;
};

struct ScriptComponent : Component
{
    static inline const unsigned int CID = Auric::Constants::SCRIPTID;

    std::string path;
};

// define scene variables
extern std::vector<std::unique_ptr<Object>> rootObjs;

Object* findObject(const std::string& name,
                   const std::vector<std::unique_ptr<Object>>& objs);
