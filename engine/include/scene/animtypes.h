#pragma once

#include <glm/glm.hpp>                  // vec3, mat4, basic types
#include <glm/gtc/matrix_transform.hpp> // perspective, lookAt, rotate, radians
#include <glm/gtc/type_ptr.hpp>         // value_ptr (hand a matrix to OpenGL)
#include <glm/gtc/quaternion.hpp>       // quat slerp, mat4_cast

#include <vector>
#include <string>


// maximum bones because I store bone indices as a single
// unsigned int so each bone has to be less than 8 bits.
const int MAX_BONES = 128;

// all vectors are of the same length bone owns each at its index
struct Skeleton
{
    std::vector<glm::mat4> inverseBind;
    std::vector<int> parent;
    std::vector<std::string> names;
    std::vector<glm::mat4> parentWorld;
};

struct BoneTrack
{
    std::vector<glm::vec3> pos;
    std::vector<glm::quat> rot;
    std::vector<glm::vec3> scale;
};

struct BonePose
{
    glm::vec3 pos{0.0f};
    glm::quat rot{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

struct Animation
{
    std::string name;
    std::vector<BoneTrack> tracks;
    int frameCount = 0;
    float fps = 30.0f;
    float duration = 0.0f; // seconds
};

struct Rig
{
    Skeleton skeleton;

    std::vector<glm::mat4> palette;
    std::vector<glm::mat4> boneWorlds;
    std::vector<Animation> animations; // all clips baked from fbx
    int currentAnim = -1;  // current animation index
    float animTime = 0.0f; // seconds into current clip
    int nextAnim = -1;     // animation set to play after current one done
    float animSpeed = 1.0f;

    // lastPose is updated every frame (what player last saw)
    // blendFrom is empty unless currently fading between animations
    std::vector<BonePose> lastPose;
    std::vector<BonePose> blendFrom;
    float blendDuration = 0.0f;
    float blendElapsed = 0.0f;

    void setAnim(int index, float blendTime = 0.0f, int nextAnim = -1)
    {
        if (index < 0 || index >= (int)animations.size()) return;
        
        // blend from the pose on screen if animation is already running
        if (blendTime > 0.0f && !lastPose.empty())
        {
            blendFrom = lastPose;
            blendDuration = blendTime;
            blendElapsed = 0.0f;
        }
        else
        {
            blendFrom.clear();
            blendDuration = 0.0f;
            blendElapsed = 0.0f;
        }

        currentAnim = index;
        animTime = 0.0f;
        this->nextAnim = nextAnim;
    }

    void setAnim(const std::string& name, float blendTime = 0.0f, int nextAnim = -1)
    {
        for (int i = 0; i < (int)animations.size(); ++i)
        {
            if (animations[i].name == name)
            {
                setAnim(i, blendTime, nextAnim);
            }
        }
    }

    int findBoneIndex(std::string name) {
        for (int b = 0; b < (int)this->skeleton.names.size(); ++b)
            if (this->skeleton.names[b] == name) return b;

        return -1;
    }
};
