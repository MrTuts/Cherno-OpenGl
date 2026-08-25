#include "Shader.h"

#include "Renderer.h"
#include <iostream>
#include <fstream>
#include <sstream>

/* Generic function for error handling */
// PFNGLGETSHADERIVPROC and PFNGLGETSHADERINFOLOGPROC are just function definitions that are same for shader/program
bool CheckStatus(GLuint objectID, PFNGLGETSHADERIVPROC objectPropertyGetterFunc, PFNGLGETSHADERINFOLOGPROC infoLogFunc, GLenum statusType)
{
  // /* Error handling */
  int status;
  GLCall(objectPropertyGetterFunc(objectID, statusType, &status));
  // GL_FALSE is just 0, so we could also write if(!status){...}
  if (status == GL_FALSE)
  {
    int length;
    GLCall(objectPropertyGetterFunc(objectID, GL_INFO_LOG_LENGTH, &length));
    // char message[length];
    // alloca allows to allocate on stack dynamically. C++ may complain about allocating memory on stack with variable length `char message[length]`.
    char *message = (char *)alloca(length * sizeof(char));
    GLCall(infoLogFunc(objectID, length, &length, message));
    std::cout << "Failed to link shaders " << std::endl;
    std::cout << message << std::endl;
    return false;
  }

  return true;
}

bool CheckProgramStatus(GLuint programId)
{
  return CheckStatus(programId, glGetProgramiv, glGetProgramInfoLog, GL_LINK_STATUS);
}

bool CheckShaderStatus(GLuint shaderId)
{
  return CheckStatus(shaderId, glGetShaderiv, glGetShaderInfoLog, GL_COMPILE_STATUS);
}

/* Parse single shader from a file with single shader */
std::string Shader::ParseSingleShader(const std::string &filepath)
{
  std::ifstream stream{filepath};

  if (!stream.good())
  {
    std::cout << "Error: Unable to open shader file: " << filepath << std::endl;
    exit(1);
  }
  return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

/* Parse fragment and vertex shaders from single file */
ShaderProgramSource Shader::ParseShader(const std::string &filepath)
{
  std::ifstream stream{filepath};

  if (!stream.good())
  {
    std::cout << "Error: Unable to open shader file: " << filepath << std::endl;
    exit(1);
  }

  enum class ShaderType
  {
    NONE = -1,
    VERTEX = 0,
    FRAGMENT = 1
  };

  std::string line;
  std::stringstream ss[2];
  ShaderType type = ShaderType::NONE;

  while (getline(stream, line))
  {
    if (line.find("#shader") != std::string::npos)
    {
      if (line.find("vertex") != std::string::npos)
      {
        type = ShaderType::VERTEX;
      }
      else if (line.find("fragment") != std::string::npos)
      {
        type = ShaderType::FRAGMENT;
      }
      else
      {
        type = ShaderType::NONE;
      }
    }
    else
    {
      ss[(int)type] << line << '\n';
    }
  }

  return {ss[0].str(), ss[1].str()};
}

unsigned int Shader::CompileShader(unsigned int type, const std::string &source)
{
  GLCall(unsigned int id = glCreateShader(type));
  const char *src = source.c_str(); // pointer to the beginning of our data. Alternative &source[0]
  GLCall(glShaderSource(id, 1, &src, nullptr));
  GLCall(glCompileShader(id));

  if (!CheckShaderStatus(id))
  {
    return 0;
  }
  return id;
}

unsigned int Shader::CreateShader(const std::string &vertexShader, const std::string &fragmentShader)
{
  GLCall(unsigned int program = glCreateProgram());
  unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
  unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

  GLCall(glAttachShader(program, vs));
  GLCall(glAttachShader(program, fs));
  GLCall(glLinkProgram(program));
  if (!CheckProgramStatus(program))
  {
    return 0;
  }
  GLCall(glValidateProgram(program));

  // The program is created, we can delete intermediate shaders (like .obj files)
  GLCall(glDeleteShader(vs));
  GLCall(glDeleteShader(fs));

  return program;
}

Shader::Shader(const std::string &vsFilePath, const std::string &fsFilePath) : m_vsFilePath(vsFilePath), m_fsFilePath(fsFilePath), m_RenderID(0)
{
  std::string vertexSource = ParseSingleShader(vsFilePath);
  std::string fragmentSource = ParseSingleShader(fsFilePath);
  m_RenderID = CreateShader(vertexSource, fragmentSource);
}

Shader::Shader(const std::string &filePath) : m_filePath(filePath), m_RenderID(0)
{
  ShaderProgramSource source = ParseShader(filePath);
  m_RenderID = CreateShader(source.VertexSource, source.FragmentSource);
}

Shader::~Shader()
{
  GLCall(glDeleteProgram(m_RenderID));
}

void Shader::Bind() const
{
  GLCall(glUseProgram(m_RenderID));
}

void Shader::Unbind() const
{
  GLCall(glUseProgram(0));
}

void Shader::SetUniform4f(const std::string &name, float v0, float v1, float v2, float v3)
{
  // assign value to uniform location
  GLCall(glUniform4f(GetUniformLocation(name), v0, v1, v2, v3));
}

void Shader::SetUniformMat4f(const std::string &name, const glm::mat4 matrix)
{
  // 1 - number of matrices we want to send
  // GL_FALSE - whether we want to transpose the matrix or not. OpenGL expects column-major order, glm uses column-major order by default, so we do not want to transpose it.
  // &matrix[0][0] - pointer to the first element
  GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

void Shader::SetUniform1d(const std::string &name, double value)
{
  GLCall(glUniform1d(GetUniformLocation(name), value));
}

void Shader::SetUniform1i(const std::string &name, int value)
{
  GLCall(glUniform1i(GetUniformLocation(name), value));
}

void Shader::SetUniform1iv(const std::string &name, unsigned int size, int *value)
{
  GLCall(glUniform1iv(GetUniformLocation(name), size, value));
}

void Shader::SetUniform1f(const std::string &name, float value)
{
  GLCall(glUniform1f(GetUniformLocation(name), value));
}

unsigned int Shader::GetUniformLocation(const std::string &name) const
{
  if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
  {
    return m_UniformLocationCache[name];
  }
  // when shader is created, OpenGL assigns id to every uniform variable, here we retrieve that location
  GLCall(int location = glGetUniformLocation(m_RenderID, name.c_str()));
  if (location == -1)
  {
    // Check uniform was found
    // Even if we specify the uniform in shader, but we do not use the value, OpenGL may strip the value away!
    std::cout << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;
  }

  m_UniformLocationCache[name] = location;
  return location;
}
