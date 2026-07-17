#ifndef CPPANIMPROGRAMMING_SHADER_H
#define CPPANIMPROGRAMMING_SHADER_H

#include <string>
#include <glad/gl.h>

class Shader {
public:
    bool LoadShaders(std::string VertexShaderFileName, std::string FragmentShaderFileName);
    void Use();

    /* for setting the index of the current model skeleton joint inside the uniform buffer containing
        the joint matrices respective the dual quaternions */
    bool GetUniformLocation(const std::string& UniformName);
    void SetUniformValue(const int Value);

    void Cleanup();

private:
    GLuint mShaderProgram = 0;
    GLint mUniformLocation = -1;

    bool CreateShaderProgram(std::string VertexShaderFileName, std::string FragmentShaderFileName);
    GLuint LoadShader(std::string ShaderFileName, GLuint ShaderType);
    std::string LoadFileToString(std::string Filename);
    bool CheckCompileStats(std::string ShaderFileName, GLuint Shader);
    bool CheckLinkStats(std::string VertexShaderFileName, std::string FragmentShaderFileName, GLuint ShaderProgram);
};


#endif //CPPANIMPROGRAMMING_SHADER_H
