#pragma once

#include "include/scene/object.h"

#include <vector>
#include <string>


namespace Auric::Scripting
{

struct ScriptInstance;

extern std::vector<ScriptInstance> scripts;

void attachScript(Object* obj, std::string path);

void resetScripting();

void loadScripts();

void initScripting();

void startScripts();

void updateScripts();

}
