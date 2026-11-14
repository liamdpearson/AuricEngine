# AuricEngine

This project is still in its early stages of development. I doubt it will ever be finished. It is mainly just for fun because I like the look of old school graphics.


## Third Party Libraries
- OpenGL 3.3 with GLFW and GLAD
- glm for linear algebra types like vec3 and mat4
- sol2 with for scripting in lua
- nlohmann json for parsing scene files
- miniaudio for audio
- stb for image loading
- ufbx for .fbx loading
- xatlas for generating light map UV


## Features
- Object hierarchy using 4x4 world matrices
- Component based system with components: Mesh, Camera, Audiosource, Script, etc
- UI hierarchy with types: UIImage and UIText as children of UIElement
- Basic collisions and raycasting for capsules
- BVH for raycasting against static meshes
- Baked light maps for static meshes with point and line lights
- Light grid for dynamic meshes containing light value and dominant light direction
- Mesh support from .obj and .fbx files
- Armature and animation support from .fbx files
- Bone parenting support for children of rigged objects
- Ability to have multiple cameras and draw from whichever you want
- Scripting with lua exposing 7 engine systems (animation, physics, audio, input, UI, camera, object transforms)
- 2D sound with volume and loop control
- 3D sound with volume, falloff, and loop control


## Code Structure
```text
engine/
├── include/
│   ├── core/
│   │   ├── animation.h
│   │   ├── audio.h
│   │   ├── constants.h
│   │   ├── input.h
│   │   └── scripts.h
│   ├── lighting/
│   │   ├── lighting.h
│   │   └── lightmap.h
│   ├── physics/
│   │   ├── collisions.h
│   │   ├── raycast.h
│   │   └── types.h
│   ├── render/
│   │   ├── graphics.h
│   │   └── window.h
│   ├── scene/
│   │   ├── assets.h
│   │   ├── load.h
│   │   ├── mesh.h
│   │   └── object.h
│   └── ui/
│       └── ui.h
├── src/
│   ├── core/
│   │   ├── animation.cpp
│   │   ├── audio.cpp
│   │   ├── audio_internal.h
│   │   ├── input.cpp
│   │   ├── scripts.cpp
│   │   └── scripts_internal.h
│   ├── lighting/
│   │   ├── lighting.cpp
│   │   └── lightmap.cpp
│   ├── physics/
│   │   ├── collisions.cpp
│   │   └── raycast.cpp
│   ├── render/
│   │   ├── graphics.cpp
│   │   └── window.cpp
│   ├── scene/
│   │   ├── assets.cpp
│   │   ├── load.cpp
│   │   ├── mesh.cpp
│   │   └── object.cpp
│   └── ui/
│       └── ui.cpp
└── main.cpp
```


## TO DO:
- Add a decal system
- Add a particle system
- Add editor with ImGui


## How to run the test scene yourself
### Windows
Prerequisites: MinGW/GCC, git, and CMake 3.16 or newer.
1. git clone https://github.com/liamdpearson/AuricEngine
2. cd AuricEngine
3. cmake -S . -B build -G "MinGW Makefiles"
4. cmake --build build
5. ./release

### Linux
Prerequisites: GCC, git, and CMake 3.16 or newer.
1. git clone https://github.com/liamdpearson/AuricEngine
2. cd AuricEngine
3. cmake -S . -B build
4. cmake --build build
5. ./release
