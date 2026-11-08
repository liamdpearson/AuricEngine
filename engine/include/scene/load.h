#pragma once

#include <string>


namespace Auric::Scene
{

// set scene variables
extern std::string startupScene;
extern std::string pendingScene;

int loadScene(const char* path);

int swapScene(const char* path);

int loadConfig(const char* path);

}
