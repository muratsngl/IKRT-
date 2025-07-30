# Linux Conversion Summary

## Changes Made to Convert from Windows GLAD to Linux GLEW

### 1. **Header File Changes**
- Replaced `#include <glad/glad.h>` with `#include <GL/glew.h>` in:
  - `main.cpp`
  - `Shader.h`
  - `Camera.h` 
  - `Mesh.h`
  - `model_bones.h`

### 2. **OpenGL Initialization**
- Changed `gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)` to `glewInit()`
- Updated error message from "Failed to initialize GLAD" to "Failed to initialize GLEW"

### 3. **Windows to Linux Shared Memory Conversion**
**Added Linux includes:**
```cpp
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
```

**Replaced Windows shared memory setup:**
```cpp
// OLD Windows code:
HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, sw);
void* pBuf = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 62);

// NEW Linux code:
int shm_fd = shm_open(shm_name.c_str(), O_RDONLY, 0666);
void* pBuf = mmap(nullptr, 62, PROT_READ, MAP_SHARED, shm_fd, 0);
```

**Replaced cleanup code:**
```cpp
// OLD Windows code:
UnmapViewOfFile(pBuf);
CloseHandle(hMapFile);

// NEW Linux code:
munmap(pBuf, 62);
close(shm_fd);
```

### 4. **Build System**
- Created `CMakeLists.txt` with proper Linux dependencies:
  - OpenGL
  - GLEW (instead of GLAD)
  - GLFW3
  - GLM
  - Assimp

### 5. **Minor Fixes**
- Removed `#include <Windows.h>` 
- Fixed `#include "glm/glm.hpp">` to `#include "glm/glm.hpp"` in FABRIK.h
- Added missing `std::` prefix for string declarations
- Created build script `build.sh`
- Created `README.md` with Linux build instructions

### 6. **Files Created**
- `CMakeLists.txt` - CMake build configuration
- `build.sh` - Build script for easy compilation
- `README.md` - Documentation with dependencies and build instructions

## Build Instructions

### Install Dependencies (Ubuntu/Debian):
```bash
sudo apt-get install libglew-dev libglfw3-dev libglm-dev libassimp-dev
```

### Build:
```bash
./build.sh
```

Or manually:
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The project now compiles successfully on Linux using standard system libraries!
