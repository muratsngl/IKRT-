#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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
void process_input(GLFWwindow* window);
void init_buffers();
void init_shaders();
void create_snake_bone_transforms();



const unsigned int SCR_WIDTH = 1240;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f,0.0f,3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float cxIndex = 0.f, cyIndex = 0.f, czIndex = 0.f, cxMiddle = 0.f, cyMiddle = 0.f, czMiddle = 0.f,
cxRing = 0.f, cyRing = 0.f, czRing = 0.f, cxPinky = 0.f, cyPinky = 0.f, czPinky = 0.f;

float deltaXRing = 0.0f, deltaYRing = 0.f, deltaZRing = 0.f, tempXRing = 0.f, tempYRing = 0.f, tempZRing = 0.f,
deltaXIndex = 0.0f, deltaYIndex = 0.f, deltaZIndex = 0.f, tempXIndex = 0.f, tempYIndex = 0.f, tempZIndex = 0.f,
deltaXMiddle = 0.0f, deltaYMiddle = 0.f, deltaZMiddle = 0.f, tempXMiddle = 0.f, tempYMiddle = 0.f, tempZMiddle = 0.f,
deltaXPinky = 0.0f, deltaYPinky = 0.f, deltaZPinky = 0.f, tempXPinky = 0.f, tempYPinky = 0.f, tempZPinky = 0.f;
//flag whether the choosen finger is at the center of the screen
bool isMiddleized = false, firstTrue = false; bool trueInput = false;


std::vector<float>guidePoints{
	.0f,.0f,.0f
};

std::vector<float> centerSkeletonPoints{
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
	-0.3f,-0.1f,0.0f };
std::vector<float> leftLegSubbase{
	-0.25f,-0.5f,0.0f,
	-0.25f,-0.8f,0.0f,
	-0.25f,-1.1f,0.0f
};
//vertex information of guiding triangle and the snake;
std::vector<float>snakeEndpoints{
		0.0f,1.6f,0.0f,
		0.0f,1.4f,0.0f,
		0.0f,1.2f,0.0f,
		0.0f,1.0f,0.0f,
		0.0f,0.8f,0.0f
};
std::vector<float>snakeEndpointsOriginal = snakeEndpoints;
std::vector<float>snakeMesh{
		0.0f,1.6f,-0.1f,  //0
		0.0f,1.6f,0.1f, 
		0.1f,1.6f,0.0f,
		-0.1f,1.6f,0.0f,
		
		0.0f,1.5f,-0.1f, //4
		0.0f,1.5f,0.1f,
		0.1f,1.5f,0.0f,
		-0.1f,1.5f,0.0f,

		0.0f,1.4f,-0.1f, //8
		0.0f,1.4f,0.1f,
		0.1f,1.4f,0.0f,
		-0.1f,1.4f,0.0f,
		
		0.0f,1.3f,-0.1f, //12
		0.0f,1.3f,0.1f,
		0.1f,1.3f,0.0f,
		-0.1f,1.3f,0.0f,
		
		0.0f,1.2f,-0.1f, //16
		0.0f,1.2f,0.1f,
		0.1f,1.2f,0.0f,
		-0.1f,1.2f,0.0f,

		0.0f,1.1f,-0.1f, //20
		0.0f,1.1f,0.1f,
		0.1f,1.1f,0.0f,
		-0.1f,1.1f,0.0f,

		0.0f,1.0f,-0.1f, //24
		0.0f,1.0f,0.1f,
		0.1f,1.0f,0.0f,
		-0.1f,1.0f,0.0f,

		0.0f,0.9f,-0.1f, //28
		0.0f,0.9f,0.1f, 
		0.1f,0.9f,0.0f, 
		-0.1f,0.9f,0.0f, 

		0.0f,0.8f,-0.1f, //32
		0.0f,0.8f,0.1f, 
		0.1f,0.8f,0.0f, 
		-0.1f,0.8f,0.0f, //35
};
std::vector<GLuint> snakeJointIDs{
	1,0,
	1,0,
	1,0,
	1,0,
	
	1,2,
	1,2,
	1,2,
	1,2,
	
	2,0,
	2,0,
	2,0,
	2,0,
	
	2,3,
	2,3,
	2,3,
	2,3,

	3,0,
	3,0,
	3,0,
	3,0,

	3,4,
	3,4,
	3,4,
	3,4,

	4,0,
	4,0,
	4,0,
	4,0,
	
	4,5,
	4,5,
	4,5,
	4,5,

	5,0,
	5,0,
	5,0,
	5,0,

};
std::vector<float> snakeJointWeights{
	1.0f,0.0f,
	1.0f,0.0f,
	1.0f,0.0f,
	1.0f,0.0f,
	
	0.5f,0.5f,
	0.5f,0.5f,
	0.5f,0.5f,
	0.5f,0.5f,

	1.0f,0.0f,
	1.0f,0.0f,
	1.0f,0.0f,
	1.0f,0.0f,

	0.5f,0.5f,
	0.5f,0.5f,
	0.5f,0.5f,
	0.5f,0.5f,

	1.0f,0.0f,
	1.0f,0.0f,
	1.0f,0.0f,
	1.0f,0.0f,

	0.5f,0.5f,
	0.5f,0.5f,
	0.5f,0.5f,
	0.5f,0.5f,
	
	1.0f,0.0f,
	1.0f,0.0f,
	1.0f,0.0f,
	1.0f,0.0f,

	0.5f,0.5f,
	0.5f,0.5f,
	0.5f,0.5f,
	0.5f,0.5f,

	1.0f,0.0f,
	1.0f,0.0f,
	1.0f,0.0f,
	1.0f,0.0f,

};
std::vector<GLuint> snakeMeshIndices{
	//top cap
	0,3,2,
	3,1,2,
	
	3,7,5, //front left-1
	3,5,1, //front left-2

	2,5,6,
	1,5,2,

	7,11,9,
	7,9,5,

	6,9,10,
	5,9,6,

	11,15,13,
	11,13,9,

	10,13,14,
	9,13,10,

	15,19,17,
	15,17,14,

	14,17,18,
	13,17,14,
	
	18,21,22,
	17,21,18,

	19,23,21,
	19,21,18,

	23,27,25,
	23,25,22,

	22,25,26,
	21,25,22,

	27,31,29,
	27,29,26,

	26,29,30,
	25,29,26,

	31,35,33,
	31,33,30,

	30,33,34,
	29,33,30,
	
	//back faces
	2,6,4,
	2,4,0,

	3,0,4,
	4,7,3,

	6,10,8,
	6,8,4,

	7,4,8,
	8,11,7,

	10,14,12,
	10,12,8,

	11,8,12,
	12,15,11,

	14,18,16,
	14,16,12,

	15,12,16,
	16,19,15,
	 
	18,22,20,
	18,20,16,

	19,16,20,
	20,23,19,

	22,26,24,
	22,24,20,

	23,20,24,
	24,27,23,

	26,30,28,
	26,28,24,

	27,24,28,
	28,31,27,

	30,34,32,
	30,32,28,

	31,28,32,
	32,35,31,
	
	//bottom cap
	32,35,34,
	35,33,34


};


int whichBuffertoRead = 0;
int whichBuffertoWrite = 1;

glm::mat4 view;
glm::mat4 projection;
glm::mat4 humanoidModel = glm::mat4(1.0f);

std::vector<glm::mat4> snake_bone_transforms={glm::mat4(1.0f),glm::mat4(1.0f) ,glm::mat4(1.0f) ,glm::mat4(1.0f),glm::mat4(1.0f)  };

glm::vec3 targetPositionIndex(0.0f, 0.8f, 0.0f);

//glm::vec3 targetPositionIndex(-0.4f, -0.2f, 0.0f);
glm::vec3 targetPositionMiddle(0.4f, -0.2f, 0.0f);
glm::vec3 targetPositionRing(-0.35f, -1.2f, 0.0f);
glm::vec3 targetPositionPinky(0.35f, -1.2f, 0.0f);

glm::mat4 guidePointsModelIndex = glm::translate(glm::mat4(1.0f), targetPositionIndex);
glm::mat4 guidePointsModelRing = glm::translate(glm::mat4(1.0f), targetPositionRing);
glm::mat4 guidePointsModelMiddle = glm::translate(glm::mat4(1.0f), targetPositionMiddle);
glm::mat4 guidePointsModelPinky = glm::translate(glm::mat4(1.0f), targetPositionPinky);

//translation vectors for each finger
glm::vec3 deltaIndex(0);
glm::vec3 deltaMiddle(0);
glm::vec3 deltaRing(0);
glm::vec3 deltaPinky(0);
//all vao&vbo&ebo
GLuint centerSkeletonVAO;
GLuint centerSkeletonVBO;
GLuint centerSkeletonTriangleEBO, centerSkeletonLineEBO;
//right arm subbase drawing setup: persistently mapped drawing
GLuint rightArmVAO;
GLuint rightArmVBO[2];
GLfloat* rightArmBufferHandle[2];
//rightLegDrawingSetup
GLuint rightLegVAO;
unsigned int rightLegVBO[2];
GLfloat* rightLegBufferHandle[2];
//leftArm drawing setup
GLuint leftArmVAO;
GLuint leftArmVBO[2];
GLfloat* leftArmBufferHandle[2];
// Left leg drawing setup
GLuint leftLegVAO;
GLuint leftLegVBO[2];
GLfloat* leftLegBufferHandle[2];
//guide points VAO VBO
GLuint guidePointsVAO;
GLuint guidePointsVBO;
//snake vao vbo
GLuint snakeVBO[2], snakeVAO ;
GLfloat* snakeBufferHandle[2];

GLuint snakeMeshVBO,snakeMeshVAO, snakeMeshEBO; 

GLuint snakeBoneUBO;


int main() 
{	
	// Variables for hand recognition and correct input taking
	

	//IPC setup via shared memory 
	const std::string shm_name = "handPositionData";
	std::wstring stemp = std::wstring(shm_name.begin(), shm_name.end());
	LPCWSTR sw = stemp.c_str();
	HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ,FALSE,sw);
	if (hMapFile == NULL) {
		std::cerr << "Could not open shared memory";
		return 1;
	}

	void* pBuf = MapViewOfFile(hMapFile, FILE_MAP_READ, 0,0,62);
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
	Shader generalPurposeShader("triangle.vs", "triangle.fs");
	Shader snakeMeshShader("snake_mesh.vs", "snake_mesh.fs");
	
	init_buffers();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		tempXIndex = cxIndex;
		tempYIndex = cyIndex;
		tempZIndex = czIndex;
		
		tempXMiddle = cxMiddle;
		tempYMiddle = cyMiddle;
		tempZMiddle = czMiddle;
		
		tempXRing = cxRing;
		tempYRing = cyRing;
		tempZRing = czRing;
		
		tempXPinky = cxPinky;
		tempYPinky = cyPinky;
		tempZPinky = czPinky;
		// room for optimization??single memcpy into an array is much more efficient. for debugging purposes it will stay this way for a while
		memcpy(&cxIndex, (char*)pBuf + 8, sizeof(float));
		memcpy(&cyIndex, (char*)pBuf + 12, sizeof(float));
		memcpy(&czIndex, (char*)pBuf + 41, sizeof(float));
		
		memcpy(&cxMiddle, (char*)pBuf + 16, sizeof(float));
		memcpy(&cyMiddle, (char*)pBuf + 20, sizeof(float));
		memcpy(&czMiddle, (char*)pBuf + 45, sizeof(float));
		
		memcpy(&cxRing, (char*)pBuf + 24, sizeof(float));
		memcpy(&cyRing, (char*)pBuf + 28, sizeof(float));
		memcpy(&czRing, (char*)pBuf + 49, sizeof(float));

		memcpy(&cxPinky, (char*)pBuf + 32, sizeof(float));
		memcpy(&cyPinky, (char*)pBuf + 36, sizeof(float));
		memcpy(&czPinky, (char*)pBuf + 53, sizeof(float));

		memcpy(&trueInput, (char*)pBuf + 40, sizeof(bool));
		//nice catch:delta resembles the difference between 2 values if the mapping were to be used at the point where delta will be used it may map to 
		// -1 at the time of rendering which would result triangle going right off the screen 
		//For the IO side I am satisfied with the result. Still there is room for optimization and more swift input taking by interpolation?
		// after creating the rigged model(humanoid) I will find ways to increase fps
		// currently no middle calibration for z axis is defined TODO::
		if (!firstTrue) { isMiddleized = fabs(cxMiddle - 0.5f) < 0.01f && fabs(cyMiddle - 0.5f) < 0.01f; }
		if (isMiddleized) 
		{
			deltaXIndex = 2*(cxIndex-tempXIndex);
			deltaYIndex = -2*(cyIndex -tempYIndex);
			deltaZIndex = 2 * (czIndex - tempZIndex);

			deltaXRing = 2 * (cxRing - tempXRing) ;
			deltaYRing = -2 * (cyRing - tempYRing) ;
			deltaZRing = 2 * (czRing - tempZRing);
			
			deltaXMiddle = 2 * (cxMiddle - tempXMiddle) ;
			deltaYMiddle = -2 * (cyMiddle - tempYMiddle) ;
			deltaZMiddle = 2 * (czMiddle - tempZMiddle);

			deltaXPinky = 2 * (cxPinky - tempXPinky) ;
			deltaYPinky = -2 * (cyPinky - tempYPinky) ;
			deltaZPinky = 2 * (czPinky - tempZPinky);

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
		deltaZIndex = fabs(deltaZIndex) < 0.008f || !trueInput ? 0 : deltaZIndex;

		deltaXRing = fabs(deltaXRing) < 0.008f || !trueInput ? 0 : deltaXRing;
		deltaYRing = fabs(deltaYRing) < 0.008f || !trueInput ? 0 : deltaYRing;
		deltaZRing = fabs(deltaZRing) < 0.008f || !trueInput ? 0 : deltaZRing;

		deltaXMiddle = fabs(deltaXMiddle) < 0.008f || !trueInput ? 0 : deltaXMiddle;
		deltaYMiddle = fabs(deltaYMiddle) < 0.008f || !trueInput ? 0 : deltaYMiddle;
		deltaZMiddle = fabs(deltaZMiddle) < 0.008f || !trueInput ? 0 : deltaZMiddle;

		deltaXPinky = fabs(deltaXPinky) < 0.008f || !trueInput ? 0 : deltaXPinky;
		deltaYPinky = fabs(deltaYPinky) < 0.008f || !trueInput ? 0 : deltaYPinky;
		deltaZPinky = fabs(deltaZPinky) < 0.008f || !trueInput ? 0 : deltaZPinky;
		//drawing commands for the guiding point
		
		glPointSize(10.0f);
		view = camera.GetViewMatrix();
		projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glBindVertexArray(guidePointsVAO);
		generalPurposeShader.use();
		generalPurposeShader.setMat4("view", view);
		generalPurposeShader.setMat4("projection", projection);

		//translating and drawing guiding points
		deltaIndex = glm::vec3(2 * deltaXIndex, 1.33 * deltaYIndex, deltaZIndex);

		guidePointsModelIndex = glm::translate(guidePointsModelIndex, deltaIndex);
		targetPositionIndex += deltaIndex;
		generalPurposeShader.setMat4("model", guidePointsModelIndex);
		glDrawArrays(GL_POINTS, 0, 1);
		
		/*deltaRing = glm::vec3(2 * deltaXRing, 1.33 * deltaYRing, deltaZRing);
		guidePointsModelRing = glm::translate(guidePointsModelRing, deltaRing);
		targetPositionRing += deltaRing;
		generalPurposeShader.setMat4("model", guidePointsModelRing);
		glDrawArrays(GL_POINTS, 0, 1);
		
		deltaMiddle = glm::vec3(2 * deltaXMiddle, 1.33 * deltaYMiddle, deltaZMiddle);
		guidePointsModelMiddle = glm::translate(guidePointsModelMiddle, deltaMiddle);
		targetPositionMiddle += deltaMiddle;
		generalPurposeShader.setMat4("model", guidePointsModelMiddle);
		glDrawArrays(GL_POINTS, 0, 1);
		
		deltaPinky = glm::vec3(2 * deltaXPinky, 1.33 * deltaYPinky, deltaZPinky);
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
		glDrawElements(GL_POINTS, 8, GL_UNSIGNED_INT, 0);*/
		
		
		
		simple_fabrik_routine(snakeEndpoints, targetPositionIndex);
		create_snake_bone_transforms();
		//simple_fabrik_routine(leftArmSubbase, targetPositionIndex);
		/*simple_fabrik_routine(rightArmSubbase, targetPositionMiddle);
		simple_fabrik_routine(leftLegSubbase, targetPositionRing);
		simple_fabrik_routine(rightLegSubbase, targetPositionPinky);
		*/
		
		if (whichBuffertoWrite) {
			//use secondHandle
			//memcpy(rightArmBufferHandle[1], rightArmSubbase.data(), sizeof(float) * rightArmSubbase.size());
			//memcpy(leftArmBufferHandle[1], leftArmSubbase.data(), sizeof(float) * leftArmSubbase.size());
			//memcpy(rightLegBufferHandle[1], rightLegSubbase.data(), sizeof(float) * rightLegSubbase.size());
			//memcpy(leftLegBufferHandle[1], leftLegSubbase.data(), sizeof(float) * leftLegSubbase.size());
			memcpy(snakeBufferHandle[1], snakeEndpoints.data(), sizeof(float)* snakeEndpoints.size());
			
		}
		else {
			/*memcpy(rightArmBufferHandle[0], rightArmSubbase.data(), sizeof(float) * rightArmSubbase.size());
			memcpy(leftArmBufferHandle[0], leftArmSubbase.data(), sizeof(float) * leftArmSubbase.size());
			memcpy(rightLegBufferHandle[0], rightLegSubbase.data(), sizeof(float) * rightLegSubbase.size());
			memcpy(leftLegBufferHandle[0], leftLegSubbase.data(), sizeof(float) * leftLegSubbase.size());*/
			memcpy(snakeBufferHandle[0], snakeEndpoints.data(), sizeof(float)* snakeEndpoints.size());
		}
		
		/*glBindVertexArray(rightArmVAO);
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
		glDrawArrays(GL_POINTS, 0, 3);*/
		generalPurposeShader.setMat4("model", humanoidModel);
		
		glBindVertexArray(snakeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, snakeVBO[whichBuffertoRead]);
		glDrawArrays(GL_LINE_STRIP, 0, 5);
		glDrawArrays(GL_POINTS, 0, 5);
		snakeMeshShader.use();
		snakeMeshShader.setMat4("model", humanoidModel);
		snakeMeshShader.setMat4("view", view);
		snakeMeshShader.setMat4("projection", projection);
		glBindBuffer(GL_UNIFORM_BUFFER, snakeBoneUBO);
		glBufferSubData(GL_UNIFORM_BUFFER,0,sizeof(glm::mat4),glm::value_ptr(snake_bone_transforms[0]));
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(snake_bone_transforms[1]));
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4)*2, sizeof(glm::mat4), glm::value_ptr(snake_bone_transforms[2]));
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 3, sizeof(glm::mat4), glm::value_ptr(snake_bone_transforms[3]));
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 4, sizeof(glm::mat4), glm::value_ptr(snake_bone_transforms[4]));
		glBindVertexArray(snakeMeshVAO);
		glDrawElements(GL_TRIANGLES, snakeMeshIndices.size(), GL_UNSIGNED_INT, 0);
		
		
		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			std::cerr << "OpenGL Error: " << std::hex << error << std::endl;
		}
		
		
		//specifies which buffer to use for drawcall::
		whichBuffertoRead = ++whichBuffertoRead%2;
		whichBuffertoWrite = ++whichBuffertoWrite%2;
		process_input(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	glfwTerminate();
	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return 0;	

}

void process_input(GLFWwindow* window)
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
//initializes vao,vbo and ubo setups
void init_buffers() {
	GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
	
	glGenVertexArrays(1, &centerSkeletonVAO);
	glBindVertexArray(centerSkeletonVAO);

	glGenBuffers(1, &centerSkeletonVBO);
	glBindBuffer(GL_ARRAY_BUFFER, centerSkeletonVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * centerSkeletonPoints.size(), centerSkeletonPoints.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &centerSkeletonTriangleEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, centerSkeletonTriangleEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * centerSkeletonTriangleIndices.size(), centerSkeletonTriangleIndices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &centerSkeletonLineEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, centerSkeletonLineEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * centerSkeletonLineIndices.size(), centerSkeletonLineIndices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glBindVertexArray(0);


	glGenVertexArrays(1, &rightArmVAO);
	glBindVertexArray(rightArmVAO);

	glGenBuffers(2, rightArmVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rightArmVBO[1]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * rightArmSubbase.size(), nullptr, flags);
	rightArmBufferHandle[1] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * rightArmSubbase.size(), flags);
	glBindBuffer(GL_ARRAY_BUFFER, rightArmVBO[0]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * rightArmSubbase.size(), rightArmSubbase.data(), flags);
	rightArmBufferHandle[0] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * rightArmSubbase.size(), flags);


	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);



	glGenVertexArrays(1, &rightLegVAO);
	glBindVertexArray(rightLegVAO);

	glGenBuffers(2, rightLegVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rightLegVBO[1]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * rightLegSubbase.size(), nullptr, flags);
	rightLegBufferHandle[1] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * rightLegSubbase.size(), flags);
	glBindBuffer(GL_ARRAY_BUFFER, rightLegVBO[0]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * rightLegSubbase.size(), rightLegSubbase.data(), flags);
	rightLegBufferHandle[0] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * rightLegSubbase.size(), flags);


	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);


	glGenVertexArrays(1, &leftArmVAO);
	glBindVertexArray(leftArmVAO);

	glGenBuffers(2, leftArmVBO);
	glBindBuffer(GL_ARRAY_BUFFER, leftArmVBO[1]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * leftArmSubbase.size(), nullptr, flags);
	leftArmBufferHandle[1] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * leftArmSubbase.size(), flags);
	glBindBuffer(GL_ARRAY_BUFFER, leftArmVBO[0]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * leftArmSubbase.size(), leftArmSubbase.data(), flags);
	leftArmBufferHandle[0] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * leftArmSubbase.size(), flags);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);


	glGenVertexArrays(1, &leftLegVAO);
	glBindVertexArray(leftLegVAO);

	glGenBuffers(2, leftLegVBO);
	glBindBuffer(GL_ARRAY_BUFFER, leftLegVBO[1]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * leftLegSubbase.size(), nullptr, flags);
	leftLegBufferHandle[1] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * leftLegSubbase.size(), flags);
	glBindBuffer(GL_ARRAY_BUFFER, leftLegVBO[0]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float) * leftLegSubbase.size(), leftLegSubbase.data(), flags);
	leftLegBufferHandle[0] = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * leftLegSubbase.size(), flags);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);


	glBindVertexArray(0);



	glGenVertexArrays(1, &guidePointsVAO);
	glBindVertexArray(guidePointsVAO);


	glGenBuffers(1, &guidePointsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, guidePointsVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * guidePoints.size(), guidePoints.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * (sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	
	glGenVertexArrays(1, &snakeVAO);
	glBindVertexArray(snakeVAO);
	
	

	glGenBuffers(2,snakeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, snakeVBO[1]);
	glBufferStorage(GL_ARRAY_BUFFER,sizeof(float)*snakeEndpoints.size(),nullptr,flags);
	snakeBufferHandle[1] = (GLfloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * snakeEndpoints.size(), flags);
	glBindBuffer(GL_ARRAY_BUFFER, snakeVBO[0]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float)* snakeEndpoints.size(), snakeEndpoints.data(), flags);
	snakeBufferHandle[0] = (GLfloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * snakeEndpoints.size(), flags);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	
	glBindVertexArray(0);

	glGenVertexArrays(1,&snakeMeshVAO);
	glBindVertexArray(snakeMeshVAO);

	glGenBuffers(1,&snakeMeshVBO);
	glGenBuffers(1, &snakeMeshEBO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,snakeMeshEBO);
	glBindBuffer(GL_ARRAY_BUFFER,snakeMeshVBO);
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)* (snakeMesh.size()+snakeJointWeights.size())+sizeof(GLuint)*snakeJointIDs.size(), nullptr, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(float)* snakeMesh.size(),snakeMesh.data());
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)* snakeMesh.size(), sizeof(float)* snakeJointWeights.size(),snakeJointWeights.data());
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)* (snakeMesh.size() + snakeJointWeights.size()), sizeof(GLuint)* snakeJointIDs.size(),snakeJointIDs.data());
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* snakeMeshIndices.size(), snakeMeshIndices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
	
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,(void*)(sizeof(float)* snakeMesh.size()));
	
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 2, GL_UNSIGNED_INT, 0, (void*) (sizeof(float)* (snakeMesh.size() + snakeJointWeights.size())));
	
	glGenBuffers(1,&snakeBoneUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, snakeBoneUBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, snakeBoneUBO);
	glBufferData(GL_UNIFORM_BUFFER,320,NULL,GL_DYNAMIC_READ);
	glBindBuffer(GL_UNIFORM_BUFFER,0);

}


void init_shaders() {
	

}

//shitty global dependent code for fast prototyping
void create_snake_bone_transforms() {
	for (int i = 2; i < snakeEndpoints.size(); i += 3) {
		glm::vec3 newPosition(snakeEndpoints[i - 2], snakeEndpoints[i - 1], snakeEndpoints[i]);
		glm::vec3 originalPosition(snakeEndpointsOriginal[i - 2], snakeEndpointsOriginal[i - 1], snakeEndpointsOriginal[i]);
		glm::vec3 translateVector(newPosition - originalPosition);
		snake_bone_transforms[i/3] = glm::translate(glm::mat4(1.0f), translateVector);
	}
}


//unused code segment
/**/



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

