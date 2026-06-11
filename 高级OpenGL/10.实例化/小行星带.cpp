//#define 小行星带
#ifdef 小行星带
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <iostream>
#include <vector>
#include <shader.h>
#include <camera.h>
#include <model.h>

const int screenWidth = 1920;
const int screenHeight = 1080;

bool isShowCursor = false;

Camera camera(glm::vec3(-150.0f, 50.0f, 150.0f));

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

	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Learn OpenGL", NULL, NULL);
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

	// imgui 初始化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	stbi_set_flip_vertically_on_load(true);
	Model planetModel("objects/planet/planet.obj");
	Model rockModel("objects/rock/rock.obj");

	Shader plantShader("shader/vertex_小行星带_planet.glsl", "shader/fragment_小行星带_planet.glsl");
	Shader rockShader("shader/vertex_小行星带_rock.glsl", "shader/fragment_小行星带_rock.glsl");

	unsigned rockAmount = 100000;
	glm::mat4* rockModels = new glm::mat4[rockAmount];
	srand(glfwGetTime());
	float radius = 150.0;
	float offset = 25.0f;
	for (int i = 0; i < rockAmount; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		float angle = (float)i / (float)rockAmount * 360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float y = displacement * 0.4f;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;
		model = glm::translate(model, glm::vec3(x, y, z));

		float scale = (rand() % 20) / 100.f + 0.05;
		model = glm::scale(model, glm::vec3(scale));

		float rotateAngle = rand() % 360;
		model = glm::rotate(model, rotateAngle, glm::vec3(0.3f, 0.4f, 0.2f));

		rockModels[i] = model;
	}

	unsigned int modelBuffer;
	glGenBuffers(1, &modelBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, modelBuffer);
	glBufferData(GL_ARRAY_BUFFER, rockAmount * sizeof(glm::mat4), &rockModels[0], GL_STATIC_DRAW);

	for (unsigned int i = 0; i < rockModel.meshes.size(); i++)
	{
		unsigned int VAO = rockModel.meshes[i].GetVAO();
		glBindVertexArray(VAO);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(1 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);
		glBindVertexArray(0);
	}

	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		{
			ImGui::Begin("Parameter Controller");

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 500.f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(8.f));
		plantShader.use();
		plantShader.setMatrix4("model", model);
		plantShader.setMatrix4("view", view);
		plantShader.setMatrix4("projection", projection);

		planetModel.Draw(plantShader);
		
		rockShader.use();
		rockShader.setMatrix4("view", view);
		rockShader.setMatrix4("projection", projection);
		for (unsigned int i = 0; i < rockModel.meshes.size(); i++)
		{
			glBindVertexArray(rockModel.meshes[i].GetVAO());
			glDrawElementsInstanced(GL_TRIANGLES, rockModel.meshes[i].indices.size(), GL_UNSIGNED_INT, 0, rockAmount);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	delete[] rockModels;
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

	static bool key_tab_lock = false;
	if (!key_tab_lock && (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS))
	{
		key_tab_lock = true;
		isShowCursor = !isShowCursor;
		glfwSetInputMode(window, GLFW_CURSOR, isShowCursor ? GLFW_CURSOR_CAPTURED : GLFW_CURSOR_DISABLED);
	}
	if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE)
		key_tab_lock = false;

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
	if (isShowCursor && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS) return;
	camera.ProcessMouseMovement(offsetX, -offsetY);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (isShowCursor && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS) return;
	camera.ProcessMouseScroll(yoffset);
}

#endif