#include "include/misc_math.hpp"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

glm::mat4 interpolate_transform(const glm::mat4& start, const glm::mat4& end, float alpha) {
    glm::vec3 scale_start, translation_start, skew_start;
    glm::vec4 perspective_start;
    glm::quat rotation_start;
    glm::decompose(start, scale_start, rotation_start, translation_start, skew_start, perspective_start);

    glm::vec3 scale_end, translation_end, skew_end;
    glm::vec4 perspective_end;
    glm::quat rotation_end;
    glm::decompose(end, scale_end, rotation_end, translation_end, skew_end, perspective_end);

    glm::vec3 final_translation = glm::mix(translation_start, translation_end, alpha);
    glm::vec3 final_scale = glm::mix(scale_start, scale_end, alpha);
    glm::quat final_rotation = glm::slerp(rotation_start, rotation_end, alpha);

    glm::mat4 result = glm::translate(glm::mat4(1.0f), final_translation) *
                       glm::toMat4(final_rotation) *
                       glm::scale(glm::mat4(1.0f), final_scale);
    return result;
}

glm::vec3 interpolate_position(const glm::vec3& start, const glm::vec3& end, float alpha) {
    return glm::mix(start, end, alpha);
}
