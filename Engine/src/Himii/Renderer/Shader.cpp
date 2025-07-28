#include "Hepch.h"
#include "Shader.h"
#include "Himii/Core/Log.h"
#include "glm/gtc/type_ptr.hpp"
#include "glad/glad.h"

namespace Himii
{
    Shader::Shader(const std::string &vertexSource, const std::string &fragmentSource)
    {
        // ������ɫ������
        uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
        uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        // ������ɫ��Դ��
        const char *vertexSourceCStr = vertexSource.c_str();
        const char *fragmentSourceCStr = fragmentSource.c_str();
        glShaderSource(vertexShader, 1, &vertexSourceCStr, nullptr);
        glShaderSource(fragmentShader, 1, &fragmentSourceCStr, nullptr);
        // ������ɫ��
        glCompileShader(vertexShader);
        glCompileShader(fragmentShader);
        // ���������
        int success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            HIMII_CORE_ERROR("Vertex Shader Compilation Failed: {0}", infoLog);
        }
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            HIMII_CORE_ERROR("Fragment Shader Compilation Failed: {0}", infoLog);
        }
        // ��������������ɫ��
        m_RendererID = glCreateProgram();
        glAttachShader(m_RendererID, vertexShader);
        glAttachShader(m_RendererID, fragmentShader);
        glLinkProgram(m_RendererID);
        // ɾ����ɫ��������Ϊ�����Ѿ������ӵ�������
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    Shader::~Shader()
    {
        glDeleteProgram(m_RendererID);
    }

    void Shader::Bind() const
    {
        glUseProgram(m_RendererID);
    }

    void Shader::Unbind() const
    {
        glUseProgram(0);
    }

    void Shader::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
    {

        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location == -1)
        {
            HIMII_CORE_ERROR("Uniform 'u_MVP' not found in shader program!");
            return;
        }
        glUniform4f(location,values.x,values.y,values.z,values.w);
    }

    void Shader::UploadUniformMat4(const std::string& name,const glm::mat4 &matrix)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location == -1)
        {
            HIMII_CORE_ERROR("Uniform 'u_MVP' not found in shader program!");
            return;
        }
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }
}