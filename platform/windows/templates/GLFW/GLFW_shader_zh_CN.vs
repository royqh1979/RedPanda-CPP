#version 330 core
layout (location = 0) in vec3 position;//in 代表输入向量， location，与下面的顶点属性描述有关。
layout (location = 1) in vec3 color;

out vec3 ourColor;//out 代表输出3维向量，作为片段着色器的输入，见下文

void main()
{
    gl_Position = vec4(position, 1.0f);
    ourColor = color;
}
