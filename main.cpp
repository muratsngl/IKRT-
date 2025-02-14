#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

//imports for the pipe
#include <Windows.h>
#include <string.h>
#include <memory>
#include <iostream>
#include <vector>

#include "Camera.h"
#include "Shader.h"
#include "FABRIK.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);


const unsigned int SCR_WIDTH = 1240;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f,0.0f,3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main() 
{	
	// Variables for hand recognition and correct input taking
	float cxIndex= 0.f, cyIndex= 0.f, cxMiddle = 0.f, cyMiddle = 0.f,
							cxRing = 0.f, cyRing = 0.f, cxPinky = 0.f, cyPinky = 0.f;
	float deltaXRing= 0.0f, deltaYRing= 0.f,tempXRing= 0.f,tempYRing = 0.f, 
		deltaXIndex = 0.0f, deltaYIndex = 0.f, tempXIndex = 0.f, tempYIndex = 0.f, 
		deltaXMiddle = 0.0f, deltaYMiddle = 0.f, tempXMiddle = 0.f, tempYMiddle = 0.f,
		deltaXPinky = 0.0f, deltaYPinky = 0.f, tempXPinky = 0.f, tempYPinky = 0.f;
	//flag whether the choosen finger is at the center of the screen
	bool isMiddleized = false, firstTrue = false; bool trueInput = false;

	//IPC setup via shared memory 
	const std::string shm_name = "handPositionData";
	std::wstring stemp = std::wstring(shm_name.begin(), shm_name.end());
	LPCWSTR sw = stemp.c_str();
	HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ,FALSE,sw);
	if (hMapFile == NULL) {
		std::cerr << "Could not open shared memory";
		return 1;
	}

	void* pBuf = MapViewOfFile(hMapFile, FILE_MAP_READ, 0,0,41);
	if (pBuf == NULL) {
		std::cerr << "Could not map view of file" << std::endl;
		return 1;
	}

	
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,6);
	glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Inverse Kinematics with Realtime Data",NULL,NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window,framebuffer_size_callback);
	glfwSetCursorPosCallback(window,mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);


	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	//vertex information of guiding triangle and the snake;
	std::vector<float>guidePoints{
		/*-0.3f,-0.1f,0.0f,  //left arm end effector
		0.3f,-0.1f,0.0f, //right arm end effector
		-0.25f,-1.1f,0.0f, //left leg end effector
		0.25f,-1.1f,0.0f //right leg end effector*/
		.0f,.0f,.0f
	};

	std::vector<float> centerSkeletonPoints {
		//root:0
		0.0f,0.0f,0.0f,
		//leftarmsub:1
		-0.3f,0.25f,0.0f,
		//rightarmsub:2
		0.3f,0.25f,0.0f,
		//pelvis:3
		0.0f,-0.5f,0.0f,
		//leftlegsub:4
		-0.25f,-0.5f,0.0f,
		//rightlegsub:5
		0.25f,-0.5f,0.0f,
		//atlas:6
		0.0f,0.25f,0.0f,
		//headtrianglebot:7
		0.0f,0.35f,0.0f,
		//headtriangleleft:8
		-0.1f,0.4f,0.0f,
		//headtriangleright:9
		0.1f,0.4f,0.0f
	};
	//center skeleton indices;
	std::vector<int> centerSkeletonTriangleIndices{
		0,1,2, //center triangle
		7,8,9 //head triangle
		

	};
	std::vector<int> centerSkeletonLineIndices{
		1, 2, //scapula
		0, 3,
		4, 5,
		6,7,
	};
	
	//defined in top to bottom fashion
	std::vector<float> rightArmSubbase{
		0.3f,0.25f,0.0f,
		0.3f,0.1f,0.0f,
		0.3f,-0.1f,0.0f
	};
	std::vector<float> rightLegSubbase{
		0.25f,-0.5f,0.0f,
		0.25f,-0.8f,0.0f,
		0.25f,-1.1f,0.0f
	};
	std::vector<float> leftArmSubbase{
		-0.3f,0.25f,0.0f,
		-0.3f,0.1f,0.0f,
		-0.3f,-0.1f,0.0f};
	std::vector<float> leftLegSubbase{
		-0.25f,-0.5f,0.0f,
		-0.25f,-0.8f,0.0f,
		-0.25f,-1.1f,0.0f
	};
	
	// flags for persistently mapped buffers
	GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
	
	//center skeleton drawing setup: indexed drawing
	unsigned int centerSkeletonVAO;
	unsigned int centerSkeletonVBO;
	unsigned int centerSkeletonTriangleEBO,centerSkeletonLineEBO;
	glGenVertexArrays(1, &centerSkeletonVAO);
	glBindVertexArray(centerSkeletonVAO);

	glGenBuffers(1,&centerSkeletonVBO);
	glBindBuffer(GL_ARRAY_BUFFER, centerSkeletonVBO);
	glBufferData(GL_ARRAY_BUFFER,sizeof(float)*centerSkeletonPoints.size(), centerSkeletonPoints.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &centerSkeletonTriangleEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, centerSkeletonTriangleEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)* centerSkeletonTriangleIndices.size(), centerSkeletonTriangleIndices.data(), GL_STATIC_DRAW);
	
	glGenBuffers(1, &centerSkeletonLineEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, centerSkeletonLineEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)* centerSkeletonLineIndices.size(), centerSkeletonLineIndices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);

	glBindVertexArray(0);
	
	//right arm subbase drawing setup: persistently mapped drawing
	unsigned int rightArmVAO;
	unsigned int rightArmVBO[2];
	float* rightArmBufferHandle[2];
	glGenVertexArrays(1,&rightArmVAO);
	glBindVertexArray(rightArmVAO);

	glGenBuffers(2,rightArmVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rightArmVBO[1]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * rightArmSubbase.size(), nullptr, flags);
	rightArmBufferHandle[1] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * rightArmSubbase.size(), flags);
	glBindBuffer(GL_ARRAY_BUFFER, rightArmVBO[0]);
	glBufferStorage(GL_ARRAY_BUFFER,sizeof(float)*rightArmSubbase.size(),rightArmSubbase.data(),flags);
	rightArmBufferHandle[0] = (float*) glMapBufferRange(GL_ARRAY_BUFFER,0, sizeof(float) * rightArmSubbase.size(),flags);
	
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);
	
	//rightLegDrawingSetup
	unsigned int rightLegVAO;
	unsigned int rightLegVBO[2];
	float* rightLegBufferHandle[2];
	glGenVertexArrays(1, &rightLegVAO);
	glBindVertexArray(rightLegVAO);

	glGenBuffers(2, rightLegVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rightLegVBO[1]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float)* rightLegSubbase.size(), nullptr, flags);
	rightLegBufferHandle[1] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * rightLegSubbase.size(), flags);
	glBindBuffer(GL_ARRAY_BUFFER, rightLegVBO[0]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float)* rightLegSubbase.size(), rightLegSubbase.data(), flags);
	rightLegBufferHandle[0] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * rightLegSubbase.size(), flags);
	
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);
	
	//leftArm drawing setup
	unsigned int leftArmVAO;
	unsigned int leftArmVBO[2];
	float* leftArmBufferHandle[2];
	glGenVertexArrays(1, &leftArmVAO);
	glBindVertexArray(leftArmVAO);

	glGenBuffers(2, leftArmVBO);
	glBindBuffer(GL_ARRAY_BUFFER, leftArmVBO[1]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float)* leftArmSubbase.size(), nullptr, flags);
	leftArmBufferHandle[1] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * leftArmSubbase.size(), flags);
	glBindBuffer(GL_ARRAY_BUFFER, leftArmVBO[0]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float)* leftArmSubbase.size(), leftArmSubbase.data(), flags);
	leftArmBufferHandle[0] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * leftArmSubbase.size(), flags);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);
	
	// Left leg drawing setup
	unsigned int leftLegVAO;
	unsigned int leftLegVBO[2];
	float* leftLegBufferHandle[2];
	glGenVertexArrays(1, &leftLegVAO);
	glBindVertexArray(leftLegVAO);

	glGenBuffers(2, leftLegVBO);
	glBindBuffer(GL_ARRAY_BUFFER, leftLegVBO[1]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float)* leftLegSubbase.size(), nullptr, flags);
	leftLegBufferHandle[1] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * leftLegSubbase.size(), flags);
	glBindBuffer(GL_ARRAY_BUFFER, leftLegVBO[0]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float)* leftLegSubbase.size(), leftLegSubbase.data(), flags);
	leftLegBufferHandle[0] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * leftLegSubbase.size(), flags);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	
	
	glBindVertexArray(0);

	
	unsigned int guidePointsVAO;
	glGenVertexArrays(1, &guidePointsVAO);
	glBindVertexArray(guidePointsVAO);
	
	unsigned int guidePointsVBO;
	glGenBuffers(1, &guidePointsVBO);
	glBindBuffer(GL_ARRAY_BUFFER,guidePointsVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * guidePoints.size(), guidePoints.data(),GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * (sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	

	glBindVertexArray(0);
	
	Shader generalPurposeShader("triangle.vs", "triangle.fs");
	
	int whichBuffertoRead = 0;
	int whichBuffertoWrite = 1;
	
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 humanoidModel = glm::mat4(1.0f);
	
	
	
	glm::vec3 targetPositionIndex(-0.4f, -0.2f, 0.0f);
	glm::vec3 targetPositionMiddle(0.4f, -0.2f, 0.0f);
	glm::vec3 targetPositionRing(-0.35f, -1.2f, 0.0f);
	glm::vec3 targetPositionPinky(0.35f, -1.2f, 0.0f);

	glm::mat4 guidePointsModelIndex = glm::translate(glm::mat4(1.0f),targetPositionIndex);
	glm::mat4 guidePointsModelRing = glm::translate(glm::mat4(1.0f), targetPositionRing);
	glm::mat4 guidePointsModelMiddle = glm::translate(glm::mat4(1.0f), targetPositionMiddle);
	glm::mat4 guidePointsModelPinky = glm::translate(glm::mat4(1.0f), targetPositionPinky);
	
	//translation vectors for each finger
	glm::vec3 deltaIndex(0);
	glm::vec3 deltaMiddle(0);
	glm::vec3 deltaRing(0);
	glm::vec3 deltaPinky(0);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		tempXIndex = cxIndex;
		tempYIndex = cyIndex;
		
		tempXMiddle = cxMiddle;
		tempYMiddle = cyMiddle;
		
		tempXRing = cxRing;
		tempYRing = cyRing;
		
		tempXPinky = cxPinky;
		tempYPinky = cyPinky;
		// room for optimization??single memcpy into an array is much more efficient. for debugging purposes it will stay this way for a while
		memcpy(&cxIndex, (char*)pBuf + 8, sizeof(float));
		memcpy(&cyIndex, (char*)pBuf + 12, sizeof(float));

		memcpy(&cxMiddle, (char*)pBuf + 16, sizeof(float));
		memcpy(&cyMiddle, (char*)pBuf + 20, sizeof(float));

		memcpy(&cxRing, (char*)pBuf + 24, sizeof(float));
		memcpy(&cyRing, (char*)pBuf + 28, sizeof(float));

		memcpy(&cxPinky, (char*)pBuf + 32, sizeof(float));
		memcpy(&cyPinky, (char*)pBuf + 36, sizeof(float));

		memcpy(&trueInput, (char*)pBuf + 40, sizeof(bool));
		//nice catch:delta resembles the difference between 2 values if the mapping were to be used at the point where delta will be used it may map to 
		// -1 at the time of rendering which would result triangle going right off the screen 
		//For the IO side I am satisfied with the result. Still there is room for optimization and more swift input taking by interpolation?
		// after creating the rigged model(humanoid) I will find ways to increase fps
		if (!firstTrue) { isMiddleized = fabs(cxMiddle - 0.5f) < 0.01f && fabs(cyMiddle - 0.5f) < 0.01f; }
		if (isMiddleized) 
		{
			deltaXIndex = 2*(cxIndex-tempXIndex);
			deltaYIndex = -2*(cyIndex -tempYIndex);

			deltaXRing = 2 * (cxRing - tempXRing) ;
			deltaYRing = -2 * (cyRing - tempYRing) ;

			deltaXMiddle = 2 * (cxMiddle - tempXMiddle) ;
			deltaYMiddle = -2 * (cyMiddle - tempYMiddle) ;

			deltaXPinky = 2 * (cxPinky - tempXPinky) ;
			deltaYPinky = -2 * (cyPinky - tempYPinky) ;

			firstTrue = true;
		}
		// per-frame time logic
	    // --------------------
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		// input
		// -----
		
		//calculating deltas for all fingers
		deltaXIndex = fabs(deltaXIndex) < 0.008f || !trueInput ? 0 : deltaXIndex;
		deltaYIndex = fabs(deltaYIndex) < 0.008f || !trueInput ? 0 : deltaYIndex;

		deltaXRing = fabs(deltaXRing) < 0.008f || !trueInput ? 0 : deltaXRing;
		deltaYRing = fabs(deltaYRing) < 0.008f || !trueInput ? 0 : deltaYRing;

		deltaXMiddle = fabs(deltaXMiddle) < 0.008f || !trueInput ? 0 : deltaXMiddle;
		deltaYMiddle = fabs(deltaYMiddle) < 0.008f || !trueInput ? 0 : deltaYMiddle;

		deltaXPinky = fabs(deltaXPinky) < 0.008f || !trueInput ? 0 : deltaXPinky;
		deltaYPinky = fabs(deltaYPinky) < 0.008f || !trueInput ? 0 : deltaYPinky;
		//drawing commands for the guiding point
		
		glPointSize(10.0f);
		view = camera.GetViewMatrix();
		projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glBindVertexArray(guidePointsVAO);
		generalPurposeShader.use();
		generalPurposeShader.setMat4("view", view);
		generalPurposeShader.setMat4("projection", projection);

		//translating and drawing guiding points
		deltaIndex = glm::vec3(2 * deltaXIndex, 1.33 * deltaYIndex, 0.0f);
		guidePointsModelIndex = glm::translate(guidePointsModelIndex, deltaIndex);
		targetPositionIndex += deltaIndex;
		generalPurposeShader.setMat4("model", guidePointsModelIndex);
		glDrawArrays(GL_POINTS, 0, 1);
		
		deltaRing = glm::vec3(2 * deltaXRing, 1.33 * deltaYRing, 0.0f);
		guidePointsModelRing = glm::translate(guidePointsModelRing, deltaRing);
		targetPositionRing += deltaRing;
		generalPurposeShader.setMat4("model", guidePointsModelRing);
		glDrawArrays(GL_POINTS, 0, 1);
		
		deltaMiddle = glm::vec3(2 * deltaXMiddle, 1.33 * deltaYMiddle, 0.0f);
		guidePointsModelMiddle = glm::translate(guidePointsModelMiddle, deltaMiddle);
		targetPositionMiddle += deltaMiddle;
		generalPurposeShader.setMat4("model", guidePointsModelMiddle);
		glDrawArrays(GL_POINTS, 0, 1);
		
		deltaPinky = glm::vec3(2 * deltaXPinky, 1.33 * deltaYPinky, 0.0f);
		guidePointsModelPinky = glm::translate(guidePointsModelPinky, deltaPinky);
		targetPositionPinky += deltaPinky;
		generalPurposeShader.setMat4("model", guidePointsModelPinky);
		glDrawArrays(GL_POINTS, 0, 1);
		
		glPointSize(5.0f);
		generalPurposeShader.setMat4("model", humanoidModel);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBindVertexArray(centerSkeletonVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, centerSkeletonTriangleEBO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, centerSkeletonLineEBO);
		glDrawElements(GL_LINES, 8, GL_UNSIGNED_INT, 0);
		glDrawElements(GL_POINTS, 8, GL_UNSIGNED_INT, 0);
		
		
		
		
		simpleFabrikRoutine(leftArmSubbase, targetPositionIndex);
		simpleFabrikRoutine(rightArmSubbase, targetPositionMiddle);
		simpleFabrikRoutine(leftLegSubbase, targetPositionRing);
		simpleFabrikRoutine(rightLegSubbase, targetPositionPinky);
		
		
		if (whichBuffertoWrite) {
			//use secondHandle
			memcpy(rightArmBufferHandle[1], rightArmSubbase.data(), sizeof(float) * rightArmSubbase.size());
			memcpy(leftArmBufferHandle[1], leftArmSubbase.data(), sizeof(float) * leftArmSubbase.size());
			memcpy(rightLegBufferHandle[1], rightLegSubbase.data(), sizeof(float) * rightLegSubbase.size());
			memcpy(leftLegBufferHandle[1], leftLegSubbase.data(), sizeof(float) * leftLegSubbase.size());
			
		}
		else {
			memcpy(rightArmBufferHandle[0], rightArmSubbase.data(), sizeof(float) * rightArmSubbase.size());
			memcpy(leftArmBufferHandle[0], leftArmSubbase.data(), sizeof(float) * leftArmSubbase.size());
			memcpy(rightLegBufferHandle[0], rightLegSubbase.data(), sizeof(float) * rightLegSubbase.size());
			memcpy(leftLegBufferHandle[0], leftLegSubbase.data(), sizeof(float) * leftLegSubbase.size());
		}
		
		glBindVertexArray(rightArmVAO);
		glBindBuffer(GL_ARRAY_BUFFER, rightArmVBO[whichBuffertoRead]);
		glDrawArrays(GL_LINE_STRIP, 0, 3);
		glDrawArrays(GL_POINTS, 0, 3); 
		
		glBindVertexArray(leftArmVAO);
		glBindBuffer(GL_ARRAY_BUFFER, leftArmVBO[whichBuffertoRead]);
		glDrawArrays(GL_LINE_STRIP, 0, 3);
		glDrawArrays(GL_POINTS, 0, 3);

		glBindVertexArray(rightLegVAO);
		glBindBuffer(GL_ARRAY_BUFFER, rightLegVBO[whichBuffertoRead]);
		glDrawArrays(GL_LINE_STRIP, 0, 3);
		glDrawArrays(GL_POINTS, 0, 3);

		glBindVertexArray(leftLegVAO);
		glBindBuffer(GL_ARRAY_BUFFER, leftLegVBO[whichBuffertoRead]);
		glDrawArrays(GL_LINE_STRIP, 0, 3);
		glDrawArrays(GL_POINTS, 0, 3);
		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			std::cerr << "OpenGL Error: " << std::hex << error << std::endl;
		}
		
		
		//specifies which buffer to use for drawcall::
		whichBuffertoRead = ++whichBuffertoRead%2;
		whichBuffertoWrite = ++whichBuffertoWrite%2;
		processInput(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	glfwTerminate();
	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return 0;	

}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}







//unused code segment
/*std::vector<float>snakeEndpoints{
		0.0f,1.6f,0.0f,
		0.0f,0.8f,0.0f,
		0.0f,0.0f,0.0f,
		0.0f,-0.8f,0.0f,
		0.0f,-1.6f,0.0f
	};*/



//FABRIK ROUTINE AND BUFFER UPDATES
		/*simpleFabrikRoutine(snakeEndpoints, targetPosition);

		if (whichBuffertoWrite) {
			//use secondHandle
			memcpy(secondHandle,snakeEndpoints.data(),sizeof(float)* snakeEndpoints.size());
		}
		else {
			memcpy(firstHandle, snakeEndpoints.data(), sizeof(float) * snakeEndpoints.size());
		}
		glBindVertexArray(snakeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, snakeVBO[whichBuffertoRead]);
		triangleShader.setMat4("model",snakeModel);
		glDrawArrays(GL_LINE_STRIP,0,5);
		glDrawArrays(GL_POINTS,0,5);*/


		/* snake drawing setup;

			unsigned int snakeVAO;
			glGenVertexArrays(1, &snakeVAO);
			glBindVertexArray(snakeVAO);

			unsigned int snakeVBO[2];
			glGenBuffers(2,snakeVBO);
			glBindBuffer(GL_ARRAY_BUFFER, snakeVBO[0]);
			std::cout << "snakeVBO[0]: " << snakeVBO[0] << ", snakeVBO[1]: " << snakeVBO[1] << std::endl;

			glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * snakeEndpoints.size(), NULL,
				flags);
			float* firstHandle = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0,
				sizeof(float) * snakeEndpoints.size(),
				flags);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

			// Allocate and persistently map second VBO
			glBindBuffer(GL_ARRAY_BUFFER, snakeVBO[1]);
			glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * snakeEndpoints.size(), NULL,
				flags);
			float* secondHandle = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0,
				sizeof(float) * snakeEndpoints.size(),
				flags);

			memcpy(firstHandle, snakeEndpoints.data(), sizeof(float) * snakeEndpoints.size());*/

