#include "shared_memory.hpp"
#include "model_loader.hpp"
#include "render_setup.hpp"
#include "application_logic.hpp"
#include "collision_visualizer.hpp"
#include <iostream>


int main() {
    // Initialize all systems
    if (!setup_shared_memory()) {
        std::cout << "Warning: Shared memory not available. Hand tracking disabled." << std::endl;
        std::cout << "The application will continue without hand input." << std::endl;
    }
    
    if (!init_rendering()) {
        std::cerr << "Failed to initialize rendering" << std::endl;
        if (is_shared_memory_available()) {
            cleanup_shared_memory();
        }
        return -1;
    }
    
    // Load shaders and initialize buffers
    load_shaders();
    init_buffers();
    create_shadow_maps();
    //load_interactor_model("./assets/models/man_with_bones/quit.dae");

    // Initialize application state
    init_application_state();
    
    // Set wireframe mode
    glPolygonMode(GL_FRONT, GL_TRIANGLES);
    
    // Main loop
    while (!should_close_window()) {
        // Update timing
        update_finger_positions();
        
        // User will implement IK/FK logic
        apply_fabrik();
        update_transforms();
        
        // Update bone boxes every frame for selection (independent of visualization)
        if (is_interactor_model_available()) {
            update_bone_boxes();
            update_target_proxy_boxes();  // Update target proxy boxes for IK chain targets
        }
        
        
        // Render frame
        //TODO DECOUPLE ANIMATION AND PHYSICS ROUTINES FROM THE RENDER FUNCTIONS
        render_frame();
    }
    
    // Cleanup
    cleanup_rendering();
    if (is_shared_memory_available()) {
        cleanup_shared_memory();
    }
    
    return 0;
}
