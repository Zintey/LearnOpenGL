//#define 使用封装好的Shader类
#ifdef 使用封装好的Shader类
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <shader.h>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

int main()
{
    // 初始化 GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "Learn OpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 绑定 opengl 函数
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //Shader ourShader("shader/vertex_shader.glsl", "shader/fragment_shader.glsl");
    //Shader ourShader("shader/vertex_reverse_shader.glsl", "shader/fragment_shader.glsl");
    //Shader ourShader("shader/vertex_moving_shader.glsl", "shader/fragment_shader.glsl");
    Shader ourShader("shader/vertex_position_shader.glsl", "shader/fragment_position_shader.glsl");

    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 1.0, 0.0, 0.0,
        0.5f, -0.5f, 0.0f,  0.0, 1.0, 0.0,
        0.0f, 0.5f, 0.0f,   0.0, 0.0, 1.0
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    // 绑定 VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // 指定 VBO 数据
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 指定每一个顶点属性的偏移（读取方式） 从哪个通道读，有几个值，是什么类型的，是否要归一化，下一个要隔几位去读，第一个从哪开始读
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    // 开启通道
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 解绑 VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);


    while (!glfwWindowShouldClose(window))
    {
        // 处理输入
        processInput(window);

        // 开始渲染
        glClearColor(0.2, 0.2, 0.4, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);


        ourShader.use();
        //ourShader.setFloat("offsetX", sin(glfwGetTime()) * 0.5);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);


        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();

    return 0;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
#endif