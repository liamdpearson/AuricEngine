#include "include/scene/load.h"

#include <algorithm>
#include <memory>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>
#include <iostream>
#include <vector>

#include "include/core/constants.h"
#include "include/core/lighting.h"
#include "include/scene/object.h"
#include "include/assets/assets.h"
#include "include/ui/ui.h"


using json = nlohmann::json;

static void buildComponent(const json& node, Object* owner)
{
    const unsigned int typeID = node.value("ID", 0);

    switch (typeID) {
        case Auric::Constants::MESHID: {
            const std::string meshpath = node.value("meshpath", "");
            const std::string texpath = node.value("texpath", "");
            const bool pixelated = node.value("pixelated", false); 

            std::pair<std::unique_ptr<MeshComponent>, std::unique_ptr<RigComponent>> pair;
            pair = Auric::Assets::makeMesh(meshpath.c_str(), texpath.c_str(), pixelated);
            
            owner->addComponent(std::move(pair.first));
            
            if (pair.second != nullptr)
                owner->addComponent(std::move(pair.second));
            
            break;
        }

        case Auric::Constants::CAMERAID: {
            const float FOV = node.value("FOV", 90.0f);

            std::unique_ptr<CameraComponent> cam = Auric::Assets::makeCamera(FOV);
            owner->addComponent(std::move(cam));

            break;
        }

        case Auric::Constants::RIGID: {
            const std::string path = node.value("path", "");

            std::unique_ptr<RigComponent> rig = Auric::Assets::makeRig(path.c_str());
            owner->addComponent(std::move(rig));

            break;
        }

        case Auric::Constants::LIGHTMAPID: {
            std::unique_ptr<LightMapComponent> lm = Auric::Assets::makeLightMap();
            owner->addComponent(std::move(lm));

            break;
        }

        case Auric::Constants::COLLIDERID: {
            std::unique_ptr<ColliderComponent> col = Auric::Assets::makeCollider();
            owner->addComponent(std::move(col));

            break;
        }

        case Auric::Constants::CAPSULECOLLIDERID: {
            const float radius = node.value("radius", 0.3f);
            const float height = node.value("height", 1.8f);

            std::unique_ptr<CapsuleColliderComponent> cap;
            cap = Auric::Assets::makeCapsuleCollider(radius, height);
            owner->addComponent(std::move(cap));

            break;
        }

        case Auric::Constants::AUDIOSOURCEID: {
            const std::string path = node.value("path", "");
            const float volume = node.value("volume", 0.0f);
            const float maxDistance = node.value("maxDistance", 0.0f);
            const float falloff = node.value("falloff", 0.0f);
            const bool loop = node.value("loop", false); 

            std::unique_ptr<AudioSourceComponent> audiosource;
            audiosource = Auric::Assets::makeAudioSource(path.c_str(), volume, maxDistance, falloff, loop);
            owner->addComponent(std::move(audiosource));

            break;
        }

        case Auric::Constants::SCRIPTID: {
            const std::string path = node.value("path", "");

            std::unique_ptr<ScriptComponent> script;
            script = Auric::Assets::makeScript(path);
            owner->addComponent(std::move(script));

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
    
    std::unique_ptr<Object> obj;
    obj->setName(name); obj->setTag(tag); obj->transform = transform;

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

    UITransform transform{t.at(0).get<float>() * SW, t.at(1).get<float>() * SH,
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

        ambient = scene.value("ambient", 0.0f);
        lightmapResScalar = scene.value("lightmapResScalar", 0.01f);

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
            Font f = bakeFont(
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
            attachScript(nullptr, path);

        rootObjs = std::move(tempObjs);
        lights = tempLights;
        for (Font& f : fonts) glDeleteTextures(1, &f.atlas);
        fonts = tempFonts;
        uiRoots = std::move(tempUI);
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
    uiRoots.clear();
    resetScripting();
    rootObjs.clear();
    uninitSounds();
    currentCam = nullptr;
    ambient = 0.0f;
    lightmapResScalar = 0.01;
    for (Light* l : lights) delete l;
    lights.clear();

    if (!loadScene(path)) return 0;
    loadScripts();

    // this clears occluders and light grid
    bakeSceneLighting();

    // this clears colliders
    collectSceneColliders();

    for (std::unique_ptr<Object>& obj : rootObjs) obj->Upload();
    for (std::unique_ptr<UIElement>& ui : uiRoots) ui->UploadUI();

    startScripts();

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

        target_frame_duration = 1.0/scene.value("MAX_FPS", 60.0);
    }
    catch(const json::exception& e)
    {
        std::cout << "loadScene: " << path << ": " << e.what() << '\n';
    }

    return 1;
}
