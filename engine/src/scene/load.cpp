#include "include/scene/load.h"

#include <memory>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <memory>

#include "include/core/constants.h"
#include "include/core/audio.h"
#include "include/core/scripts.h"
#include "include/lighting/lighting.h"
#include "include/lighting/lightmap.h"
#include "include/physics/collisions.h"
#include "include/render/graphics.h"
#include "include/render/window.h"
#include "include/scene/object.h"
#include "include/scene/assets.h"
#include "include/ui/ui.h"


using json = nlohmann::json;

using namespace Auric;

static void buildComponent(const json& node, Object* owner)
{
    const unsigned int typeID = node.value("ID", 0);

    switch (typeID) {
        case Constants::MESHID: {
            const std::string meshpath = node.value("meshpath", "");
            const std::string texpath = node.value("texpath", "");
            const bool pixelated = node.value("pixelated", false); 
            const bool draw = node.value("draw", true); 

            std::pair<std::unique_ptr<MeshComponent>, std::unique_ptr<RigComponent>> pair;
            pair = Assets::makeMesh(meshpath.c_str(), texpath.c_str(), pixelated, draw);
            
            owner->addComponent(std::move(pair.first));
            
            if (pair.second)
                owner->addComponent(std::move(pair.second));
            
            break;
        }

        case Constants::CAMERAID: {
            const float FOV = node.value("FOV", 90.0f);

            std::unique_ptr<CameraComponent> cam = Assets::makeCamera(FOV);
            if (node.value("isCurrent", false)) currentCam = cam.get();
            owner->addComponent(std::move(cam));

            break;
        }

        case Constants::RIGID: {
            const std::string path = node.value("path", "");

            std::unique_ptr<RigComponent> rig = Assets::makeRig(path.c_str());
            owner->addComponent(std::move(rig));

            break;
        }

        case Constants::LIGHTMAPID: {
            std::unique_ptr<LightMapComponent> lm = Assets::makeLightMap();
            owner->addComponent(std::move(lm));

            break;
        }

        case Constants::COLLIDERID: {
            std::unique_ptr<ColliderComponent> col = Assets::makeCollider();
            owner->addComponent(std::move(col));

            break;
        }

        case Constants::CAPSULECOLLIDERID: {
            const float radius = node.value("radius", 0.3f);
            const float height = node.value("height", 1.8f);

            std::unique_ptr<CapsuleColliderComponent> cap;
            cap = Assets::makeCapsuleCollider(radius, height);
            owner->addComponent(std::move(cap));

            break;
        }

        case Constants::AUDIOSOURCEID: {
            const std::string path = node.value("path", "");
            const float volume = node.value("volume", 0.0f);
            const float maxDistance = node.value("maxDistance", 0.0f);
            const float falloff = node.value("falloff", 0.0f);
            const bool loop = node.value("loop", false); 

            std::unique_ptr<AudioSourceComponent> audiosource;
            audiosource = Assets::makeAudioSource(path.c_str(), volume, maxDistance, falloff, loop);
            owner->addComponent(std::move(audiosource));

            break;
        }

        case Constants::SCRIPTID: {
            const std::string path = node.value("path", "");

            std::unique_ptr<ScriptComponent> script;
            script = Assets::makeScript(path);
            owner->addComponent(std::move(script));
            Scripting::attachScript(owner, path);

            break;
        }
    }
}

static std::unique_ptr<Object> buildObject(const json& node, Object* parent)
{
    const std::string name = node.value("name", "Unnamed Object");
    const std::string tag  = node.value("tag", "");

    const json& t = node.at("transform");

    Transform transform{t.at(0).get<float>(), t.at(1).get<float>(), t.at(2).get<float>(),
                        t.at(3).get<float>(), t.at(4).get<float>(), t.at(5).get<float>(),
                        t.at(6).get<float>(), t.at(7).get<float>(), t.at(8).get<float>()};
    
    std::unique_ptr<Object> obj = std::make_unique<Object>();
    obj->setName(name); 
    obj->setTag(tag); 
    obj->transform = transform;

    for (const json& comp: node.value("components", json::array()))
        buildComponent(comp, obj.get());


    const std::string pbn = node.value("parentbonename", "");
    int pbi = -1;
    if (parent && pbn != "") {
        RigComponent* rig = parent->getComponent<RigComponent>();
        if (rig) {
            pbi = rig->rig.findBoneIndex(pbn); 
        }
    }

    obj->setBoneIndex(pbi);
    if (parent) obj->parent = parent;

    for (const json& child : node.value("children", json::array()))
    {
        auto c = buildObject(child, obj.get());
        if (c) obj->addChild(std::move(c));
    }

    return obj;
}

static Light* buildLight(const json& node)
{
    std::string type = node.value("type", "point"); // defauts to point light

    std::string name = node.value("name", "Unnamed Light");
    std::string tag  = node.value("tag", "");

    const json& c = node.at("color");
    glm::vec3 color = glm::vec3{c.at(0).get<float>(), c.at(1).get<float>(), c.at(2).get<float>()};
    float intensity = node.at("intensity").get<float>();
    float radius    = node.at("radius").get<float>();
    float falloff   = node.at("falloff").get<float>();

    if (type == "point") {
        PointLight* light = new PointLight{};

        const json& p = node.at("pos");
        light->pos   = glm::vec3{p.at(0).get<float>(), p.at(1).get<float>(), p.at(2).get<float>()};

        light->name = name;
        light->tag = tag;

        light->color = color;
        light->intensity = intensity;
        light->radius = radius;
        light->falloff = falloff;
        
        return light;
    }
    else if (type == "line") {
        LineLight* light = new LineLight{};

        const json& start = node.at("start");
        const json& end = node.at("end");

        light->start = glm::vec3{start.at(0).get<float>(), start.at(1).get<float>(), start.at(2).get<float>()};
        light->end = glm::vec3{end.at(0).get<float>(), end.at(1).get<float>(), end.at(2).get<float>()};

        light->segments = node.value("segments", 1);

        light->name = name;
        light->tag = tag;

        light->color = color;
        light->intensity = intensity;
        light->radius = radius;
        light->falloff = falloff;
        
        return light;
    }
    
    return nullptr;
}

static std::unique_ptr<UIElement> buildUIElement(const json& node, UIElement* parent)
{
    const std::string name = node.value("name", "Unnamed Object");
    const std::string type = node.value("type", "object");
    const std::string tag  = node.value("tag", "");
    const json& t = node.at("uitransform");

    UITransform transform{t.at(0).get<float>() * Render::SW, t.at(1).get<float>() * Render::SH,
                          t.at(2).get<float>(), t.at(3).get<float>(), t.at(4).get<float>()};

    std::unique_ptr<UIElement> ui;
    
    if (type == "uielement")
    {
        ui = std::make_unique<UIElement>(transform);
    }
    else if (type == "uiimage")
    {
        ui = std::make_unique<UIImage>(
            transform, node.at("imagepath").get<std::string>().c_str()
        );
    }
    else if (type == "uitext")
    {
        const json& c = node.at("color");

        ui = std::make_unique<UIText>(
            transform, node.at("text").get<std::string>().c_str(),
            node.value("size", 32.0f), node.at("fontindex").get<int>(),
            glm::vec3{c.at(0).get<float>(), c.at(1).get<float>(), c.at(2).get<float>()},
            (unsigned char)node.at("anchorX").get<std::string>()[0],
            (unsigned char)node.at("anchorY").get<std::string>()[0]
        );
    }
    else
    {
        std::cout << "buildUI: Error unrecognized ui type " << type << '\n';
        return nullptr;
    }

    ui->setName(name); ui->setTag(tag);
    if (parent) ui->parent = parent;

    for (const json& child : node.value("children", json::array()))
    {
        auto c = buildUIElement(child, ui.get());
        if (c) ui->addChild(std::move(c));
    }

    return ui;
}

namespace Auric::Scene
{

// set scene variables
std::string startupScene = "assets/scenes/scene1.json";
std::string pendingScene = "";


int loadScene(const char* path)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cout << "loadScene: Could not find scene: " << path << '\n';
        return 0;
    }

    std::vector<std::unique_ptr<Object>> tempObjs;
    std::vector<Light*> tempLights;
    std::vector<Font> tempFonts;
    std::vector<std::unique_ptr<UIElement>> tempUI;

    try
    {
        json scene;
        file >> scene;

        Lighting::ambient = scene.value("ambient", 0.0f);
        Lighting::lightmapResScalar = scene.value("lightmapResScalar", 0.01f);

        for (const json& node : scene.value("objects", json::array()))
        {
            auto obj = buildObject(node, nullptr);
            if (obj) tempObjs.push_back(std::move(obj));
        }
        for (const json& node : scene.value("lights", json::array()))
        {
            Light* l = buildLight(node);
            if (l) tempLights.push_back(l);
        }
        for (const json& node : scene.value("fonts", json::array()))
        {
            Font f = UI::bakeFont(
                node.at("ttfpath").get<std::string>().c_str(),
                node.value("pixelheight", 32.0f)
            );
            tempFonts.push_back(f);
        }
        for (const json& node : scene.value("ui", json::array()))
        {
            auto ui = buildUIElement(node, nullptr);
            if (ui) tempUI.push_back(std::move(ui));
        }
        // global scripts operate the same way as regular scripts just with no self object
        for (const std::string& path : scene.value("globalscripts", json::array()))
            Scripting::attachScript(nullptr, path);

        rootObjs = std::move(tempObjs);
        Lighting::lights = tempLights;
        for (Font& f : UI::fonts) glDeleteTextures(1, &f.atlas);
        UI::fonts = tempFonts;
        UI::uiRoots = std::move(tempUI);
    }
    catch(const json::exception& e)
    {
        std::cout << "loadScene: " << path << ": " << e.what() << '\n';
        for (Light* l : tempLights) delete l;
        tempLights.clear();
        return 0;
    }

    return 1;
}

int swapScene(const char* path)
{
    UI::uiRoots.clear();
    Scripting::resetScripting();
    rootObjs.clear();
    Audio::uninitSounds();
    currentCam = nullptr;
    Lighting::ambient = 0.0f;
    Lighting::lightmapResScalar = 0.01;
    for (Light* l : Lighting::lights) delete l;
    Lighting::lights.clear();

    if (!loadScene(path)) return 0;
    Scripting::loadScripts();

    // this clears occluders and light grid
    Lighting::bakeSceneLighting();

    // this clears colliders
    Physics::collectSceneColliders();

    // for (std::unique_ptr<Object>& obj : rootObjs) obj->Upload();
    // for (std::unique_ptr<UIElement>& ui : uiRoots) ui->UploadUI();

    Scripting::startScripts();

    return 1;
}

int loadConfig(const char* path)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cout << "loadScene: Could not find scene: " << path << '\n';
        return 0;
    }

    try
    {
        json scene;
        file >> scene;

        bool vsync = scene.value("vsync", false);
        std::cout << '\n' << vsync << "\n\n";
        if (vsync) glfwSwapInterval(1);
        else glfwSwapInterval(0);

        Render::target_frame_duration = 1.0/scene.value("MAX_FPS", 60.0);
    }
    catch(const json::exception& e)
    {
        std::cout << "loadScene: " << path << ": " << e.what() << '\n';
    }

    return 1;
}

}
