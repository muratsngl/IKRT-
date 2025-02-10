#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

//imports for the pipe
#include <Windows.h>
#include <string.h>
#include <memory>
#include <iostream>

#include "Camera.h"
#include "Shader.h"

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
	float cxThumb, cyThumb, cxIndex= 0.f, cyIndex= 0.f, mappedCxIndex = 0.f, mappedCyIndex = 0.f, cxMiddle, cyMiddle,
		  cxRing, cyRing, cxPinky, cyPinky;
	float deltaX= 0.0f, deltaY= 0.f,tempX= 0.f,tempY = 0.f, mappedTempX = 0.f, mappedTempY = 0.f;
	//flag whether the choosen finger is at the center of the screen
	bool isMiddleized = false, firstTrue = false; bool trueInput;

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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
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
	
	float triangleVertices[] = {
		// Triangle 1
		0.0f, 0.0f, 0.0f,  // Vertex 1
		0.1f, 0.1f, 0.0f,  // Vertex 2
		0.1f, 0.0f, 0.0f,  // Vertex 3

	};

	unsigned int triangleVBO;
	glGenBuffers(1, &triangleVBO);
	glBindBuffer(GL_ARRAY_BUFFER,triangleVBO);
	glBufferData(GL_ARRAY_BUFFER,sizeof(triangleVertices),triangleVertices,GL_DYNAMIC_DRAW);

	unsigned int triangleVAO;
	glGenVertexArrays(1,&triangleVAO);
	glBindVertexArray(triangleVAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3,GL_FLOAT,GL_FALSE,3*(sizeof(float)),(void*)0);

	glBindVertexArray(0);

	Shader triangleShader("triangle.vs", "triangle.fs");
	
	triangleShader.use();
	glBindVertexArray(triangleVAO);
	glm::mat4 model = glm::mat4(1.0f);
	while (!glfwWindowShouldClose(window)) {
		tempX = cxIndex;
		tempY = cyIndex;
		memcpy(&cxIndex, (char*)pBuf + 8, sizeof(float));
		memcpy(&cyIndex, (char*)pBuf + 12, sizeof(float));
		memcpy(&trueInput, (char*)pBuf + 40, sizeof(bool));
		//nice catch:delta resembles the difference between 2 values if the mapping were to be used at the point where delta will be used it may map to 
		// -1 at the time of rendering which would result triangle going right off the screen 
		//For the IO side I am satisfied with the result. Still there is room for optimization and more swift input taking by interpolation?
		// after creating the rigged model(humanoid) I will find ways to increase fps
		if (!firstTrue) { isMiddleized = fabs(cxIndex - 0.5f) < 0.01f && fabs(cyIndex - 0.5f) < 0.01f; }
		if (isMiddleized) 
		{
			mappedTempX = 2.0f * tempX - 1;
			mappedTempY = 2.0f * tempY - 1;
			mappedCxIndex = 2.0f * cxIndex - 1;
			mappedCyIndex = 2.0f * cyIndex - 1;
			deltaX = mappedCxIndex - mappedTempX;
			deltaY = mappedTempY-mappedCyIndex;
			firstTrue = true;
		}
		
		
		
		
		
		
		
		
		std::cout << deltaX << " " << deltaY << std::endl;
		
		
		// per-frame time logic
	    // --------------------
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		// input
		// -----
		deltaX = fabs(deltaX) < 0.008f || !trueInput ? 0 : deltaX;
		deltaY = fabs(deltaY) < 0.008f || !trueInput ? 0 : deltaY;
		processInput(window);
		model = glm::translate(model, glm::vec3(2*deltaX, 1.33*deltaY, 0.0f));
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		triangleShader.setMat4("view", view);
		triangleShader.setMat4("projection", projection);
		triangleShader.setMat4("model",model);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES,0,3);
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