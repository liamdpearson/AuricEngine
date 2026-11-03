#include "include/scene/object.h"
#include <fstream>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "include/render/graphics.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>                  // vec3, mat4, basic types
#include <glm/gtc/matrix_transform.hpp> // perspective, lookAt, rotate, radians
#include <glm/gtc/type_ptr.hpp>         // value_ptr (hand a matrix to OpenGL)

#include <iostream>
#include <string>
#include <memory>


// define scene variables
std::vector<std::unique_ptr<Object>> rootObjs;

// ptr to current camera
CameraComponent* currentCam;

// set scene variables
std::string startupScene = "assets/scenes/scene1.json";
std::string pendingScene = "";

// define opengl variables
GLFWwindow* window;
int SW, SH;

double target_frame_duration;
float deltaTime = 0.0f;
std::chrono::high_resolution_clock::time_point lastFrameTime = std::chrono::high_resolution_clock::now();
std::chrono::high_resolution_clock::time_point currentFrameTime = std::chrono::high_resolution_clock::now();

unsigned int shaderProgram;

int modelLoc, projectionLoc, viewLoc, normalMatLoc, boneMatricesLoc;
int texLoc, lightMapLoc, lightModeLoc, lightDirLoc, objectLightLoc;

const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aLightMapCoord;
layout (location = 4) in uint aBoneIndices;
layout (location = 5) in vec4 aBoneWeights;

const int MAX_BONES = 128;

out vec2 TexCoord;
out vec3 Normal;
out vec2 LightMapCoord;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
uniform mat3 normalMat;
uniform mat4 boneMatrices[MAX_BONES];

void main()
{
    float total = aBoneWeights.x + aBoneWeights.y + aBoneWeights.z + aBoneWeights.w;

    vec4 pos;
    mat4 skin = mat4(1.0);
    if (total > 0.0001) {
        uint b0 = (aBoneIndices >> 24) & 0xFFu;
        uint b1 = (aBoneIndices >> 16) & 0xFFu;
        uint b2 = (aBoneIndices >>  8) & 0xFFu;
        uint b3 =  aBoneIndices        & 0xFFu;

        skin = 
            aBoneWeights.x * boneMatrices[int(b0)] +
            aBoneWeights.y * boneMatrices[int(b1)] +
            aBoneWeights.z * boneMatrices[int(b2)] +
            aBoneWeights.w * boneMatrices[int(b3)];
        pos = skin * vec4(aPos, 1.0);

    } else {
        pos = vec4(aPos, 1.0);
    }

    gl_Position = projection * view * model * pos;
    TexCoord = aTexCoord;
    LightMapCoord = aLightMapCoord;
    Normal = normalMat * mat3(skin) * aNormal;
    FragPos = vec3(model * pos);
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec2 LightMapCoord;
in vec3 FragPos;

uniform sampler2D tex;
uniform sampler2D lightMap; // only used when lightMode = 1
uniform int lightMode;
uniform vec3 lightDir;    // only used when lightMode = 0
uniform vec3 objectLight; // only used when lightMode = 0

void main()
{
    vec3 lit;

    if (lightMode == 0)
    {
        vec3 n = normalize(Normal);
        vec3 L = normalize(vec3(lightDir));

        vec3 diff = max(dot(n, L), 0.0) * objectLight;

        lit = objectLight * 0.8f + diff * 0.2f;
    }
    else if (lightMode == 1)
    {
        vec4 l = texture(lightMap, LightMapCoord);
        lit = l.rgb;
    }

    vec4 t = texture(tex, TexCoord);
    FragColor = vec4(t.rgb * lit, t.a * 0.5);
}
)glsl";

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// compiles glsl shader.
int compileShader(GLenum type, const char* src)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[1024] = {};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

// links shaders to shader program.
unsigned int linkShaderProgram(const char* vsSrc, const char* fsSrc)
{
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    unsigned int sp = glCreateProgram();

    glAttachShader(sp, vs);
    glAttachShader(sp, fs);
    glLinkProgram(sp);

    int success;
    char infoLog[1024] = {};
    glGetProgramiv(sp, GL_LINK_STATUS, &success);
    if (!success)
    {
        // sp is a program name, so glGetShaderInfoLog raises GL_INVALID_OPERATION and
        // leaves infoLog untouched -- must be glGetProgramInfoLog.
        glGetProgramInfoLog(sp, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINK_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return sp;

}

static std::string loadShaderAsString(const std::string& path) {
    std::string result;
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cout << "Could not open shader file " << path << '\n';
        return "";
    }

    std::stringstream stream;
    stream << file.rdbuf();
    file.close();

    return stream.str();
}

// builds shader program and sets locations for shader variables.
void buildShaderProgram()
{
    shaderProgram = linkShaderProgram(vertexShaderSource, fragmentShaderSource);

    // vert shader uniforms
    modelLoc        = glGetUniformLocation(shaderProgram, "model");
    projectionLoc   = glGetUniformLocation(shaderProgram, "projection");
    viewLoc         = glGetUniformLocation(shaderProgram, "view");
    normalMatLoc    = glGetUniformLocation(shaderProgram, "normalMat");
    boneMatricesLoc = glGetUniformLocation(shaderProgram, "boneMatrices");

    // frag shader uniforms
    texLoc = glGetUniformLocation(shaderProgram, "tex");
    lightMapLoc = glGetUniformLocation(shaderProgram, "lightMap");
    lightModeLoc = glGetUniformLocation(shaderProgram, "lightMode");
    lightDirLoc = glGetUniformLocation(shaderProgram, "lightDir");
    objectLightLoc = glGetUniformLocation(shaderProgram, "objectLight");
}

std::pair<int, int> textureDimensions(const char* path)
{
    int tw = 0, th = 0, tch = 0;
    if (!stbi_info(path, &tw, &th, &tch)) {
        std::cout << "Failed to read texture dimensions of: " << path << '\n';
        return {0, 0};
    }
    return {tw, th};
}

// uses stb to image files as textures.
unsigned int loadTexture(const char* src, bool pixelated, bool clampEdge)
{
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLint wrap = clampEdge ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    clampEdge ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    pixelated ? GL_NEAREST : GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // pngs are top down gl expects bottom up
    unsigned char* data = stbi_load(src, &width, &height, &nrChannels, 0);
    if (data)
    {   
        GLenum format = nrChannels == 4 ? GL_RGBA : nrChannels == 3 ? GL_RGB : GL_RED;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture: " << src << std::endl;
    }

    stbi_image_free(data);
    return texture;
}

// converts light map pixel rgb values to a texture.
unsigned int loadLightMap(std::vector<glm::vec3>& pixels, int width, int height)
{
    unsigned int lightMap;
    glGenTextures(1, &lightMap);
    glBindTexture(GL_TEXTURE_2D, lightMap);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_FLOAT, pixels.data());

    return lightMap;
}

// creates proj and view matrices for the camera and sends them to shader locations.
// takes in cam as an argument so a scene can have multiple cameras.
void configureCamera(Camera*& cam)
{
    int frameBuffWidth, frameBuffHeight;
    glfwGetFramebufferSize(window, &frameBuffWidth, &frameBuffHeight);

    glm::mat4 projection = glm::perspective(
        glm::radians(cam->FOV),                          // fov
        (float)frameBuffWidth / (float)frameBuffHeight,  // window aspect ratio
        0.01f, 100.0f                                    // near and far clip dist
    );

    glm::mat4 view = glm::lookAt(
        cam->getPos(),
        cam->getPos() + cam->getFront(),
        cam->getUp()
    );

    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc,       1, GL_FALSE, glm::value_ptr(view));
}
