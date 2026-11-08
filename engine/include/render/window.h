#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace Auric::Render
{

// define opengl variables
extern GLFWwindow* window;
extern int SW, SH;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

}
