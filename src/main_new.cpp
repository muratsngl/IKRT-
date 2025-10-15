#include "shared_memory.hpp"
#include "model_loader.hpp"
#include "render_setup.hpp"
#include "application_logic.hpp"
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
    //load_interactor_model("./assets/models/man_with_bones/quit.dae");

    // Initialize application state
    init_application_state();
    
    // Set wireframe mode
    glPolygonMode(GL_FRONT, GL_TRIANGLES);
    
    // Main loop
    while (!should_close_window()) {
        // Check if shared memory is available, attempt to reinitialize if not
        if (!is_shared_memory_available()) {
            static bool retry_notified = false;
            if (setup_shared_memory()) {
                std::cout << "Shared memory successfully initialized! Hand tracking is now active." << std::endl;
                retry_notified = false; // Reset notification flag
            } else if (!retry_notified) {
                std::cout << "Note: Shared memory still not available. Retrying each frame..." << std::endl;
                retry_notified = true; // Only show this message once
            }
        }
        
        // Update shared memory data
        update_shared_memory();
        // Update application logic
        update_finger_positions();
        calculate_deltas();
        apply_fabrik();
        update_transforms();
        rearrange_finger_positions_based_on_collision();
        
        
        // Render frame
        render_frame();
    }
    
    // Cleanup
    cleanup_rendering();
    if (is_shared_memory_available()) {
        cleanup_shared_memory();
    }
    
    return 0;
}
