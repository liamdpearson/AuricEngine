#pragma once

#include <glm/glm.hpp>                  // vec3, mat4, basic types
#include <glm/gtc/matrix_transform.hpp> // perspective, lookAt, rotate, radians
#include <glm/gtc/type_ptr.hpp>         // value_ptr (hand a matrix to OpenGL)
#include <glm/gtc/quaternion.hpp>       // quat slerp, mat4_cast
                                        

struct Tri{
    glm::vec3 a, b, c;
};

struct AABB
{
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};

    bool overlaps(const AABB& other) const
    {
        return min.x <= other.max.x && max.x >= other.min.x
            && min.y <= other.max.y && max.y >= other.min.y
            && min.z <= other.max.z && max.z >= other.min.z;
    }

    void expand(float margin)
    {
        min -= glm::vec3(margin);
        max += glm::vec3(margin);
    }
};

struct TriAABB {
    Tri tri;
    AABB aabb;
};

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
