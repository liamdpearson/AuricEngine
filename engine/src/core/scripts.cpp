#include "include/physics/raycast.h"
#include "scripts_internal.h"

#include "include/scene/object.h"
#include "include/scene/load.h"
#include "include/physics/collisions.h"
#include "include/render/window.h"
#include "include/core/input.h"
#include "include/core/audio.h"
#include "include/ui/ui.h"
#include <cstring>
#include <vector>
#include <fstream>


using namespace Auric;

namespace Auric::Scripting
{

sol::state lua;

std::vector<ScriptInstance> scripts;

void attachScript(Object* obj, std::string path)
{
    ScriptInstance si;
    si.obj = obj;
    si.path = path;
    scripts.push_back(si);
}

static sol::object uiToLua(UIElement* ui)
{
    if (UIImage* image = dynamic_cast<UIImage*>(ui))
        return sol::make_object(lua, image);

    else if (UIText* text = dynamic_cast<UIText*>(ui))
        return sol::make_object(lua, text);

    return sol::make_object(lua, ui);
}

static void registerLuaAPI()
{
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);

    // tell lua about the data types it can use
    lua.new_usertype<glm::vec3>("Vec3",
        sol::constructors<glm::vec3(float), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z,
        "normalize", [](glm::vec3& v) { // mutates v
            float len = glm::length(v);
            if (len > 1e-6f) v /= len;
        },
        "normalized", [](const glm::vec3& v) { // returns new v
            float len = glm::length(v);
            return len > 1e-6f ? v / len : glm::vec3(0.0f);
        },
        "length", [](const glm::vec3& v) { return glm::length(v); },
        "dot",    [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },
        "cross",  [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); },
        sol::meta_function::addition,    [](const glm::vec3& a, const glm::vec3 b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3 b) { return a - b; },
        sol::meta_function::multiplication, sol::overload(
            [](const glm::vec3& v, float s) { return v * s; },
            [](float s, const glm::vec3& v) { return v * s; }
        ),
        sol::meta_function::unary_minus, [](const glm::vec3& v) { return -v; },
        sol::meta_function::to_string,   [](const glm::vec3& v) {
            return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
        }
    );
    lua.new_usertype<UITransform>("UITransform",
        "x", &UITransform::x,
        "y", &UITransform::y,
        "angle", &UITransform::angle,
        "scaleX", &UITransform::scaleX,
        "scaleY", &UITransform::scaleY
    );
    lua.new_usertype<UIElement>("UIElement",
        "name", sol::property(&UIElement::getName, &UIElement::setName),
        "tag", sol::property(&UIElement::getTag, &UIElement::setTag),
        "draw", &UIElement::draw,
        "transform", &UIElement::transform
    );
    lua.new_usertype<UIImage>("UIImage",
        sol::base_classes, sol::bases<UIElement>()
    );
    lua.new_usertype<UIText>("UIText",
        "text", &UIText::text,
        "color", &UIText::color,
        sol::base_classes, sol::bases<UIElement>()
    );
    lua.new_usertype<Transform>("Transform",
        "x", &Transform::x,
        "y", &Transform::y,
        "z", &Transform::z,
        "yaw", &Transform::yaw,
        "pitch", &Transform::pitch,
        "roll", &Transform::roll,
        "scaleX", &Transform::scaleX,
        "scaleY", &Transform::scaleY,
        "scaleZ", &Transform::scaleZ,
        // returns just (x, y, z), small helper fn for users
        "pos", [](Transform* t) { return glm::vec3{t->x, t->y, t->z}; } 
    );
    lua.new_usertype<Object>("Object",
        "name", sol::property(&Object::getName, &Object::setName),
        "tag", sol::property(&Object::getTag, &Object::setTag),
        "draw", &Object::draw,
        "transform", &Object::transform,
        "getMesh", [](Object* obj) { return obj->getComponent<MeshComponent>(); },
        "getCamera", [](Object* obj) { return obj->getComponent<CameraComponent>(); },
        "getRig", [](Object* obj) { return obj->getComponent<RigComponent>(); },
        "getCapsuleCollider", [](Object* obj) { return obj->getComponent<CapsuleColliderComponent>(); },
        "getAudioSource", [](Object* obj) { return obj->getComponent<AudioSourceComponent>(); },
        "getScript", [](Object* obj) -> sol::object {
            for (ScriptInstance& si : scripts)
                if (si.obj == obj) return sol::make_object(lua, si.env);
            return sol::lua_nil;
        }
    );
    lua.new_usertype<MeshComponent>("Mesh",
        "draw", &MeshComponent::draw
    );
    lua.new_usertype<CameraComponent>("Camera",
        "pos", &CameraComponent::pos,
        "front", &CameraComponent::front,
        "up", &CameraComponent::up, 
        "setCurrent", [](CameraComponent* c) { currentCam = c; }
    );
    lua.new_usertype<RigComponent>("Rig",
        "currentAnim", sol::property(
          [](RigComponent& r)        { return r.rig.currentAnim; },
          [](RigComponent& r, int v) { r.rig.currentAnim = v; }
        ),
        "animSpeed", sol::property(
          [](RigComponent& r)          { return r.rig.animSpeed; },
          [](RigComponent& r, float v) { r.rig.animSpeed = v; }
        ),
        "setAnim", sol::overload(
            [](RigComponent& r, int i)                 { r.rig.setAnim(i); },
            [](RigComponent& r, int i, float b)        { r.rig.setAnim(i, b); },
            [](RigComponent& r, int i, float b, int n) { r.rig.setAnim(i, b, n); }
        )
    );
    lua.new_usertype<CapsuleColliderComponent>("Capsule",
        "height", &CapsuleColliderComponent::height,
        "radius", &CapsuleColliderComponent::radius,
        "velocity", &CapsuleColliderComponent::velocity,
        "resolveCollisions", [](CapsuleColliderComponent* cap, float deltaTime) { Physics::resolveCapsuleCollisions(cap, deltaTime); }
    );
    lua.new_usertype<AudioSourceComponent>("AudioSource",
        "loop", &AudioSourceComponent::loop,
        "volume", &AudioSourceComponent::volume,
        "Play", [](AudioSourceComponent* audiosource) {
            Audio::playAudioSource(audiosource); 
        },
        "Stop", [](AudioSourceComponent* audiosource) {
            Audio::stopAudioSource(audiosource); 
        }
    );
    lua.create_named_table("input",
        "keyPressed",   &Auric::Input::keyPressed,
        "keyHeld",      &Auric::Input::keyHeld,
        "keyReleased",  &Auric::Input::keyReleased,
        "mousePressed",  &Auric::Input::mouseButtonPressed,
        "mouseHeld",     &Auric::Input::mouseButtonHeld,
        "mouseReleased", &Auric::Input::mouseButtonReleased,
        "mouseDX", &Auric::Input::mouseDX,
        "mouseDY", &Auric::Input::mouseDY
    );
    lua.create_named_table("key",
        "Q", GLFW_KEY_Q,
        "W", GLFW_KEY_W,
        "E", GLFW_KEY_E,
        "R", GLFW_KEY_R,
        "T", GLFW_KEY_T,
        "Y", GLFW_KEY_Y,
        "U", GLFW_KEY_U,
        "A", GLFW_KEY_A,
        "S", GLFW_KEY_S,
        "D", GLFW_KEY_D,
        "SPACE",  GLFW_KEY_SPACE,
        "LSHIFT", GLFW_KEY_LEFT_SHIFT,
        "ESCAPE", GLFW_KEY_ESCAPE
    );
    lua.create_named_table("mouse",
        "LEFT",  GLFW_MOUSE_BUTTON_LEFT,
        "MIDDLE",  GLFW_MOUSE_BUTTON_MIDDLE,
        "RIGHT", GLFW_MOUSE_BUTTON_RIGHT
    );
    sol::table find = lua.create_named_table("find");
    find.set_function("obj", [](const std::string& name) {
        Object* obj = findObject(name, rootObjs);
        return obj;
    });
    find.set_function("ui", [](const std::string& name) {
        UIElement* ui = findUIElement(name, UI::uiRoots);
        return uiToLua(ui);
    });
    find.set_function("allWithTag", [](const std::string& tag) {
        std::vector<Object*> out;
        forEachObj([&](Object& o) { if (o.getTag() == tag) out.push_back(&o); });
        return sol::as_table(out);
    });
    sol::table raycast = lua.create_named_table("raycast");
    raycast.set_function("occluded", sol::overload(
        [](const glm::vec3& origin, const glm::vec3& dir, float maxDist) {
            return Physics::rayOccluded(origin, dir, maxDist, Physics::rootNode);
        },
        [](const glm::vec3& origin, const glm::vec3& destination) {
            return Physics::rayOccluded(origin, destination, Physics::rootNode);
        }
    ));
    raycast.set_function("hitCapsule", [](const glm::vec3& origin, const glm::vec3& dir, float maxDist,
                                          CapsuleColliderComponent* cap) { // returns distance to capsule or -1 if doesnt hit
            return Physics::rayHitCapsule(origin, dir, maxDist, cap);
    });
    lua.set_function(
        "swapScene", [](const std::string& path) { Auric::Scene::pendingScene = path; }
    );
    lua.set_function(
        "playSound2D", [](const std::string& path, float vol) { Auric::Audio::playSound2D(path.c_str(), vol); }
    );
    lua.set_function(
        "setMasterVolume", [](float vol) { Auric::Audio::setMasterVolume(vol); }
    );
    lua.set_function(
        "quit", []() { glfwSetWindowShouldClose(Auric::Render::window, true); }
    );
}

void resetScripting()
{
    scripts.clear();
    lua = sol::state{};
    registerLuaAPI();
}

void loadScripts()
{
    for (ScriptInstance& si : scripts)
    {
        std::ifstream file(si.path);
        if (!file) {
            std::cout << "Error opening lua file" << '\n';
            continue;
        }

        std::string source{
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        };

        sol::environment env(lua, sol::create, lua.globals());
        si.env = env;

        if (si.obj) {
            si.env["self"] = si.obj;
        }

        sol::load_result loaded = lua.load(source, si.path);
        if (!loaded.valid()) {
            sol::error err = loaded;
            std::cout << "Script load error: " << err.what() << '\n';
            continue;
        }
        sol::protected_function body = loaded;
        si.env.set_on(body);

        auto result = body();

        if (!result.valid()) {
            sol::error err = result;
            std::cout << "Script body error: " << err.what() << '\n';
            continue;
        }

        si.startFn = si.env["start"];
        if (!si.startFn.valid())
            std::cout << "Start function invalid" << '\n';
        si.updateFn = si.env["update"];
        if (!si.updateFn.valid())
            std::cout << "Update function invalid" << '\n';
    }
}

void ScriptInstance::Start() const
{
    if (!startFn.valid()) return;

    auto result = startFn();

    if (!result.valid()) {
        sol::error err = result;
        std::cout << "Start function error: " << err.what() << '\n';
    }
}

void ScriptInstance::Update(float deltaTime) const
{
    if (!updateFn.valid()) return;
 
    auto result = updateFn(deltaTime);

    if (!result.valid()) {
        sol::error err = result;
        std::cout << "Update function error: " << err.what() << '\n';
    }
}

void initScripting()
{
    registerLuaAPI();
    loadScripts();
}

void startScripts()
{
    for (const ScriptInstance& si : scripts) si.Start();
}

void updateScripts()
{
    for (const ScriptInstance& si : scripts) si.Update(deltaTime);
}

}
