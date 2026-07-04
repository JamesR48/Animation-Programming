#include "Shader.h"
#include <fstream>
#include <vector>
#include <GLFW/glfw3.h>
#include "../../Tools/Logger.h"

bool Shader::LoadShaders(std::string VertexShaderFileName, std::string FragmentShaderFileName)
{
    Logger::Log(1, "%s: loading vertex shader '%s' and fragment shader '%s'\n", __FUNCTION__, VertexShaderFileName.c_str(), FragmentShaderFileName.c_str());

    if (!CreateShaderProgram(VertexShaderFileName, FragmentShaderFileName))
    {
        Logger::Log(1, "%s error: shader program creation failed\n", __FUNCTION__);
        return false;
    }

    return true;
}

void Shader::Use()
{
    glUseProgram(mShaderProgram);
}

void Shader::Cleanup()
{
    // This also remove the shaders
    glDeleteProgram(mShaderProgram);
}

bool Shader::CreateShaderProgram(std::string VertexShaderFileName, std::string FragmentShaderFileName)
{
    GLuint vertexShader = LoadShader(VertexShaderFileName, GL_VERTEX_SHADER);
    if (!vertexShader)
    {
        return false;
    }
    GLuint fragmentShader = LoadShader(FragmentShaderFileName, GL_FRAGMENT_SHADER);
    if (!fragmentShader)
    {
        return false;
    }

    mShaderProgram = glCreateProgram();
    glAttachShader(mShaderProgram, vertexShader);
    glAttachShader(mShaderProgram, fragmentShader);
    glLinkProgram(mShaderProgram);

    if (!CheckLinkStats(VertexShaderFileName, FragmentShaderFileName, mShaderProgram))
    {
        Logger::Log(1, "%s error: program linking from vertex shader '%s' / fragment shader '%s' failed\n", __FUNCTION__, VertexShaderFileName.c_str(), FragmentShaderFileName.c_str());
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    /* bind UniformBufferObject in shader */
    GLint UboIndex = glGetUniformBlockIndex(mShaderProgram, "Matrices");
    glUniformBlockBinding(mShaderProgram, UboIndex, 0);

    /* It is safe to delete the two loaded shader programs at this point, as this will
     * just mark them to be removed. But all intermediate data inside the graphics cards
     * will be cleaned, freeing up some space */
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

GLuint Shader::LoadShader(std::string ShaderFileName, GLuint ShaderType)
{
    std::string ShaderAsText;
    ShaderAsText = LoadFileToString(ShaderFileName);
    Logger::Log(4, "%s: loaded shader file '%s', size %i\n", __FUNCTION__, ShaderFileName.c_str(),ShaderAsText.size());

    const GLchar* ShaderSource = ShaderAsText.c_str();
    GLuint OutShader = glCreateShader(ShaderType);
    glShaderSource(OutShader, 1, &ShaderSource, 0);
    glCompileShader(OutShader);

    if (!CheckCompileStats(ShaderFileName, OutShader))
    {
        Logger::Log(1, "%s error: compiling shader '%s' failed\n", __FUNCTION__, ShaderFileName.c_str());
        return 0;
    }

    Logger::Log(1, "%s: shader %#x loaded and compiled\n", __FUNCTION__, OutShader);
    return OutShader;
}

std::string Shader::LoadFileToString(std::string Filename)
{
    std::ifstream inFile(Filename);
    std::string OutStr;

    if (inFile.is_open())
    {
        OutStr.clear();

        /* Get the length of the shader file by seeking the end and
         * allocate the number of bytes in our destination string */
        inFile.seekg(0, std::ios::end);
        OutStr.reserve(inFile.tellg());
        inFile.seekg(0, std::ios::beg);

        // reading the content of ifstream into our string
        OutStr.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();
    }
    else
    {
        char ErrorBuffer[256];
        strerror_s(ErrorBuffer, sizeof(ErrorBuffer), errno);
        Logger::Log(1, "%s error: could not open file %s\n", __FUNCTION__, Filename.c_str());
        Logger::Log(1, "%s error: system says '%s'\n", __FUNCTION__, ErrorBuffer);

        return std::string();
    }

    if (inFile.bad() || inFile.fail())
    {
        Logger::Log(1, "%s error: error while reading file %s\n", __FUNCTION__, Filename.c_str());
        inFile.close();
        return std::string();
    }

    inFile.close();
    Logger::Log(1, "%s: file %s successfully read to string\n", __FUNCTION__, Filename.c_str());
    return OutStr;
}

bool Shader::CheckCompileStats(std::string ShaderFileName, GLuint Shader)
{
    GLint bIsShaderCompiled;
    int LogMessageLength;
    std::vector<char> ShaderLog;

    glGetShaderiv(Shader, GL_COMPILE_STATUS, &bIsShaderCompiled);
    if (!bIsShaderCompiled)
    {
        glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &LogMessageLength);
        ShaderLog = std::vector<char>(LogMessageLength + 1);
        glGetShaderInfoLog(Shader, LogMessageLength, &LogMessageLength, ShaderLog.data());
        ShaderLog.at(LogMessageLength) = '\0';
        Logger::Log(1, "%s error: shader compile of shader '%s' failed\n", __FUNCTION__, ShaderFileName.c_str());
        Logger::Log(1, "%s compile log:\n%s\n", __FUNCTION__, ShaderLog.data());

        return false;
    }

    return true;
}

bool Shader::CheckLinkStats(std::string VertexShaderFileName, std::string FragmentShaderFileName, GLuint ShaderProgram)
{
    GLint bIsProgramLinked;
    int LogMessageLength;
    std::vector<char> ProgramLog;

    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &bIsProgramLinked);
    if (!bIsProgramLinked)
    {
        glGetProgramiv(ShaderProgram, GL_INFO_LOG_LENGTH, &LogMessageLength);
        ProgramLog = std::vector<char>(LogMessageLength + 1);
        glGetProgramInfoLog(ShaderProgram, LogMessageLength, &LogMessageLength, ProgramLog.data());
        ProgramLog.at(LogMessageLength) = '\0';
        Logger::Log(1, "%s error: program linking of shaders '%s' and '%s' failed\n", __FUNCTION__, VertexShaderFileName.c_str(), FragmentShaderFileName.c_str());
        Logger::Log(1, "%s compile log:\n%s\n", __FUNCTION__, ProgramLog.data());
        return false;
    }

    return true;
}
