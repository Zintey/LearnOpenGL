//#define 渲染阴影
#ifdef 渲染阴影

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <iostream>

#include <camera.h>
#include <shader.h>

const int screenWidth = 1920;
const int screenHeight = 1080;

bool isShowCursor = false;

Camera camera(glm::vec3(-2.0f, 10.0f, 15.0f));
glm::vec3 lightPosition = glm::vec3(-2.0f, 4.0f, -1.0f);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_pos_callback(GLFWwindow* window, double x, double y);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void loadTexImage(const char* filename, bool gammaCorrection);


void renderScene(Shader& shader);
void renderPlane();
void renderCube();
void renderQuad();

int main()
{
#pragma region init
	// glfw 初始化
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Learn OpenGL", NULL, NULL);
	if (window == nullptr)
	{
		std::cout << "Failed to Create GLFW Window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	// 绑定回调函数
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// glad 初始化
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
#pragma endregion

	// 配置深度图FBO
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glReadBuffer(GL_NONE);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 加载纹理
	unsigned int diffuse_texture;
	glGenTextures(1, &diffuse_texture);
	glBindTexture(GL_TEXTURE_2D, diffuse_texture);
	stbi_set_flip_vertically_on_load(true);
	loadTexImage("texture/wood.png", false);

	// 加载gamma修正纹理
	unsigned int diffuse_texture_gamma_correction;
	glGenTextures(1, &diffuse_texture_gamma_correction);
	glBindTexture(GL_TEXTURE_2D, diffuse_texture_gamma_correction);
	loadTexImage("texture/wood.png", true);

	Shader simpleDepthShader("shader/shadow_mapping_depth.vs", "shader/shadow_mapping_depth.fs");
	Shader shader("shader/shadow.vs", "shader/shadow.fs");
	Shader quadShader("shader/quad.vs", "shader/quad.fs");

	glEnable(GL_DEPTH_TEST);

	// 光源颜色参数
	glm::vec3 lightAmbient = glm::vec3(0.05f);
	glm::vec3 lightDiffuse = glm::vec3(0.8f);
	glm::vec3 lightSpecular = glm::vec3(0.3f);
	float materialShininess = 64.0f;

	float baseBias = 0.003;
	bool frontCulling = false;

	float lightOrtho = 5.0;
	bool pcf = false;
	int pcf_level = 2;
	camera.Pitch = -30.0f;
	glEnable(GL_CULL_FACE);
	while (!glfwWindowShouldClose(window))
	{
		// 处理输入
		processInput(window);

		ImGui_ImplGlfw_NewFrame();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui::NewFrame();
		{
			ImGui::Begin("Parameter Controller");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::DragFloat3("Light position", glm::value_ptr(lightPosition), 0.1f);
			ImGui::DragFloat("shadow bias", &baseBias, 0.00001);
			ImGui::Checkbox("Front Culling", &frontCulling);
			ImGui::DragFloat("lightOrtho", &lightOrtho, 0.2);
			ImGui::Checkbox("enable pcf", &pcf);
			ImGui::DragInt("pcf level", &pcf_level, 1, 0, 100);
			ImGui::End();
		}

		// 光源空间矩阵配置
		float near_plane = 0.1f, far_plane = 100.0f;
		glm::mat4 lightProjection = glm::ortho(-lightOrtho, lightOrtho, -lightOrtho, lightOrtho, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(lightPosition, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		// 渲染场景至深度贴图
		simpleDepthShader.use();
		simpleDepthShader.setMatrix4("lightSpaceMatrix", lightSpaceMatrix);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		// ---------- Plane ----------
		glCullFace(GL_BACK);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(10.0f));
		simpleDepthShader.setMatrix4("model", model);
		renderPlane();

		// ---------- Cubes ----------
		if (frontCulling)
			glCullFace(GL_FRONT);
		else
			glCullFace(GL_BACK);

		// Cube1
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.5f, 1.0f));
		model = glm::scale(model, glm::vec3(0.5f));
		simpleDepthShader.setMatrix4("model", model);
		renderCube();

		// Cube2
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.5f, 1.5f));
		model = glm::scale(model, glm::vec3(0.5f));
		simpleDepthShader.setMatrix4("model", model);
		renderCube();

		// Cube3
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 2.0f));
		model = glm::rotate(model,
			glm::radians(60.0f),
			glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
		model = glm::scale(model, glm::vec3(0.25f));
		simpleDepthShader.setMatrix4("model", model);
		renderCube();

		glCullFace(GL_BACK);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 渲染到屏幕
		glViewport(0, 0, screenWidth, screenHeight);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		shader.setMatrix4("projection", projection);
		shader.setMatrix4("view", view);
		shader.setMatrix4("lightSpaceMatrix", lightSpaceMatrix);

		shader.setVector3("cameraPos", camera.Position);
		glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f) - lightPosition);
		shader.setVector3("light.direction", lightDir);
		shader.setVector3("light.ambient", lightAmbient);
		shader.setVector3("light.diffuse", lightDiffuse);
		shader.setVector3("light.specular", lightSpecular);
		shader.setFloat("material.shininess", materialShininess);
		shader.setFloat("baseBias", baseBias);
		shader.setBool("pcf", pcf);
		shader.setInt("pcf_level", pcf_level);

		shader.setInt("material.diffuse_texture", 0);
		shader.setInt("shadowMap", 1); 

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuse_texture_gamma_correction);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);

		renderScene(shader);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}


void renderScene(Shader& shader)
{
	// 地板
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(10.0));
	shader.setMatrix4("model", model);
	renderPlane();

	// 立方体 1
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 1.5f, 1.0));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.setMatrix4("model", model);
	renderCube();

	// 立方体 2
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.5f, 1.5));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.setMatrix4("model", model);
	renderCube();

	// 立方体 3
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 2.0));
	model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	model = glm::scale(model, glm::vec3(0.25));
	shader.setMatrix4("model", model);
	renderCube();


	// Light Cube
	model = glm::mat4(1.0f);
	model = glm::translate(model, lightPosition);
	model = glm::scale(model, glm::vec3(0.1f));
	shader.setMatrix4("model", model);
	renderCube();
}

unsigned int planeVAO = 0;
unsigned int planeVBO = 0;
unsigned int planeEBO = 0;

void renderPlane()
{
	if (planeVAO == 0)
	{

		float vertices[] = {
			-1.0, 0.0, -1.0, 	0.0, 1.0, 0.0,	0.0, 0.0,
			 1.0, 0.0, -1.0,	0.0, 1.0, 0.0,	1.0, 0.0,
			-1.0, 0.0,  1.0,	0.0, 1.0, 0.0,	0.0, 1.0,
			 1.0, 0.0,  1.0,	0.0, 1.0, 0.0,	1.0, 1.0
		};
		unsigned int indices[] = {
			0, 2, 1,
			1, 2, 3
		};
		// 绑定顶点数据
		glGenVertexArrays(1, &planeVAO);
		glBindVertexArray(planeVAO);
		glGenBuffers(1, &planeVBO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glGenBuffers(1, &planeEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glBindVertexArray(0);
	}
	glBindVertexArray(planeVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;

void renderCube()
{
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		// 绑定顶点数据
		glGenVertexArrays(1, &cubeVAO);
		glBindVertexArray(cubeVAO);
		glGenBuffers(1, &cubeVBO);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
	}
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

unsigned int quadVAO = 0;
unsigned int quadVBO = 0;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float vertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		glGenVertexArrays(1, &quadVAO);
		glBindVertexArray(quadVAO);

		glGenBuffers(1, &quadVBO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

		glBindVertexArray(0);
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
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
	if (isShowCursor) return;
	//if (isShowCursor && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS) return;
	camera.ProcessMouseMovement(offsetX, -offsetY);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (isShowCursor && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS) return;
	camera.ProcessMouseScroll(yoffset);
}

void loadTexImage(const char* filename, bool gammaCorrection)
{
	int width, height, nrChannels;
	unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum internalFormat;
		GLenum dataFormat;
		if (nrChannels == 1) internalFormat = dataFormat = GL_RED;
		else if (nrChannels == 3)
		{
			dataFormat = GL_RGB;
			internalFormat = gammaCorrection ? GL_SRGB : dataFormat;
		}
		else if (nrChannels == 4)
		{
			dataFormat = GL_RGBA;
			internalFormat = gammaCorrection ? GL_SRGB_ALPHA : dataFormat;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture : " << filename << std::endl;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(data);
}

#endif