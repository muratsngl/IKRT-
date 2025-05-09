#ifndef FABRIKH
#define FABRIKH

#include <vector>
#include "glm/glm.hpp">
// single end effector single subbase fabrik implementation
void simple_fabrik_routine(std::vector<float>& currentPositions, const glm::vec3& targetPosition);
void simple_fabrik_routine_indexed(std::vector<glm::vec3>& currentPositions, const glm::vec3& targetPosition,const std::vector<unsigned short> indices);
std::vector<float> find_joint_distances(std::vector<glm::vec3>& currentPositions, const std::vector<unsigned short> indices);
float accum_distances(std::vector<float>&distances);

#endif