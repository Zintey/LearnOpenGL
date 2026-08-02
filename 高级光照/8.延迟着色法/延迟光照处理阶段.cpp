/*
前向渲染：1000的模型（每个约6000个面），100个灯光下帧率为25左右
延迟渲染：1000的模型（每个约6000个面），100个灯光下帧率为75左右

目前代码存在一个问题：
    在处理gbuffer时写入了lightcube数据
    在后面的lighting阶段，lightcube对应的像素也会参与光照计算
    而且gPosition，gNormal等数据不对应
*/
//#define 延迟光照处理阶段
#ifdef 延迟光照处理阶段

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
#include <model.h>

const int screenWidth = 1920;
const int screenHeight = 1080;
int windowWidth = 1920;
int windowHeight = 1080;

bool isShowCursor = false;

Camera camera(glm::vec3(-7.0f, 12.5f, 49.0f), glm::vec3(0.0, 1.0, 0.0), -23.5, -52.8);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_pos_callback(GLFWwindow* window, double x, double y);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void loadTexImage(const char* filename);

void renderCube();
void renderQuad();

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

    // ImGui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    // Load Model
    stbi_set_flip_vertically_on_load(true);
    Model itemModel("objects/nanosuit_reflection/nanosuit.obj");

    // Shader
    Shader GBufferShader("shader/get_Gbuffer.vert", "shader/get_Gbuffer.frag");
    Shader deferredShader("shader/deferred.vert", "shader/deferred.frag");
    Shader lightCubeShader("shader/vertex_light_cube_shader.glsl", "shader/fragment_light_cube_shader.glsl");


    // Spawn Model modelmatrix Data
    srand(time(0));
    constexpr int ITEM_CNT = 1000;
    constexpr float ITEM_SPAWN_X_RANGE = 50;
    constexpr float ITEM_SPAWN_Z_RANGE = 50;
    glm::mat4 itemModelMatrixs[ITEM_CNT];
    for (int i = 0; i < ITEM_CNT; i++)
    {
        itemModelMatrixs[i] = glm::mat4(1.0);
        glm::vec3 position = glm::vec3(0.0f);
        position.x = (rand() % int(10 * ITEM_SPAWN_X_RANGE)) / 10;
        position.z = (rand() % int(10 * ITEM_SPAWN_Z_RANGE)) / 10;
        itemModelMatrixs[i] = glm::translate(itemModelMatrixs[i], position);
        itemModelMatrixs[i] = glm::scale(itemModelMatrixs[i], glm::vec3(0.2));
        float angle = rand() % 360;
        itemModelMatrixs[i] = glm::rotate(itemModelMatrixs[i], angle, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    itemModel.SetInstancedData(itemModelMatrixs, ITEM_CNT);


    // Spawn Light Cube Data
    constexpr int LIGHT_CNT = 100;
    constexpr float LIGHT_SPAWN_X_RANGE = 50;
    constexpr float LIGHT_SPAWN_Y_RANGE_UP = 4;
    constexpr float LIGHT_SPAWN_Y_RANGE_DOWN = 0;
    constexpr float LIGHT_SPAWN_Z_RANGE = 50;
    struct PointLight {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 ambient = glm::vec3(0.05f);
        glm::vec3 diffuse = glm::vec3(0.8f);
        glm::vec3 specular = glm::vec3(1.0f);
        float constant = 1.0f;
        float linear = 0.05f;
        float quadratic = 0.032f;
    };
    PointLight light[LIGHT_CNT];
    for (int i = 0; i < LIGHT_CNT; i++)
    {
        glm::vec3 position = glm::vec3(0.0f);
        position.x = (rand() % int(10 * LIGHT_SPAWN_X_RANGE)) / 10.f;
        position.y = (rand() % int(10 * (LIGHT_SPAWN_Y_RANGE_UP - LIGHT_SPAWN_Y_RANGE_DOWN))) / 10.f + LIGHT_SPAWN_Y_RANGE_DOWN;
        position.z = (rand() % int(10 * LIGHT_SPAWN_Z_RANGE)) / 10.f;
        light[i].position = position;
        glm::vec3 color = glm::vec3(0.0f);
        color.x = (rand() % 256) / 256.f;
        color.y = (rand() % 256) / 256.f;
        color.z = (rand() % 256) / 256.f;
        light[i].ambient = color * glm::float32(0.01f);
        light[i].diffuse = color * glm::float32(0.8f);
        light[i].specular = color * glm::float32(1.0f);
    }

    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedoSpec;
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    deferredShader.use();
    deferredShader.setInt("gPosition", 0);
    deferredShader.setInt("gNormal", 1);
    deferredShader.setInt("gAlbedoSpec", 2);

    int enable_light_cnt = 10;
    int show_mode = 0;
    camera.MovementSpeed = 10.0;

    while (!glfwWindowShouldClose(window))
    {
        // Process input
        processInput(window);

        // Start new frame
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Parameter Controller");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            if (ImGui::TreeNodeEx("camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("position", glm::value_ptr(camera.Position), 0.1f);
                ImGui::DragFloat("pitch", &camera.Pitch, 0.01f);
                ImGui::DragFloat("yaw", &camera.Yaw, 0.01f);
                ImGui::DragFloat("speed", &camera.MovementSpeed, 0.01f);
                ImGui::TreePop();
            }
            const char* modes[] = { "lighting", "position", "normal", "abedo", "spec"};
            if (ImGui::Combo("show mode", &show_mode, modes, IM_ARRAYSIZE(modes))) {
                ImGui::Text("mode: %s", modes[show_mode]);
            }

            ImGui::SliderInt("enable_light_cnt", &enable_light_cnt, 1, LIGHT_CNT);
            ImGui::End();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.f, 0.f, 0.f, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
            GBufferShader.use();
            GBufferShader.setMatrix4("view", view);
            GBufferShader.setMatrix4("projection", projection);
            itemModel.DrawInstanced(GBufferShader, ITEM_CNT);

            lightCubeShader.use();
            lightCubeShader.setMatrix4("view", view);
            lightCubeShader.setMatrix4("projection", projection);
            for (int i = 0; i < LIGHT_CNT; i++)
            {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, light[i].position);
                model = glm::scale(model, glm::vec3(0.1f));
                lightCubeShader.setMatrix4("model", model);
                lightCubeShader.setVector3("lightColor", light[i].diffuse);
                renderCube();
            }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.f, 0.f, 0.f, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
            deferredShader.use();
            deferredShader.setInt("show_mode", show_mode);
            deferredShader.setVector3("cameraPos", camera.Position);
            deferredShader.setInt("enable_light_cnt", enable_light_cnt);
            for (int i = 0; i < enable_light_cnt; i++)
            {
                std::string name = "light[" + std::to_string(i) + "].";
                deferredShader.setVector3(name + "position", light[i].position);
                deferredShader.setVector3(name + "ambient", light[i].ambient);
                deferredShader.setVector3(name + "diffuse", light[i].diffuse);
                deferredShader.setVector3(name + "specular", light[i].specular);
                deferredShader.setFloat(name + "constant", light[i].constant);
                deferredShader.setFloat(name + "linear", light[i].linear);
                deferredShader.setFloat(name + "quadratic", light[i].quadratic);
            }
            renderQuad();

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

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
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
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderQuad()
{
    // Create quad
    static unsigned int VAO = 0;
    static unsigned int VBO;
    if (VAO == 0)
    {
        float vertices[] = {
            // pos       // uv
            -1,  1, 0,   0,1,
            -1, -1, 0,   0,0,
             1, -1, 0,   1,0,

            -1,  1, 0,   0,1,
             1, -1, 0,   1,0,
             1,  1, 0,   1,1
        };
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // texcoords
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    windowWidth = width;
    windowHeight = height;
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
