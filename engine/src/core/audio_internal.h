#pragma once

#include <glm/glm.hpp>
#include <miniaudio/miniaudio.h>

#include <vector>


inline ma_engine engine;
inline bool initialized = false;

// only 32 voices allowed to play at once
const int MAX_VOICES = 32;

struct Voice
{
    ma_sound sound;
    bool active = false;
    bool initialized = false;
};

// cached for volume calculation and muffle checking
static glm::vec3 listener;

static Voice voices[MAX_VOICES];
static std::vector<int> voicesActive2d = {};

int findEmpty();

int initSound(const char* path, const glm::vec3& pos, float vol, bool loop);
