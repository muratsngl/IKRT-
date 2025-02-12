#include "FABRIK.h"

//stop the process if target position is within the tolerance
#define TOLERANCE 0.0001
#define MAX_ITERATIONS 50

void simpleFabrikRoutine(std::vector<float>& currentPositions, const glm::vec3& targetPosition) {
	//for the simple snake model I have set joint distances to 1 this will 
	// vary as more complex models are introduced. but 
	// they will always be constant so its a good idea to calculate them prior to the routine
	int totalSize = currentPositions.size();
	int totalSizeinJoints = totalSize / 3;
	int numberOfBones = totalSizeinJoints - 1;
	float distanceBetweenJoints = numberOfBones * 0.4f;
	glm::vec3 rootNode(currentPositions[0], currentPositions[1], currentPositions[2]);
	float distanceBetweenRootAndTarget = glm::distance(rootNode,targetPosition);
	glm::vec3 prevNode,currentNode;
	float lambda;
	//target is unreachable
	if (distanceBetweenRootAndTarget > distanceBetweenJoints) {
		float distanceBetweenJandR ;
		prevNode = rootNode;
		for (int i = 3; i < totalSize;i+=3) {
			distanceBetweenJandR = glm::distance(prevNode,targetPosition);
			if (distanceBetweenJandR > 0) {
				lambda = 0.4f / distanceBetweenJandR;
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
		while (distanceBetweenEndTargetPosition > TOLERANCE||loopCounter<MAX_ITERATIONS) {
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
					lambda = 0.4f / distanceBetweenPiPii;
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
					lambda = 0.4f / distanceBetweenPiPii;
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