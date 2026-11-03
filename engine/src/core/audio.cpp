#include "include/audio.h"
#include "audio_internal.h"

#include <iostream>


int initAudio()
{
    if (initialized) return 1; // audio already initialized

    ma_result result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS)
    {
        std::cout << "initAudio: Audio failed to initialize\n"
                  << ma_result_description(result) << '\n';
        return 0;
    }

    initialized = true;
    ma_engine_set_volume(&engine, 1.0f);

    return 1;
}

// sets master volume 0 is silent 1 is normal 2 is double
void setMasterVolume(float vol) 
{
    if (initialized) {
        ma_result result = ma_engine_set_volume(&engine, vol);
        if (result != MA_SUCCESS) {
            std::cout << "Failed to set volume" << '\n';
        }
    }
}

// finds first empty voice slot and returns index
int findEmpty()
{
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        if (!voices[i].active) return i;
    }
    return -1;
}

// no spacial audio - miniaudio handles ma_sound stuff
void playSound2D(const char* path, float vol)
{
    int i = findEmpty();
    if (i < 0) {
        std::cout << "voices full\n"; return;
    }
    ma_sound& sound = voices[i].sound;

    if (voices[i].initialized) {
        ma_sound_uninit(&sound);
        voices[i].initialized = false;
    }

    ma_result result = ma_sound_init_from_file(&engine, path, MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION,
                                               NULL, NULL, &sound);                               
    if (result != MA_SUCCESS) {
        std::cout << "Failed to initialize " << path << '\n';
        return;
    }
    
    voices[i].active = true;
    voices[i].initialized = true;
    
    ma_sound_set_volume(&sound, vol);
    ma_sound_start(&sound);

    voicesActive2d.push_back(i);
}

// initalizes sound and returns index to it in voices
int initSound(const char* path, const glm::vec3& pos, float vol, bool loop)
{
    int i = findEmpty();
    if (i < 0) {
        std::cout << "voices full\n"; return -1;
    }

    ma_sound& sound = voices[i].sound;

    if (voices[i].initialized) {
        ma_sound_uninit(&sound);
        voices[i].initialized = false;
    }
    
    ma_result result = ma_sound_init_from_file(&engine, path, MA_SOUND_FLAG_DECODE,
                                               NULL, NULL, &sound);                               
    if (result != MA_SUCCESS) {
        std::cout << "Failed to initialize " << path << '\n';
        return -1;
    }

    voices[i].active = true;
    voices[i].initialized = true;
  
    ma_sound_set_position(&sound, pos.x, pos.y, pos.z);
    ma_sound_set_attenuation_model(&sound, ma_attenuation_model_linear);
    ma_sound_set_rolloff(&sound, 0.0f);

    ma_sound_set_volume(&sound, vol);
    ma_sound_set_looping(&sound, loop ? MA_TRUE : MA_FALSE);

    return i;
}

void updateAudio(const glm::vec3& pos, const glm::vec3& front, const glm::vec3& up)
{
    if (!initialized) return;

    listener = pos;

    std::vector<int> remove = {};
    for (int i : voicesActive2d) {
        if (ma_sound_at_end(&voices[i].sound)) {
            ma_sound_uninit(&voices[i].sound);
            voices[i].initialized = false;
            voices[i].active = false;
            remove.push_back(i);
        }
    }
    for (int i : remove) {
        voicesActive2d.erase(std::remove(voicesActive2d.begin(), voicesActive2d.end(), i), voicesActive2d.end());
    }
    ma_engine_listener_set_position(&engine, 0, pos.x, pos.y, pos.z);
    ma_engine_listener_set_direction(&engine, 0, front.x, front.y, front.z);
    ma_engine_listener_set_world_up(&engine, 0, up.x, up.y, up.z);
}

// uninits all sounds
void uninitSounds()
{
    for (Voice& v : voices) {
        if (v.initialized) {
            ma_sound_stop(&v.sound);
            ma_sound_uninit(&v.sound);
            v.active = false;
            v.initialized = false;
        }
    }
}

// called when closing game
void uninitAudio()
{
    if (initialized) {
        uninitSounds();
        ma_engine_uninit(&engine);
        initialized = false;
    }
}
