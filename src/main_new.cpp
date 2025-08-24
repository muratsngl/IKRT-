#include "shared_memory.hpp"
#include "model_loader.hpp"
#include "render_setup.hpp"
#include "application_logic.hpp"
#include <iostream>

int main() {
    // Initialize all systems
    if (!setup_shared_memory()) {
        std::cerr << "Failed to setup shared memory" << std::endl;
        return -1;
    }
    
    if (!init_rendering()) {
        std::cerr << "Failed to initialize rendering" << std::endl;
        cleanup_shared_memory();
        return -1;
    }
    
    // Load shaders and initialize buffers
    load_shaders();
    init_buffers();
    
    //Load the model
    if (!load_interactor_model("assets/models/man_with_bones/MAN_WITH_CORRECT_BONES_AND_SYSTEM_REALLY.dae")) {
        std::cerr << "Failed to load model" << std::endl;
        cleanup_rendering();
        cleanup_shared_memory();
        return -1;
    }
    // if(load_scene_element_model("assets/models/watermelon/scene.gltf") == false){
    //     std::cerr << "Failed to load scene element model" << std::endl;
    //     cleanup_rendering();
    //     cleanup_shared_memory();
    //     return -1;
    // } //commeted out to flip the scene until the scenelementdata is being implemented
    
    
    if(load_scene_element_model("assets/models/mug/mug.dae")==false){
        std::cerr << "Failed to load scene element model" << std::endl;
        cleanup_rendering();
        cleanup_shared_memory();
        return -1;
    }



    // Initialize application state
    init_application_state();
    
    // Set wireframe mode
    glPolygonMode(GL_FRONT, GL_TRIANGLES);
    
    // Main loop
    while (!should_close_window()) {
        // Update shared memory data
        update_shared_memory();
        // Update application logic
        update_finger_positions();
        calculate_deltas();
        apply_fabrik();
        rearrange_finger_positions_based_on_collision();
        apply_fabrik();
        update_transforms();
        
        // Render frame
        render_frame();
    }
    
    // Cleanup
    cleanup_rendering();
    cleanup_shared_memory();
    
    return 0;
}
