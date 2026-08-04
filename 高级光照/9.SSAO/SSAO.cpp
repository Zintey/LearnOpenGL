/*
*/
//#define ScreenSpaceAmbientOcclusion
#ifdef ScreenSpaceAmbientOcclusion

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
#include <random>

#include <camera.h>
#include <shader.h>
#include <model.h>

const int screenWidth = 1920;
const int screenHeight = 1080;
int windowWidth = 1920;
int windowHeight = 1080;

bool isShowCursor = false;

// 场景贴图
unsigned int texWhite = 0; // 1x1 白色，场景全部用白模
unsigned int texBlack = 0; // 1x1 黑色，用来关掉高光

Camera camera(glm::vec3(1.5f, 3.0f, 7.0f), glm::vec3(0.0, 1.0, 0.0), -18.0, -99.8);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_pos_callback(GLFWwindow* window, double x, double y);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void loadTexImage(const char* filename);

void renderCube();
void renderQuad();
void drawTexturedCube(Shader& shader, const glm::mat4& model, unsigned int diffuseTex);

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
    Shader GBufferShader("shader/SSAO_Gbuffer.vert", "shader/SSAO_Gbuffer.frag");
    Shader deferredShader("shader/SSAO_deferred.vert", "shader/SSAO_deferred.frag");
    Shader lightCubeShader("shader/vertex_light_cube_shader.glsl", "shader/fragment_light_cube_shader.glsl");
    Shader SSAOShader("shader/SSAO.vert", "shader/SSAO.frag");
    Shader blurShader("shader/SSAO_blur.vert", "shader/SSAO_blur.frag");

    // 光源
    constexpr int LIGHT_CNT = 4;
    struct PointLight {
        glm::vec3 position;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float constant;
        float linear;
        float quadratic;
    };
    PointLight light[LIGHT_CNT] = {
        { glm::vec3(3.0f, 2.0f, 2.0f),    glm::vec3(0.10f, 0.03f, 0.03f), glm::vec3(1.0f, 0.3f, 0.3f), glm::vec3(1.0f), 1.0f, 0.09f, 0.032f },
        { glm::vec3(-3.0f, 2.5f, -2.0f),  glm::vec3(0.03f, 0.04f, 0.10f), glm::vec3(0.3f, 0.4f, 1.0f), glm::vec3(1.0f), 1.0f, 0.09f, 0.032f },
        { glm::vec3(0.0f, 3.0f, -3.0f),   glm::vec3(0.03f, 0.10f, 0.03f), glm::vec3(0.3f, 1.0f, 0.3f), glm::vec3(1.0f), 1.0f, 0.09f, 0.032f },
        { glm::vec3(0.0f, 1.5f, 3.0f),    glm::vec3(0.10f, 0.10f, 0.03f), glm::vec3(1.0f, 0.8f, 0.3f), glm::vec3(1.0f), 1.0f, 0.09f, 0.032f },
    };

    bool lightEnabled[LIGHT_CNT] = { true, true, true, true };

    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedoSpec;
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL); // RGBA16F：alpha 通道存线性深度，供 SSAO 读取
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

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ---- SSAO ----
    // 半球采样核
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = (float)i / 64.0f;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // 随机旋转噪声纹理（4x4）
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
        ssaoNoise.push_back(noise);
    }
    unsigned int noiseTexture;
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // SSAO framebuffer
    unsigned int ssaoFBO, ssaoColorBuffer;
    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // SSAO blur framebuffer
    unsigned int ssaoBlurFBO, ssaoBlurColorBuffer;
    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoBlurColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // blur framebuffer（多次模糊 ping-pong 用）
    unsigned int ssaoBlurFBO2, ssaoBlurColorBuffer2;
    glGenFramebuffers(1, &ssaoBlurFBO2);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO2);
    glGenTextures(1, &ssaoBlurColorBuffer2);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurColorBuffer2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurColorBuffer2, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    deferredShader.use();
    deferredShader.setInt("gPosition", 0);
    deferredShader.setInt("gNormal", 1);
    deferredShader.setInt("gAlbedoSpec", 2);
    deferredShader.setInt("ssao", 3);

    SSAOShader.use();
    SSAOShader.setInt("gPositionDepth", 0);
    SSAOShader.setInt("gNormal", 1);
    SSAOShader.setInt("texNoise", 2);
    for (unsigned int i = 0; i < 64; ++i)
        SSAOShader.setVector3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);

    blurShader.use();
    blurShader.setInt("ssaoInput", 0);

    // 场景贴图
    unsigned char whitePixel[] = { 255, 255, 255, 255 };
    glGenTextures(1, &texWhite);
    glBindTexture(GL_TEXTURE_2D, texWhite);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

    unsigned char blackPixel[] = { 0, 0, 0, 255 };
    glGenTextures(1, &texBlack);
    glBindTexture(GL_TEXTURE_2D, texBlack);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blackPixel);

    int show_mode = 0;
    int kernelCnt = 64;
    float sampleRadius = 1.0f;
    int noiseScale = 4;     
    int blurPasses = 1;     
    bool useSSAO = true;    
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
            const char* modes[] = { "lighting", "position", "normal", "abedo", "spec", "ao" };
            if (ImGui::Combo("show mode", &show_mode, modes, IM_ARRAYSIZE(modes))) {
                ImGui::Text("mode: %s", modes[show_mode]);
            }

            ImGui::SliderInt("SSAO kernelCnt", &kernelCnt, 1, 64);
            ImGui::SliderFloat("SSAO sampleRadius", &sampleRadius, 0.1f, 5.0f);
            ImGui::SliderInt("SSAO noiseScale", &noiseScale, 1, 16);
            ImGui::SliderInt("SSAO blurPasses", &blurPasses, 0, 8);
            ImGui::Checkbox("use SSAO", &useSSAO);
            for (int i = 0; i < LIGHT_CNT; i++)
            {
                std::string label = "light " + std::to_string(i) + " enabled";
                ImGui::Checkbox(label.c_str(), &lightEnabled[i]);
            }
            ImGui::End();
        }

        // render GBuffer
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.f, 0.f, 0.f, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
        GBufferShader.use();
        GBufferShader.setMatrix4("view", view);
        GBufferShader.setMatrix4("projection", projection);

        // 地板
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.05f, 0.0f));
        model = glm::scale(model, glm::vec3(10.0f, 0.1f, 10.0f));
        drawTexturedCube(GBufferShader, model, texWhite);

        // 箱子
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.5f, 1.0f));
        drawTexturedCube(GBufferShader, model, texWhite);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.0f, 0.5f, -1.0f));
        drawTexturedCube(GBufferShader, model, texWhite);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.0f, 0.5f, -2.0f));
        model = glm::scale(model, glm::vec3(0.5f, 1.0f, 0.5f));
        drawTexturedCube(GBufferShader, model, texWhite);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.0f, 0.25f, 2.0f));
        model = glm::scale(model, glm::vec3(1.5f, 0.5f, 1.5f));
        drawTexturedCube(GBufferShader, model, texWhite);

        // 墙面
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 2.0f, -4.0f));
        model = glm::scale(model, glm::vec3(8.0f, 4.0f, 0.2f));
        drawTexturedCube(GBufferShader, model, texWhite);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-4.0f, 2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2f, 4.0f, 8.0f));
        drawTexturedCube(GBufferShader, model, texWhite);

        // 人形模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2f));
        GBufferShader.setMatrix4("model", model);
        GBufferShader.setInt("forceWhite", 1); // 白模：忽略模型自带贴图
        itemModel.Draw(GBufferShader);
        GBufferShader.setInt("forceWhite", 0);

        // SSAO pass
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        SSAOShader.use();
        SSAOShader.setMatrix4("view", view);
        SSAOShader.setMatrix4("projection", projection);
        SSAOShader.setInt("kernelCnt", kernelCnt);
        SSAOShader.setFloat("sampleRadius", sampleRadius);
        SSAOShader.setVector2("texNoiseScale", glm::vec2((float)screenWidth / noiseScale, (float)screenHeight / noiseScale));
        renderQuad();

        // SSAO blur pass
        unsigned int finalAO;
        if (blurPasses <= 0)
        {
            finalAO = ssaoColorBuffer;
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
            blurShader.use();
            renderQuad();
            for (int i = 1; i < blurPasses; ++i)
            {
                if (i % 2 == 1)
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO2);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, ssaoBlurColorBuffer);
                    renderQuad();
                }
                else
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, ssaoBlurColorBuffer2);
                    renderQuad();
                }
            }
            finalAO = (blurPasses % 2 == 1) ? ssaoBlurColorBuffer : ssaoBlurColorBuffer2;
        }

        // deferred lighting (render quad)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.f, 0.f, 0.f, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, finalAO);
        deferredShader.use();
        deferredShader.setInt("show_mode", show_mode);
        deferredShader.setInt("useSSAO", useSSAO ? 1 : 0);
        for (int i = 0; i < LIGHT_CNT; i++)
            deferredShader.setInt("lightEnabled[" + std::to_string(i) + "]", lightEnabled[i] ? 1 : 0);
        deferredShader.setVector3("cameraPos", camera.Position);
        for (int i = 0; i < LIGHT_CNT; i++)
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

        // forward rendering light cube
        if (show_mode == 0)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            lightCubeShader.use();
            lightCubeShader.setMatrix4("view", view);
            lightCubeShader.setMatrix4("projection", projection);
            for (int i = 0; i < LIGHT_CNT; i++)
            {
                if (!lightEnabled[i]) continue;
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, light[i].position);
                model = glm::scale(model, glm::vec3(0.1f));
                lightCubeShader.setMatrix4("model", model);
                lightCubeShader.setVector3("lightColor", light[i].diffuse);
                renderCube();
            }
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
    return 0;
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;


void drawTexturedCube(Shader& shader, const glm::mat4& model, unsigned int diffuseTex)
{
    shader.use();
    shader.setMatrix4("model", model);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseTex);
    shader.setInt("material.texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texBlack);
    shader.setInt("material.texture_specular1", 1);
    renderCube();
}

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
