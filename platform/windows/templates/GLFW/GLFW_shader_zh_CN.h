#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/glew.h>

class Shader
{
public:
    GLuint Program;
    
    Shader(const GLchar* vertexPath, const GLchar* fragmentPath)
    {
        // 1. 读取顶点（vertex）/片段（fragment）源文件
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // 确保ifstream对象能够抛出异常:
        vShaderFile.exceptions(std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::badbit);
        try
        {
            // 打开文件
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // 将文件内容读取成字符流
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // 关闭文件
            vShaderFile.close();
            fShaderFile.close();
            // 将字符流转换为string字符串
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
        const GLchar* vShaderCode = vertexCode.c_str();
        const GLchar * fShaderCode = fragmentCode.c_str();
        // 2. 编译着色器（shaders）
        GLuint vertex, fragment;
        GLint success;
        GLchar infoLog[512];
        // 顶点着色器（Vertex Shader）
        vertex = glCreateShader(GL_VERTEX_SHADER);//创建顶点着色器
        glShaderSource(vertex, 1, &vShaderCode, NULL);//指定源代码
        glCompileShader(vertex);//编译着色器
        // 输出编译错误
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);//查看是否编译成功
        if (!success)
        {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // 片段着色器（Fragment Shader）
        fragment = glCreateShader(GL_FRAGMENT_SHADER);//创建片段着色器
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        // 输出编译错误
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // 着色器程序（Shader Program）
        this->Program = glCreateProgram();//创建着色程序
        glAttachShader(this->Program, vertex);//关联顶点着色器
        glAttachShader(this->Program, fragment);//关联片段着色器
        glLinkProgram(this->Program);//链接编译器
        // 输出链接错误
        glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        // 链接入程序后不再需要这些着色器了，删除它们。
        glDeleteShader(vertex);
        glDeleteShader(fragment);

    }
    // 使用当前着色器
    void Use()
    {
        glUseProgram(this->Program);
    }
};

#endif
