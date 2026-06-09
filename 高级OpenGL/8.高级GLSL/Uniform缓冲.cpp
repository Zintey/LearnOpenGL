//#define Uniform»º³å
#ifdef Uniform»º³å
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <shader.h>
#include <camera.h>

const int screenWidth = 1920;
const int screenHeight = 1080;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_pos_callback(GLFWwindow* window, double x, double y);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Learn OpenGL - UBO Assignment", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to Create GLFW Window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetScrollCallback(window, scroll_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to Initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	Shader shaderRed("shader/vertex_advanced_glsl_shader.glsl", "shader/fragment_advanced_glsl_shader.glsl");
	shaderRed.use();
	shaderRed.setVector3("color", glm::vec3(1.0f, 0.0f, 0.0f));

	Shader shaderGreen("shader/vertex_advanced_glsl_shader.glsl", "shader/fragment_advanced_glsl_shader.glsl");
	shaderGreen.use();
	shaderGreen.setVector3("color", glm::vec3(0.0f, 1.0f, 0.0f));

	Shader shaderBlue("shader/vertex_advanced_glsl_shader.glsl", "shader/fragment_advanced_glsl_shader.glsl");
	shaderBlue.use();
	shaderBlue.setVector3("color", glm::vec3(0.0f, 0.0f, 1.0f));

	Shader shaderYellow("shader/vertex_advanced_glsl_shader.glsl", "shader/fragment_advanced_glsl_shader.glsl");
	shaderYellow.use();
	shaderYellow.setVector3("color", glm::vec3(1.0f, 1.0f, 0.0f));

	float cubeVertices[] = {
		-0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f
	};

	unsigned int cubeVAO, cubeVBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);


	unsigned int uBlockRed = glGetUniformBlockIndex(shaderRed.ID, "Matrices");
	unsigned int uBlockGreen = glGetUniformBlockIndex(shaderGreen.ID, "Matrices");
	unsigned int uBlockBlue = glGetUniformBlockIndex(shaderBlue.ID, "Matrices");
	unsigned int uBlockYellow = glGetUniformBlockIndex(shaderYellow.ID, "Matrices");

	glUniformBlockBinding(shaderRed.ID, uBlockRed, 0);
	glUniformBlockBinding(shaderGreen.ID, uBlockGreen, 0);
	glUniformBlockBinding(shaderBlue.ID, uBlockBlue, 0);
	glUniformBlockBinding(shaderYellow.ID, uBlockYellow, 0);

	unsigned int uboMatrices;
	glGenBuffers(1, &uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));

	while (!glfwWindowShouldClose(window))
	{
		processInput(window); 

		glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.f);

		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glBindVertexArray(cubeVAO);

		shaderRed.use();
		glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.75f, 0.75f, 0.0f));
		shaderRed.setMatrix4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		shaderGreen.use();
		model = glm::translate(glm::mat4(1.0f), glm::vec3(0.75f, 0.75f, 0.0f));
		shaderGreen.setMatrix4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		shaderYellow.use();
		model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.75f, -0.75f, 0.0f));
		shaderYellow.setMatrix4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		shaderBlue.use();
		model = glm::translate(glm::mat4(1.0f), glm::vec3(0.75f, -0.75f, 0.0f));
		shaderBlue.setMatrix4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &uboMatrices);

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
	static float lastTime = 0.0f;
	float deltaTime = glfwGetTime() - lastTime;
	lastTime = glfwGetTime();

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::DOWN, deltaTime);
}

void cursor_pos_callback(GLFWwindow* window, double x, double y)
{
	static float lastX = 400.0f, lastY = 300.0f;
	static bool firstMouse = true;
	if (firstMouse) {
		lastX = x; lastY = y; firstMouse = false;
	}
	float offsetX = x - lastX;
	float offsetY = y - lastY;
	lastX = x;
	lastY = y;
	camera.ProcessMouseMovement(offsetX, -offsetY);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

#endif