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
        if (!obj.parent) {
            obj.setWorld(obj.transform.matrix());
        } else {
            glm::mat4 parentWorld = obj.parent->getWorld();

            int boneIndex = obj.getBoneIndex();
            RigComponent* parentRig = boneIndex > -1 ? obj.parent->getComponent<RigComponent>() : nullptr;
            Rig* rig = parentRig ? &parentRig->rig : nullptr;

            if (rig && boneIndex < (int)rig->boneWorlds.size()) {

                obj.setWorld(parentWorld * rig->boneWorlds[boneIndex] * obj.transform.matrix());

            } else {
                obj.setWorld(parentWorld * obj.transform.matrix());
            }
        }

        // update cameras
        CameraComponent* cam = obj.getComponent<CameraComponent>();
        if (cam) {
            glm::mat4 w = obj.getWorld();
            cam->pos = glm::vec3(w[3]);
            cam->front = glm::normalize(-glm::vec3(w[2]));
            cam->up    = glm::normalize(glm::vec3(w[1]));
        }
    });

    Lighting::bakeSceneLighting();
    Physics::collectSceneColliders();

    if (!Audio::initAudio()) return 0;

    // upload mesh components
    forEachObj([](Object& obj) {
        MeshComponent* mesh = obj.getComponent<MeshComponent>();
        if (mesh) Render::uploadMesh(mesh);
    });

    // upload ui objects
    for (std::unique_ptr<UIElement>& ui : UI::uiRoots) ui->UploadUI();

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

        // compute poses for rigs
        forEachObj([](Object& obj) {
            RigComponent* r = obj.getComponent<RigComponent>();
            if (!r) return;
            Rig& rig = r->rig;

            rig.animTime += deltaTime * rig.animSpeed;
            if (rig.nextAnim != -1) {
                if (rig.animations[rig.currentAnim].duration - rig.animTime <= 0) // if cur anim done
                    { rig.setAnim(rig.nextAnim, 0.1f); rig.nextAnim = -1; }
            }
            if (rig.blendDuration > 0.0f) rig.blendElapsed += deltaTime;
            Animate::computePose(rig);
        });

        // compose objects
        forEachObj([](Object& obj) {
            if (!obj.parent) {
                obj.setWorld(obj.transform.matrix());
            } else {
                glm::mat4 parentWorld = obj.parent->getWorld();

                int boneIndex = obj.getBoneIndex();
                RigComponent* parentRig = boneIndex > -1 ? obj.parent->getComponent<RigComponent>() : nullptr;
                Rig* rig = parentRig ? &parentRig->rig : nullptr;

                if (rig && boneIndex < (int)rig->boneWorlds.size()) {

                    obj.setWorld(parentWorld * rig->boneWorlds[boneIndex] * obj.transform.matrix());

                } else {
                    obj.setWorld(parentWorld * obj.transform.matrix());
                }
            }

            // update cameras
            CameraComponent* cam = obj.getComponent<CameraComponent>();
            if (cam) {
                glm::mat4 w = obj.getWorld();
                cam->pos = glm::vec3(w[3]);
                cam->front = glm::normalize(-glm::vec3(w[2]));
                cam->up    = glm::normalize(glm::vec3(w[1]));
            }
        });

        // render here
        glClearColor(0.2f, 0.1f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(Render::shaderProgram);
        if (currentCam) {
            Render::configureCamera(currentCam);
            Audio::updateAudio(currentCam->pos,
                        currentCam->front,
                        currentCam->up);
        }

        // draw loop
        forEachObj([](Object& obj) {
            MeshComponent* mesh = obj.getComponent<MeshComponent>();
            if (!mesh) return;
            if (!mesh->draw) return;

            glm::mat4 w = obj.getWorld();
            glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(w)));
            glUniformMatrix4fv(Render::modelLoc, 1, GL_FALSE, glm::value_ptr(w));
            glUniformMatrix3fv(Render::normalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMat));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh->texture);
            glUniform1i(Render::texLoc, 0);

            LightMapComponent* lm = obj.getComponent<LightMapComponent>();
            if (lm) {
                glUniform1i(Render::lightModeLoc, 1);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, lm->lightMap);
                glUniform1i(Render::lightMapLoc, 1);
            } else {
                glUniform1i(Render::lightModeLoc, 0);

                std::pair<glm::vec3, glm::vec3> litAndDir = Lighting::gridLightAt(glm::vec3(w[3][0], w[3][1], w[3][2]));
                glUniform3fv(Render::objectLightLoc, 1, glm::value_ptr(litAndDir.first));
                glUniform3fv(Render::lightDirLoc,    1, glm::value_ptr(litAndDir.second));
            }

            RigComponent* r = obj.getComponent<RigComponent>();
            if (r) {
                GLsizei count = (GLsizei)std::min((size_t)Constants::MAX_BONES, r->rig.palette.size());
                glUniformMatrix4fv(Render::boneMatricesLoc, count, GL_FALSE, glm::value_ptr(r->rig.palette[0]));
            }

            glBindVertexArray(mesh->VAO);
            glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
        });

        glBindVertexArray(0);

        UI::beginUI();

        for (std::unique_ptr<UIElement>& ui : UI::uiRoots) ui->ComposeUI();
        for (std::unique_ptr<UIElement>& ui : UI::uiRoots) ui->DrawUI();

        UI::endUI();
        
        glfwSwapBuffers(Render::window);
        glfwPollEvents();
    }

    Audio::uninitAudio();
    glfwTerminate();
    return 0;
}
