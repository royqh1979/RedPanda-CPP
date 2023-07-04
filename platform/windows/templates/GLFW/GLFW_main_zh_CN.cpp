#include <iostream>
// GLEW 使用静态链接，注意函数选项中的相关链接参数
#define GLEW_STATIC
#include <GL/glew.h>
// GLFW
#include <GLFW/glfw3.h>

#include "shader.h"

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// 窗口大小
const GLuint WIDTH = 800, HEIGHT = 600;

// main函数，程序从这里开始执行
int main()
{
    // 初始化GLFW
    glfwInit();
    // 设置GLFW必要选项
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// 创建GLFW窗口对象
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // 设置必要的回调函数
    glfwSetKeyCallback(window, key_callback);

    // 让GLEW使用现代方式来获取函数指针和扩展
    glewExperimental = GL_TRUE;
    // 初始化GLEW，设置OpenGL函数指针
    glewInit();

    // 设置视图尺寸
    glViewport(0, 0, WIDTH, HEIGHT);


    // 读取shader文件，并编译，见shader.h代码
    Shader ourShader("shader.vs", "shader.fs");


    // 一维数组，每六个代表一个顶点属性，前三个代表位置属性，后三个代表颜色属性
    GLfloat vertices[] = {
        // Positions         // Colors
        0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,  // Bottom Right
        -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,  // Bottom Left
        0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f   // Top 
    };
    GLuint VBO, VAO;//声明顶点缓冲，声明顶点数组用于管理顶点数据
    glGenVertexArrays(1, &VAO);//创建顶点数组，返回一个独一无二的整数，标识数组
    glGenBuffers(1, &VBO);//创建顶点缓冲，返回一个独一无二的整数，标识缓冲区
    
    glBindVertexArray(VAO);//绑定顶点数组
    glBindBuffer(GL_ARRAY_BUFFER, VBO);//绑定顶点缓冲
	//指定顶点数组的数据源为vertices，第四个参数代表显卡如何管理给定的数据，GL_STATIC_DRWA代表几乎不会改变
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 指定顶点属性的解析方式。即，如何从顶点缓冲获取相应的顶点属性和相应的颜色属性。或者说，顶点着色器中如何知道去哪个顶点属性分量重着色呢
	//对每一个顶点而言，属性有2种，一是位置属性，而是颜色属性，因此每六个浮点数决定了一个顶点的位置和颜色

	//顶点着色器中使用layout(location = 0)定义了position顶点属性的位置值(Location)，因此第一个参数，代表属性分量的索引
	//参数二：顶点位置属性的维度，参数三：属性向量的数据类型，参数四:是否标准化；参数五，顶点位置属性的总字节长度，参数六：在缓冲数组中的偏移量，即起始位置
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);//启用属性0，因为默认是禁用的

    // 参数一，对应顶点着色器中的layout (location = 1) in vec3 color;参数六：说明颜色属性的偏移量在三个浮点数后，与上文vertices一致
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);//启用属性1.

	//顶点数组对象(Vertex Array Object, VAO)的好处就是，当配置顶点属性指针时，你只需要将上面的代码调用执行一次，之后再绘制物体的时候只需要绑定相应的VAO就行了。如下文循环中的绑定再解绑
    glBindVertexArray(0); // 解绑 VAO
    // 游戏主循环
    while (!glfwWindowShouldClose(window))
    {
        // 检查事件，调用相应的回调函数，如下文的key_callback函数
        glfwPollEvents();

        // 渲染
        // 清除颜色缓冲区（the colorbuffer）
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);//渲染颜色到后台缓冲
        glClear(GL_COLOR_BUFFER_BIT);//清除前台缓冲

        // 绘制三角形
        ourShader.Use();//启用着色器程序
        glBindVertexArray(VAO);//每次循环都调用，绑定函数绑定VAO
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);//解绑

        // 交换屏幕缓冲区（the screen buffers）
        glfwSwapBuffers(window);
    }
    // 正确释放所有不再需要的资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    // 终止GLFW，清理所有GLFW使用的资源
    glfwTerminate();
    return 0;
}


// 当键盘按键被按下/释放时，该函数被调用
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}
