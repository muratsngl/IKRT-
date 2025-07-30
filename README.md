# IKRT - Inverse Kinematics Project

This project has been converted from Windows/GLAD to use Linux/GLEW.

## Dependencies

Make sure you have the following packages installed:

### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake
sudo apt-get install libglew-dev libglfw3-dev libglm-dev libassimp-dev
```

### Fedora/RHEL:
```bash
sudo dnf install gcc-c++ cmake
sudo dnf install glew-devel glfw-devel glm-devel assimp-devel
```

### Arch Linux:
```bash
sudo pacman -S base-devel cmake
sudo pacman -S glew glfw glm assimp
```

## Building

1. Clone the repository and navigate to the project directory:
```bash
cd IKRT-
```

2. Run the build script:
```bash
./build.sh
```

Or manually:
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

3. Run the executable:
```bash
./build/IKRT
```

## Changes Made

- Replaced `#include <glad/glad.h>` with `#include <GL/glew.h>` in all source files
- Replaced `gladLoadGLLoader()` with `glewInit()` in main.cpp
- Removed Windows-specific `#include <Windows.h>` and replaced with Linux-compatible includes
- Created CMakeLists.txt for building with CMake
- Removed glad.c file (no longer needed with GLEW)
