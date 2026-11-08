#include "include/render/window.h"

namespace Auric::Render
{

// define opengl variables
GLFWwindow* window;
int SW, SH;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

}
