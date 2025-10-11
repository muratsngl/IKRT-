#include "include/raycast.hpp"
#include "include/render_setup.hpp"
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <limits>
#include <iostream>

Ray generate_ray(float mouse_x, float mouse_y, 
                 const glm::mat4& view, 
                 const glm::mat4& projection, 
                 const glm::vec3& camera_pos,
                 int screen_width, 
                 int screen_height) {
    
    // Convert mouse coordinates to normalized device coordinates (NDC)
    // Screen coordinates: (0,0) is top-left, (width, height) is bottom-right
    // NDC: (-1,-1) is bottom-left, (1,1) is top-right
    float x_ndc = (2.0f * mouse_x) / screen_width - 1.0f;
    float y_ndc = 1.0f - (2.0f * mouse_y) / screen_height;  // Flip Y axis
    
    // Create NDC point at near and far planes
    glm::vec4 near_point_ndc(x_ndc, y_ndc, -1.0f, 1.0f);  // Near plane (z = -1 in NDC)
    glm::vec4 far_point_ndc(x_ndc, y_ndc, 1.0f, 1.0f);    // Far plane (z = 1 in NDC)
    
    // Calculate inverse matrices
    glm::mat4 inverse_projection = glm::inverse(projection);
    glm::mat4 inverse_view = glm::inverse(view);
    
    // Transform to view space
    glm::vec4 near_point_view = inverse_projection * near_point_ndc;
    glm::vec4 far_point_view = inverse_projection * far_point_ndc;
    
    // Perspective divide
    near_point_view /= near_point_view.w;
    far_point_view /= far_point_view.w;
    
    // Transform to world space
    glm::vec4 near_point_world = inverse_view * near_point_view;
    glm::vec4 far_point_world = inverse_view * far_point_view;
    
    // Create ray
    Ray ray;
    ray.origin = camera_pos;  // Use camera position as ray origin
    
    // Calculate direction from camera to the unprojected point
    glm::vec3 world_point = glm::vec3(near_point_world);
    ray.direction = glm::normalize(world_point - camera_pos);
    
    return ray;
}

bool ray_aabb_intersect(const Ray& ray, const Aabb& aabb, float& t_min, float& t_max) {
    // Ray-AABB intersection using the slab method
    glm::vec3 inv_dir = 1.0f / ray.direction;
    
    // Calculate t values for each axis
    glm::vec3 t1 = (aabb.min - ray.origin) * inv_dir;
    glm::vec3 t2 = (aabb.max - ray.origin) * inv_dir;
    
    // Ensure t1 contains the smaller values
    glm::vec3 t_near = glm::min(t1, t2);
    glm::vec3 t_far = glm::max(t1, t2);
    
    // Find the largest t_near and smallest t_far
    t_min = std::max({t_near.x, t_near.y, t_near.z});
    t_max = std::min({t_far.x, t_far.y, t_far.z});
    
    // Check for intersection
    // Ray intersects AABB if t_min <= t_max and t_max >= 0
    return (t_min <= t_max) && (t_max >= 0.0f);
}

int intersect_ray(const Ray& ray, const std::vector<Shape>& shapes) {
    float closest_distance = std::numeric_limits<float>::max();
    int closest_model_id = -1;
    
    // Get access to model matrices for transformation to local space
    const std::vector<glm::mat4>& model_matrices = get_model_matrices();
    
    for (const Shape& shape : shapes) {
        // Currently only handle AABB shapes
        if (shape.type != AABB) {
            continue;
        }
        
        // Get the model matrix for this shape using the existing ID-to-index mapping
        unsigned int model_index = get_model_index_by_id(shape.id);
        
        // Make sure the model index is valid
        if (model_index >= model_matrices.size()) {
            std::cerr << "Warning: Model index " << model_index << " out of bounds for model ID " << shape.id << std::endl;
            continue;
        }
        
        // Get the model matrix and calculate its inverse
        const glm::mat4& model_matrix = model_matrices[model_index];
        glm::mat4 inverse_model_matrix = glm::inverse(model_matrix);
        
        // Transform ray to local space
        Ray local_ray;
        
        // Transform ray origin to local space
        glm::vec4 local_origin = inverse_model_matrix * glm::vec4(ray.origin, 1.0f);
        local_ray.origin = glm::vec3(local_origin);
        
        // Transform ray direction to local space (use w=0 for directions)
        glm::vec4 local_direction = inverse_model_matrix * glm::vec4(ray.direction, 0.0f);
        local_ray.direction = glm::normalize(glm::vec3(local_direction));
        
        // Perform intersection test in local space
        float t_min, t_max;
        if (ray_aabb_intersect(local_ray, shape.aabb, t_min, t_max)) {
            // Transform intersection distance back to world space
            // We need to account for the scaling in the model matrix
            glm::vec3 world_intersection_point = ray.origin + ray.direction * t_min;
            float world_distance = glm::length(world_intersection_point - ray.origin);
            
            if (t_min >= 0.0f && world_distance < closest_distance) {
                closest_distance = world_distance;
                closest_model_id = shape.id;
            }
        }
    }
    
    return closest_model_id;
}

std::vector<int> intersect_ray_all(const Ray& ray, const std::vector<Shape>& shapes) {
    std::vector<std::pair<float, int>> hit_objects; // pair of (distance, model_id)
    
    // Get access to model matrices for transformation to local space
    const std::vector<glm::mat4>& model_matrices = get_model_matrices();
    
    for (const Shape& shape : shapes) {
        // Currently only handle AABB shapes
        if (shape.type != AABB) {
            continue;
        }
        
        // Get the model matrix for this shape using the existing ID-to-index mapping
        unsigned int model_index = get_model_index_by_id(shape.id);
        
        // Make sure the model index is valid
        if (model_index >= model_matrices.size()) {
            std::cerr << "Warning: Model index " << model_index << " out of bounds for model ID " << shape.id << std::endl;
            continue;
        }
        
        // Get the model matrix and calculate its inverse
        const glm::mat4& model_matrix = model_matrices[model_index];
        glm::mat4 inverse_model_matrix = glm::inverse(model_matrix);
        
        // Transform ray to local space
        Ray local_ray;
        
        // Transform ray origin to local space
        glm::vec4 local_origin = inverse_model_matrix * glm::vec4(ray.origin, 1.0f);
        local_ray.origin = glm::vec3(local_origin);
        
        // Transform ray direction to local space (use w=0 for directions)
        glm::vec4 local_direction = inverse_model_matrix * glm::vec4(ray.direction, 0.0f);
        local_ray.direction = glm::normalize(glm::vec3(local_direction));
        
        // Perform intersection test in local space
        float t_min, t_max;
        if (ray_aabb_intersect(local_ray, shape.aabb, t_min, t_max)) {
            // Transform intersection distance back to world space
            if (t_min >= 0.0f) {
                glm::vec3 world_intersection_point = ray.origin + ray.direction * t_min;
                float world_distance = glm::length(world_intersection_point - ray.origin);
                hit_objects.push_back(std::make_pair(world_distance, shape.id));
            }
        }
    }
    
    // Sort by distance (nearest first)
    std::sort(hit_objects.begin(), hit_objects.end());
    
    // Extract just the model IDs in sorted order
    std::vector<int> sorted_model_ids;
    for (const auto& hit : hit_objects) {
        sorted_model_ids.push_back(hit.second);
    }
    
    return sorted_model_ids;
}