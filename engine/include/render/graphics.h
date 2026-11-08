#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>                  // vec3, mat4, basic types
#include <glm/gtc/matrix_transform.hpp> // perspective, lookAt, rotate, radians
#include <glm/gtc/type_ptr.hpp>         // value_ptr (hand a matrix to OpenGL)
#include <glm/gtc/quaternion.hpp>       // quat slerp, mat4_cast

#include <xatlas/xatlas.h>

#include <chrono>

#include "include/scene/object.h"


// draws from this camera
extern CameraComponent* currentCam;
extern float deltaTime;

namespace Auric::Render
{

extern double target_frame_duration;
extern std::chrono::high_resolution_clock::time_point lastFrameTime, currentFrameTime;

extern unsigned int shaderProgram;

extern int modelLoc, projectionLoc, viewLoc, normalMatLoc, boneMatricesLoc;
extern int texLoc, lightMapLoc, lightModeLoc, lightDirLoc, objectLightLoc;

extern const char* vertexShaderSource;
extern const char* fragmentShaderSource;

int compileShader(GLenum type, const char* src);

unsigned int linkShaderProgram(const char* vsSrc, const char* fsSrc);

void buildShaderProgram();

std::pair<int, int> textureDimensions(const char* path);

unsigned int loadTexture(const char* src, bool pixelated, bool clampEdge = false);

unsigned int loadLightMap(std::vector<glm::vec3>& pixels, int width, int height);

}
