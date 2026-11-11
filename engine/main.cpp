#include "include/core/audio.h"
#include "include/core/input.h"
#include "include/core/scripts.h"
#include "include/lighting/lighting.h"
#include "include/physics/collisions.h"
#include "include/render/graphics.h"
#include "include/render/window.h"
#include "include/scene/load.h"
#include "include/scene/object.h"
#include "include/ui/ui.h"

#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>


using namespace Auric;

int main()
{
    if (!glfwInit()) { std::cout << "Failed to init GLFW\n"; return -1; }

    GLFWmonitor* moniter = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(moniter);
    Render::SW = mode->width; Render::SH = mode->height;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Render::window = glfwCreateWindow(Render::SW, Render::SH, "Game Engine", moniter, NULL);
    if (!Render::window)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwSetWindowPos(Render::window, 0, 0);
    glfwMakeContextCurrent(Render::window);
    glfwSetInputMode(Render::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(Render::window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glViewport(0, 0, Render::SW, Render::SH);
    glfwSetFramebufferSizeCallback(Render::window, Render::framebuffer_size_callback);

    glEnable(GL_DEPTH_TEST);

    // build shader programs
    Render::buildShaderProgram();
    UI::buildUIProgram();

    // set callbacks
    glfwSetKeyCallback(Render::window, Input::keyCallback);
    glfwSetCursorPosCallback(Render::window, Input::mouseMoveCallback);
    glfwSetMouseButtonCallback(Render::window, Input::mouseButtonCallback);

    // load game config
    Scene::loadConfig("assets/config.json");

    // load scene
    if (!Scene::loadScene(Scene::startupScene.c_str())) return 0;
    Scripting::initScripting();

    // initial compose loop
    forEachObj([](Object& obj) {
        glm::mat4 parentWorld = obj.parent ? obj.parent->getWorld() : glm::mat4{1.0f};
        obj.setWorld(parentWorld * obj.transform.matrix());
    });

    Lighting::bakeSceneLighting();
    Physics::collectSceneColliders();
    if (!Audio::initAudio()) return 0;

    // upload meshes and ui

    Scripting::startScripts();

    while(!glfwWindowShouldClose(Render::window))
    {
        // wait for frame cap
        Render::currentFrameTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = Render::currentFrameTime - Render::lastFrameTime;

        // double time_left = target_frame_duration - elapsed.count();
        //
        // while (time_left > 0.016)
        // {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //     currentFrameTime = std::chrono::high_resolution_clock::now();
        //     elapsed = currentFrameTime - lastFrameTime;
        //     time_left = target_frame_duration - elapsed.count();
        // }
        //
        // while (time_left > 0.0)
        // {
        //     currentFrameTime = std::chrono::high_resolution_clock::now();
        //     elapsed = currentFrameTime - lastFrameTime;
        //     time_left = target_frame_duration - elapsed.count();
        // }

        // set dt
        Render::lastFrameTime = Render::currentFrameTime;
        deltaTime = (float)(elapsed.count());

        // call update fn in scripts
        Scripting::updateScripts();

        if (Scene::pendingScene != "") {
            if (!Scene::swapScene(Scene::pendingScene.c_str())) return 0;
            Scene::pendingScene = "";
        }

        if (Input::keyHeld(GLFW_KEY_LEFT_ALT) && Input::keyPressed(GLFW_KEY_F4))
            glfwSetWindowShouldClose(Render::window, true);

        Input::endFrameInput();

        // for (std::unique_ptr<Object>& obj : rootObjs) obj->ComputePose(deltaTime);
        // for (std::unique_ptr<Object>& obj : rootObjs) obj->Compose();

        // render here
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        if (currentCam) {
            configureCamera(currentCam);
            updateAudio(currentCam->getPos(),
                        currentCam->getFront(),
                        currentCam->getUp());
        }

        for (std::unique_ptr<Object>& obj : rootObjs) obj->Draw();
        glBindVertexArray(0);

        beginUI();

        for (std::unique_ptr<UIElement>& ui : uiRoots) ui->ComposeUI();
        for (std::unique_ptr<UIElement>& ui : uiRoots) ui->DrawUI();

        endUI();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    uninitAudio();
    glfwTerminate();
    return 0;
}
