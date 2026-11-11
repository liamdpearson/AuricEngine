#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp> // vec3, mat4, basic types
#include <glm/gtc/matrix_transform.hpp> // perspective, lookAt, rotate, radians
#include <glm/gtc/type_ptr.hpp>         // value_ptr (hand a matrix to OpenGL)
#include <glm/gtc/quaternion.hpp>       // quat slerp, mat4_cast

#include <xatlas/xatlas.h>
                       
#include "include/core/constants.h"
#include "include/scene/animtypes.h"

#include <vector>
#include <string>
#include <memory>


struct Transform
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
    float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;

    Transform() = default;

    Transform(const Transform&) = default;

    // builds local to world matrix
    glm::mat4 matrix() const
    {
        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3(x, y, z));
        m = glm::rotate(m, glm::radians(yaw), glm::vec3(0, 1, 0));
        m = glm::rotate(m, glm::radians(pitch), glm::vec3(1, 0, 0));
        m = glm::rotate(m, glm::radians(roll), glm::vec3(0, 0, 1));
        m = glm::scale(m, glm::vec3(scaleX, scaleY, scaleZ));
        return m;
    }

    bool operator==(const Transform& other) const
    {
        return (x == other.x && y == other.y && z == other.z
                && yaw == other.yaw && pitch == other.pitch && roll == other.roll
                && scaleX == other.scaleX && scaleY == other.scaleY && scaleZ == other.scaleZ);
    }

    Transform& operator=(const Transform&) = default;
};

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

template <typename F>
void forEachObj(Object& obj, F&& fn)
{
    fn(obj); 
    for (auto& child : obj.children)
        forEachObj(*child, fn);
}

template <typename F>
void forEachObj(F&& fn)
{
    for (auto& obj: rootObjs)
        forEachObj(*obj, fn);
}

Object* findObject(const std::string& name,
                   const std::vector<std::unique_ptr<Object>>& objs);
