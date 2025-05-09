#include "FABRIK.h"
#include <iostream>

//stop the process if target position is within the tolerance
#define TOLERANCE 0.001f
#define MAX_ITERATIONS 50

void simple_fabrik_routine(std::vector<float>& currentPositions, const glm::vec3& targetPosition) {
	//for the simple snake model I have set joint distances to 1 this will 
	// vary as more complex models are introduced. but 
	// they will always be constant so its a good idea to calculate them prior to the routine
	int totalSize = currentPositions.size();
	int totalSizeinJoints = totalSize / 3;
	int numberOfBones = totalSizeinJoints - 1;
	float distanceBetweenJoints = numberOfBones * 0.4f;
	glm::vec3 rootNode(currentPositions[0], currentPositions[1], currentPositions[2]);
	float distanceBetweenRootAndTarget = glm::distance(rootNode,targetPosition);
	glm::vec3 prevNode,currentNode, prevNodeOld;
	float lambda;
	glm::vec3 distanceConsecJoints;
	//target is unreachable
	if (distanceBetweenRootAndTarget > distanceBetweenJoints) {
		float distanceBetweenJandR ;
		prevNode = rootNode;
		for (int i = 3; i < totalSize;i+=3) {
			

			distanceBetweenJandR = glm::distance(prevNode,targetPosition);
			if (distanceBetweenJandR > 0) {
				lambda = 0.2f / distanceBetweenJandR;
			}
			else lambda = 0;
			
			currentNode = (1 - lambda) * prevNode + lambda * targetPosition;
			currentPositions[i] = currentNode.x;
			currentPositions[i+1] = currentNode.y;
			currentPositions[i+2] = currentNode.z;
			
			prevNode = currentNode;
		}
	}
	else {
		
		glm::vec3 endEffectorPosition(currentPositions[totalSize-3], 
									  currentPositions[totalSize - 2], 
									  currentPositions[totalSize - 1]);
		float distanceBetweenEndTargetPosition = glm::distance(endEffectorPosition, targetPosition);
		float distanceBetweenPiPii;
		int loopCounter = 0;
		while (distanceBetweenEndTargetPosition > TOLERANCE&&loopCounter<MAX_ITERATIONS) {
			loopCounter++;
			endEffectorPosition = targetPosition;
			currentPositions[totalSize - 3] = targetPosition.x;
			currentPositions[totalSize - 2] = targetPosition.y;
			currentPositions[totalSize - 1] = targetPosition.z;
			
			prevNode = endEffectorPosition;
			for (int k = totalSize - 6; k >= 0; k -= 3) {
				currentNode = glm::vec3 (currentPositions[k],
							   currentPositions[k+1],
							   currentPositions[k+2]);
				distanceBetweenPiPii = glm::distance(prevNode, currentNode);
				if (distanceBetweenPiPii > 0) {
					lambda = 0.2f / distanceBetweenPiPii;
				}
				else lambda = 0;
				currentNode = (1 - lambda) * prevNode + lambda * currentNode;
				currentPositions[k] = currentNode.x;
				currentPositions[k+1] = currentNode.y;
				currentPositions[k+2] = currentNode.z;
				prevNode = currentNode;
			}
			currentPositions[0] = rootNode.x;
			currentPositions[1] = rootNode.y;
			currentPositions[2] = rootNode.z;
			prevNode = rootNode;
			for (int j = 3; j < totalSize; j += 3) {
				currentNode = glm::vec3(currentPositions[j],
					currentPositions[j + 1],
					currentPositions[j + 2]);
				distanceBetweenPiPii = glm::distance(prevNode, currentNode);
				if (distanceBetweenPiPii > 0) {
					lambda = 0.2f / distanceBetweenPiPii;
				}
				else lambda = 0;
				currentNode = (1 - lambda) * prevNode + lambda * currentNode;
				currentPositions[j] = currentNode.x;
				currentPositions[j + 1] = currentNode.y;
				currentPositions[j + 2] = currentNode.z;
				prevNode = currentNode;
			}

			endEffectorPosition = glm::vec3 (currentPositions[totalSize - 3],
											 currentPositions[totalSize - 2],
											 currentPositions[totalSize - 1]);
			distanceBetweenEndTargetPosition = glm::distance(endEffectorPosition, targetPosition);
		
		}


	}

	

}
void simple_fabrik_routine_indexed(std::vector<glm::vec3>& currentPositions, const glm::vec3& targetPosition, const std::vector<unsigned short> indices) {
	
	short totalSizeinJoints = indices.size();
	short totalSize = totalSizeinJoints * 3;
	short numberOfBones = totalSizeinJoints - 1;
	std::vector<float> distances = find_joint_distances(currentPositions,indices);
	float totalLength = accum_distances(distances);
	float distanceBetweenRootAndTarget = glm::distance(currentPositions[indices[0]], targetPosition);
	if (totalLength < distanceBetweenRootAndTarget){
		float distanceBetweenJandR = distanceBetweenRootAndTarget;
		glm::vec3 prevJoint = currentPositions[indices[0]];
		float lambda;
		for (short i = 0; i < totalSizeinJoints-1; i++) {
			if (distanceBetweenJandR > 0) {
				lambda = distances[i]/distanceBetweenJandR;
			}
			else lambda = 0.0f;
			currentPositions[indices[i + 1]] = (1 - lambda) * prevJoint + lambda * targetPosition;
			distanceBetweenJandR = glm::distance(currentPositions[indices[i + 1]], targetPosition);
			prevJoint = currentPositions[indices[i + 1]];
			}
		
	}
	//target is reachable
	else {
		int loopCounter = 0;
		glm::vec3& endEffector = currentPositions[indices.back()];
		float distanceBetweenEndTargetPosition = glm::distance(endEffector, targetPosition);
		glm::vec3 rootOriginalPosition = currentPositions[indices[0]];
		float distanceBetweenJointk1andk0;
		float distanceBetweenJointk0andk1;
		float lambda = 0.f;
		while (distanceBetweenEndTargetPosition > TOLERANCE && loopCounter < MAX_ITERATIONS) {
			currentPositions[indices.back()] = targetPosition;
			//forward reaching pass
			for (short k = totalSizeinJoints - 1; k > 0; k--) {
				distanceBetweenJointk1andk0 = glm::distance(currentPositions[indices[k]],currentPositions[indices[k-1]]);
				if (distanceBetweenJointk1andk0 > 0) {
					lambda =  distances[k - 1]/distanceBetweenJointk1andk0;
				}
				else lambda = 0.f;
				currentPositions[indices[k - 1]] = (1 - lambda) * currentPositions[indices[k]] + lambda * currentPositions[indices[k-1]];
				
			}
			currentPositions[indices[0]] = rootOriginalPosition;
			for (short j = 0; j < totalSizeinJoints - 1; j++) {
				distanceBetweenJointk0andk1 = glm::distance(currentPositions[indices[j]], currentPositions[indices[j + 1]]);
				if (distanceBetweenJointk0andk1 > 0) {
					lambda = distances[j]/distanceBetweenJointk0andk1;
				}
				else lambda = 0;
				currentPositions[indices[j + 1]] = (1 - lambda) * currentPositions[indices[j]] + lambda * currentPositions[indices[j + 1]];
			}
			distanceBetweenEndTargetPosition = glm::distance(currentPositions[indices.back()], targetPosition);
			loopCounter++;
		}
		
	}
	
}

std::vector<float> find_joint_distances(std::vector<glm::vec3>& currentPositions, const std::vector<unsigned short> indices) {
	std::vector<float> distances;
	for (short i = 0; i < indices.size() - 1; i++) {
		distances.push_back(glm::distance(currentPositions[indices[i]], currentPositions[indices[i + 1]]));
		
	}
	return distances;
}

float accum_distances(std::vector<float>& distances) {
	float accum = 0.f;
	for (float k : distances) {
		accum += k;
	}
	
	return accum;
}