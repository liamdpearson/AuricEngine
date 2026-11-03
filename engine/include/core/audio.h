// only included by audioSource.cpp
#pragma once

#include <glm/glm.hpp>                  // vec3, mat4, basic types


int initAudio();

void setMasterVolume(float vol);

void playSound2D(const char* path, float vol);

void updateAudio(const glm::vec3& pos, const glm::vec3& front, const glm::vec3& up);

void uninitSounds();

void uninitAudio();
