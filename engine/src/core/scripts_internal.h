#pragma once

#include "include/core/scripts.h"
#include <lua/lua.hpp>
#include <sol/sol.hpp>


namespace Auric::Scripting
{

struct ScriptInstance
{
    std::string path;
    Object* obj;

    sol::environment env;
    sol::protected_function startFn;
    sol::protected_function updateFn;
    void Start() const;
    void Update(float deltaTime) const;
};

extern sol::state lua;

}
