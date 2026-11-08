// only included by audioSource.cpp
#pragma once

#include <glm/glm.hpp>                  // vec3, mat4, basic types

#include "include/scene/object.h"


namespace Auric::Audio
{

int initAudio();

void setMasterVolume(float vol);

void playSound2D(const char* path, float vol);

void playAudioSource(AudioSourceComponent* as);

void stopAudioSource(AudioSourceComponent* as);

void uninitializeAudioSource(AudioSourceComponent* as);

void updateAudio(const glm::vec3& pos, const glm::vec3& front, const glm::vec3& up);

void uninitSounds();

void uninitAudio();

}
