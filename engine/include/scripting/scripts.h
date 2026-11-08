#pragma once

#include "include/scene/object.h"

#include <vector>
#include <string>


struct ScriptInstance;

extern std::vector<ScriptInstance> scripts;

namespace Auric::Scripting
{

void attachScript(Object* obj, std::string path);

void resetScripting();

void loadScripts();

void initScripting();

void startScripts();

void updateScripts();

}
