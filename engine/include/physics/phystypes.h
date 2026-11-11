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
