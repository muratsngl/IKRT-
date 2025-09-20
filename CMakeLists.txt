cmake_minimum_required(VERSION 3.10)
project(IKRT)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set policy to suppress OpenGL warning
if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()

# --- Find required packages (Original Setup) ---
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED) # <-- KEPT EXACTLY AS YOU HAD IT


# --- Dear ImGui Integration ---
# 1. Define the location of the ImGui source files
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)

# 2. Create a static library for ImGui
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/ImGuiFileDialog.cpp
)

# 3. Provide include directories for ImGui
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
)

# 4. Link ImGui to its own dependencies (GLFW and OpenGL)
target_link_libraries(imgui PUBLIC
    glfw
    ${OPENGL_LIBRARIES}
)


# --- Your Application ---

# Source files (Added stb_image.cpp)
set(SOURCES
    src/main_new.cpp
    src/FABRIK.cpp
    src/shared_memory.cpp
    src/model_loader.cpp
    src/render_setup.cpp
    src/application_logic.cpp
    src/Texture.cpp
    src/collision.cpp
    src/model_bones.cpp
    src/collision_visualizer.cpp
    src/Interaction.cpp
    src/stb_image.cpp
    IO.cpp
)

# Include directories (Original Setup)
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

# Link libraries (Original Assimp linking + new ImGui library)
target_link_libraries(IKRT
    # NEW ImGui library
    imgui

    # Your original libraries
    ${OPENGL_LIBRARIES}
    glm::glm
    GLEW::GLEW
    glfw
    assimp
)