//#define BlinnPhong
#ifdef BlinnPhong

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

Camera camera(glm::vec3(0.0f, 10.0f, 10.0f));

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_pos_callback(GLFWwindow* window, double x, double y);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void loadTexImage(const char* filename);

int main()
{
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


	float vertices[] = {
		-1.0, 0.0, -1.0, 	0.0, 1.0, 0.0,	0.0, 0.0,
		 1.0, 0.0, -1.0,	0.0, 1.0, 0.0,	1.0, 0.0,
		-1.0, 0.0,  1.0,	0.0, 1.0, 0.0,	0.0, 1.0,
		 1.0, 0.0,  1.0,	0.0, 1.0, 0.0,	1.0, 1.0
	};
	unsigned indices[] = {
		0, 2, 1,
		2, 1, 3
	};
	// 绑定顶点数据
	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);

	// 加载纹理
	unsigned int diffuse_texture;
	glGenTextures(1, &diffuse_texture);
	glBindTexture(GL_TEXTURE_2D, diffuse_texture);

	stbi_set_flip_vertically_on_load(true);
	loadTexImage("texture/wood.png");

	// Shader
	Shader PhongShader("shader/vertex_Phong.glsl", "shader/fragment_Phong.glsl");
	Shader BlinnPhongShader("shader/vertex_BlinnPhong.glsl", "shader/fragment_BlinnPhong.glsl");
	Shader* CurrentShader;
	CurrentShader = &PhongShader;
	//CurrentShader = &BlinnPhongShader;

	glEnable(GL_DEPTH_TEST);

	// 光源颜色参数
	glm::vec3 lightPosition = glm::vec3(1.0f, 1.5f, 0.0f); 
	glm::vec3 lightAmbient = glm::vec3(0.05f);            
	glm::vec3 lightDiffuse = glm::vec3(0.8f);             
	glm::vec3 lightSpecular = glm::vec3(0.3f);

	float lightConstant = 1.0f;
	float lightLinear = 0.09f;
	float lightQuadratic = 0.032f;

	float materialShininess = 16.0f;

	camera.Pitch = -30.0f;
	while (!glfwWindowShouldClose(window))
	{
		// 处理输入
		processInput(window);

		static bool Blinn = false;
		static bool isPressKeyb = false;
		if (!isPressKeyb && glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		{
			isPressKeyb = true;
			Blinn = !Blinn;
		}
		if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
			isPressKeyb = false;

		CurrentShader = Blinn ? (&BlinnPhongShader) : (&PhongShader);

		// 初始化新帧

		ImGui_ImplGlfw_NewFrame();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui::NewFrame();
		{
			ImGui::Begin("Parameter Controller");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

			ImGui::Checkbox("Blinn", &Blinn);

			if (ImGui::TreeNodeEx("light", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat3("position", glm::value_ptr(lightPosition), 0.1f);
				ImGui::ColorEdit3("ambient", glm::value_ptr(lightAmbient));
				ImGui::ColorEdit3("diffuse", glm::value_ptr(lightDiffuse));
				ImGui::ColorEdit3("specular", glm::value_ptr(lightSpecular));
				ImGui::Separator();
				ImGui::Text("attenuation");
				ImGui::SliderFloat("constant", &lightConstant, 0.0f, 2.0f, "%.2f");
				ImGui::SliderFloat("linear", &lightLinear, 0.0f, 1.0f, "%.3f");
				ImGui::SliderFloat("quadratic", &lightQuadratic, 0.0f, 1.0f, "%.3f");
				ImGui::TreePop();
			}
			if (ImGui::TreeNodeEx("material", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::SliderFloat("shininess", &materialShininess, 1, 512, "%.0f");
				ImGui::TreePop();
			}
			ImGui::End();
		}

		glClearColor(0., 0., 0., 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	


		// 材质参数配置
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuse_texture);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(10.0));
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
		
		CurrentShader->use();
		CurrentShader->setVector3("light.position", lightPosition);
		CurrentShader->setVector3("light.ambient", lightAmbient);
		CurrentShader->setVector3("light.diffuse", lightDiffuse);
		CurrentShader->setVector3("light.specular", lightSpecular);
		CurrentShader->setFloat("light.constant", lightConstant);
		CurrentShader->setFloat("light.linear", lightLinear);
		CurrentShader->setFloat("light.quadratic", lightQuadratic);

		CurrentShader->setInt("material.diffuse_texture", 0);
		CurrentShader->setFloat("material.shininess", materialShininess);

		CurrentShader->setVector3("cameraPos", camera.Position);
		CurrentShader->setMatrix4("model", model);
		CurrentShader->setMatrix4("view", view);
		CurrentShader->setMatrix4("projection", projection);

		// 渲染
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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

void loadTexImage(const char* filename)
{
	int width, height, nrChannels;
	unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = GL_RGB;
		if (nrChannels == 1) format = GL_RED;
		else if (nrChannels == 3) format = GL_RGB;
		else if (nrChannels == 4) format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture : " << filename << std::endl;
	}
	stbi_image_free(data);
}

#endif