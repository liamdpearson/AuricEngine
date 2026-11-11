#include "include/core/animation.h"

#include <algorithm>


namespace Auric::Animate
{

// calculates the pose of each bone given the values at each frame and time
static void sampleClip(const Animation& anim, float t, std::vector<BonePose>& out)
{
    // calc which two frames to blend and by how much
    float dur = anim.duration > 0.0f ? anim.duration : 1.0f;
    float wrapped = std::fmod(t, dur); // wraps t to between 0 and dur
    float frameF = wrapped * anim.fps;
    int   f0 = (int)std::floor(frameF); // last frame
    int   f1 = (f0 + 1); // next frame
    float a = frameF - std::floor(frameF); // gets percentage between last and next

    int n = (int)std::min(out.size(), anim.tracks.size());
    for (int b = 0; b < n; ++b)
    {
        const BoneTrack& tr = anim.tracks[b];
        // calculates correct values given the nearby values and percentage between
        out[b].pos = glm::mix(tr.pos[f0], tr.pos[f1], a);
        out[b].rot = glm::slerp(tr.rot[f0], tr.rot[f1], a);
        out[b].scale = glm::mix(tr.scale[f0], tr.scale[f1], a);
    }
}

// fills palette with each bone's skinning matrix, doesn't take
// AnimatedMesh by const ref because it needs to set its lastPose
void computePose(Rig& rig)
{
    const Skeleton& sk = rig.skeleton;
    int n = sk.parent.size();
    rig.palette.assign(n, glm::mat4(1.0f)); // fills palette with a bunch of I mats
    if (n == 0 || rig.currentAnim < 0 || rig.currentAnim >= (int)rig.animations.size())
        return;
    const Animation& anim = rig.animations[rig.currentAnim];
    if (anim.frameCount == 0) return;

    // sample clip and get each bone's pose
    std::vector<BonePose> pose(n);
    sampleClip(anim, rig.animTime, pose);

    if (rig.blendDuration > 0.0f && (int)rig.blendFrom.size() == n)
    {
        float along = std::clamp(rig.blendElapsed / rig.blendDuration, 0.0f, 1.0f);
        for (int b = 0; b < n; ++b)
        {
            pose[b].pos = glm::mix(rig.blendFrom[b].pos, pose[b].pos, along);
            pose[b].rot = glm::slerp(rig.blendFrom[b].rot, pose[b].rot, along);
            pose[b].scale = glm::mix(rig.blendFrom[b].scale, pose[b].scale, along);
        }
        if (along >= 1.0f) // fade finished
        {
            rig.blendDuration = 0.0f;
            rig.blendElapsed = 0.0f;
            rig.blendFrom.clear();
        }
    }

    // remember what last went on screen so switching clips
    // mid blend starts from the pose the eye last saw.
    rig.lastPose = pose;

    // now we convert from local pos, rot, scale to local matrix
    std::vector<glm::mat4> local(n);
    for (int b = 0; b < n; ++b)
    {
        local[b] = glm::translate(glm::mat4(1.0f), pose[b].pos)
                 * glm::mat4_cast(pose[b].rot)
                 * glm::scale(glm::mat4(1.0f), pose[b].scale);
    }

    // now walk bone hierarchy and create world matrices
    std::vector<glm::mat4> world(n);
    std::vector<char> done(n, 0);
    int remaining = n;
    while (remaining > 0)
    {
        int progressed = 0;
        for (int b = 0; b < n; ++b)
        {
            if (done[b]) continue;
            int par = sk.parent[b];
            if (par < 0) // root bone
            {
                world[b] = sk.parentWorld[b] * local[b];
                done[b] = 1;
                remaining--;
                progressed = 1;
            }
            else if (done[par]) // seen parent already
            {
                world[b] = world[par] * local[b];
                done[b] = 1;
                remaining--;
                progressed = 1;
            }   
        }
        if (!progressed) break; // guards against broken parent chain
    }

    // palette = animated world * invbind
    if (sk.inverseBind.size() == size_t(n))
        for (int b = 0; b < n; ++b) rig.palette[b] = world[b] * sk.inverseBind[b];
    else
        for (int b = 0; b < n; ++b) rig.palette[b] = world[b];
    rig.boneWorlds = world;
}

}
