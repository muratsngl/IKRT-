cmake_minimum_required(VERSION 3.10)
project(IKRT)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set policy to suppress OpenGL warning
if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)

# Source files
set(SOURCES
    src/main_new.cpp
    src/FABRIK.cpp
    src/shared_memory.cpp
    src/model_loader.cpp
    src/render_setup.cpp
    src/application_logic.cpp
    src/Texture.cpp
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${GLM_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    vendor/assimp/include
    vendor/stb_image
)

# Create executable
add_executable(IKRT ${SOURCES})

# Set the runtime working directory to the project root
set_target_properties(IKRT PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(IKRT
    ${OPENGL_LIBRARIES}
    glm::glm
    GLEW::GLEW
    glfw
    assimp
)
