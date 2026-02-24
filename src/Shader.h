#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

class Shader
{
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath)
    {
        std::string vertexCode = readShaderFile(vertexPath);
        std::string fragmentCode = readShaderFile(fragmentPath);

        if (vertexCode.empty() || fragmentCode.empty()) {
            std::cerr << "Failed to load shader files\n";
            ID = 0;
            return;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // Compile vertex shader
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Compile fragment shader
        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Link program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        if (ID != 0) {
            std::cout << "✓ Shader program compiled and linked successfully\n";
        }
    }

    ~Shader()
    {
        if (ID != 0) {
            glDeleteProgram(ID);
        }
    }

    void use() const
    {
        glUseProgram(ID);
    }

    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
    }

    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

private:
    static std::string findShaderFile(const char* shaderName)
    {
        // Get project source directory (passed by CMake at compile time)
        std::string projectDir;
#ifdef PROJECT_SOURCE_DIR
        projectDir = PROJECT_SOURCE_DIR;
#endif

        // Standard locations to check for shader files
        std::vector<std::string> searchPaths = {
            std::string(shaderName),                                    // Current directory
            std::string("src/") + shaderName,                           // src/ subdirectory
        };

        // Add paths from project source directory if available
        if (!projectDir.empty()) {
            searchPaths.push_back(projectDir + "/src/" + shaderName);
        }

        for (const auto& path : searchPaths) {
            if (fs::exists(path)) {
                return path;
            }
        }

        return "";
    }

    static std::string readShaderFile(const char* filePath)
    {
        std::string actualPath = findShaderFile(filePath);
        if (actualPath.empty()) {
            std::cerr << "\n✗ Could not find shader file: " << filePath << "\n";
            std::cerr << "Looked in:\n";
            std::cerr << "  - " << filePath << "\n";
            std::cerr << "  - src/" << filePath << "\n";
#ifdef PROJECT_SOURCE_DIR
            std::cerr << "  - " << PROJECT_SOURCE_DIR << "/src/" << filePath << "\n";
#endif
            std::cerr << "\n";
            return "";
        }

        std::ifstream shaderFile;
        shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            shaderFile.open(actualPath);
            std::stringstream shaderStream;
            shaderStream << shaderFile.rdbuf();
            shaderFile.close();
            std::cout << "✓ Loaded shader: " << actualPath << "\n";
            return shaderStream.str();
        }
        catch (std::ifstream::failure& e) {
            std::cerr << "✗ Failed to read shader file: " << actualPath << "\n";
            std::cerr << "  Error: " << e.what() << "\n";
            return "";
        }
    }

    static void checkCompileErrors(unsigned int shader, const std::string& type)
    {
        int success;
        char infoLog[1024];

        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "Shader compilation error (" << type << "):\n"
                          << infoLog << "\n";
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "Program linking error:\n" << infoLog << "\n";
            }
        }
    }
};
