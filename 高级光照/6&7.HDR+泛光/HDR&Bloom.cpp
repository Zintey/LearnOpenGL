//#define HDR_AND_BLOOM
#ifdef HDR_AND_BLOOM

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
int windowWidth = 1920;
int windowHeight = 1080;

bool isShowCursor = false;

Camera camera(glm::vec3(0.0f, 10.0f, 0.0f));

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

    // Load textures
    stbi_set_flip_vertically_on_load(true);

    unsigned int diffuse_texture;
    glGenTextures(1, &diffuse_texture);
    glBindTexture(GL_TEXTURE_2D, diffuse_texture);
    loadTexImage("texture/bricks2.jpg");

    // Shader
    Shader lightCubeShader("shader/HDR_LightCube.vert", "shader/HDR_LightCube.frag");
    Shader lightingShader("shader/lighting.vert", "shader/lighting.frag");
    Shader blurShader("shader/hdr.vert", "shader/blur.frag");
    Shader bloomFinalShader("shader/hdr.vert", "shader/bloom_final.frag");
    Shader hdrShader("shader/hdr.vert", "shader/hdr.frag");

    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    
    unsigned int colorBuffer[2];
    glGenTextures(2, colorBuffer);
    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffer[i], 0);
    }
    unsigned rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Ping-pong FBOs for gaussian blur (bloom)
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Pingpong FBO " << i << " not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    glEnable(GL_DEPTH_TEST);

    // Light parameters
    float lightIntensity = 10.f;
    glm::vec3 lightPosition = glm::vec3(5.0f, 1.5f, -12.0f);
    glm::vec3 lightAmbient = glm::vec3(0.05f);
    glm::vec3 lightDiffuse = glm::vec3(0.8f);
    glm::vec3 lightSpecular = glm::vec3(0.3f);

    float lightConstant = 1.0f;
    float lightLinear = 0.09f;
    float lightQuadratic = 0.032f;

    float materialShininess = 16.0f;
    float exposure = 1.0;

    camera.Pitch = -30.0f;

    // Bloom settings
    bool bloomEnabled = true;
    float bloomThreshold = 1.0f;
    int blurAmount = 5;

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

            ImGui::DragFloat("exposure", &exposure, 0.01, 0.0, 10.0);
            ImGui::Separator();
            ImGui::Text("Bloom");
            ImGui::Checkbox("bloom", &bloomEnabled);
            ImGui::DragFloat("threshold", &bloomThreshold, 0.01, 0.0, 5.0);
            ImGui::SliderInt("blur passes", &blurAmount, 1, 20);

            if (ImGui::TreeNodeEx("light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                
                ImGui::DragFloat("lightIntensity", &lightIntensity, 0.1f);
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

        // 1. 渲染到 HDR FBO上
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClearColor(0., 0., 0., 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuse_texture);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(9.0, -4, -8.0));
        model = glm::scale(model, glm::vec3(3.0));
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

        lightingShader.use();
        lightingShader.setVector3("light.position", lightPosition);
        lightingShader.setVector3("light.ambient", lightAmbient);
        lightingShader.setVector3("light.diffuse", lightDiffuse);
        lightingShader.setVector3("light.specular", lightSpecular);
        lightingShader.setFloat("light.constant", lightConstant);
        lightingShader.setFloat("light.linear", lightLinear);
        lightingShader.setFloat("light.quadratic", lightQuadratic);

        lightingShader.setInt("material.diffuse_texture", 0);
        lightingShader.setFloat("material.shininess", materialShininess);

        lightingShader.setVector3("cameraPos", camera.Position);
        lightingShader.setMatrix4("model", model);
        lightingShader.setMatrix4("view", view);
        lightingShader.setMatrix4("projection", projection);
        lightingShader.setFloat("bloomThreshold", bloomThreshold);

        renderCube();

        glm::vec3 cubePositions[] = {
            glm::vec3(-3.0f, -1.0f, -5.0f),
            glm::vec3( 0.0f, -2.0f,  0.0f),
            glm::vec3( 4.0f, -2.0f, -3.0f),
            glm::vec3(-5.0f, -1.5f,  2.0f)
        };
        for (unsigned int i = 0; i < 4; i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            model = glm::scale(model, glm::vec3(0.8f));
            lightingShader.setMatrix4("model", model);
            renderCube();
        }

        lightCubeShader.use();
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPosition);
        model = glm::scale(model, glm::vec3(0.3f));
        lightCubeShader.setVector3("lightColor", glm::vec3(lightIntensity));
        lightCubeShader.setFloat("bloomThreshold", bloomThreshold);
        lightCubeShader.setMatrix4("model", model);
        lightCubeShader.setMatrix4("view", view);
        lightCubeShader.setMatrix4("projection", projection);
        renderCube();

        // 2. 高斯模糊高亮区域
        bool horizontal = true;
        bool first_iteration = true;
        blurShader.use();
        glActiveTexture(GL_TEXTURE0);
        for (int i = 0; i < blurAmount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            glClear(GL_COLOR_BUFFER_BIT);
            blurShader.setBool("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffer[1] : pingpongColorbuffers[!horizontal]);
            renderQuad();
            horizontal = !horizontal;
            first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClearColor(0., 0., 0., 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        int halfW = windowWidth / 2;
        int halfH = windowHeight / 2;

        // Top-left: Scene color (tone mapped)
        glViewport(0, halfH, halfW, halfH);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer[0]);
        hdrShader.use();
        hdrShader.setInt("tex", 0);
        hdrShader.setFloat("exposure", exposure);
        renderQuad();

        // Top-right: Bright areas
        glViewport(halfW, halfH, halfW, halfH);
        glBindTexture(GL_TEXTURE_2D, colorBuffer[1]);
        renderQuad();

        // Bottom: Final composite (full width)
        // 3. 把高斯模糊过的高亮区域 与 原帧缓冲 混合并做hdr处理
        glViewport(0, 0, halfW, halfH);
        bloomFinalShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        bloomFinalShader.setInt("scene", 0);
        bloomFinalShader.setInt("bloomBlur", 1);
        bloomFinalShader.setFloat("exposure", exposure);
        bloomFinalShader.setBool("bloom", bloomEnabled);
        renderQuad();


        glViewport(0, 0, windowWidth, windowHeight);
        glEnable(GL_DEPTH_TEST);

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
