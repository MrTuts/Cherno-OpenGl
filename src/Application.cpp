#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
// #include <gl2d/gl2d.h>
#include <openglErrorReporting.h>
#include "Renderer.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "VertexArray.h"

static void GLFW_error_callback(int error, const char *description)
{
	std::cout << "Error: " << description << "\n";
}

/* Parse single shader from a file with single shader */
static std::string ParseSingleShader(const std::string &filepath)
{
	std::ifstream stream{filepath};
	std::string line;
	std::string shader;
	while (std::getline(stream, line))
	{
		shader.append(line + "\n");
	}
	return shader;
}

struct ShaderProgramSource
{
	std::string VertexSource;
	std::string FragmentSource;
};
/* Parse fragment and vertex shaders from single file */
static ShaderProgramSource ParseShader(const std::string &filepath)
{
	std::ifstream stream{filepath};

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

static unsigned int CompileShader(unsigned int type, const std::string &source)
{
	GLCall(unsigned int id = glCreateShader(type));
	const char *src = source.c_str(); // pointer to the beginning of our data. Alternative &source[0]
	GLCall(glShaderSource(id, 1, &src, nullptr));
	GLCall(glCompileShader(id));

	// /* Error handling */
	int result;
	GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
	// GL_FALSE is just 0, so we could also write if(!result){...}
	if (result == GL_FALSE)
	{
		int length;
		GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
		// char message[length];
		// alloca allows to allocate on stack dynamically. C++ may complain about allocating memory on stack with variable length `char message[length]`.
		char *message = (char *)alloca(length * sizeof(char));
		GLCall(glGetShaderInfoLog(id, length, &length, message));
		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
		std::cout << message << std::endl;
		GLCall(glDeleteShader(id));
		return 0;
	}

	return id;
}

static unsigned int CreateShader(const std::string &vertexShader, const std::string &fragmentShader)
{
	GLCall(unsigned int program = glCreateProgram());
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	GLCall(glAttachShader(program, vs));
	GLCall(glAttachShader(program, fs));
	GLCall(glLinkProgram(program));
	GLCall(glValidateProgram(program));

	// The program is created, we can delete intermediate shaders (like .obj files)
	GLCall(glDeleteShader(vs));
	GLCall(glDeleteShader(fs));

	return program;
}

int main(void)
{

	glfwSetErrorCallback(GLFW_error_callback);

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow *window;
	window = glfwCreateWindow(640, 480, "Cherno series", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	GLCall(std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl);

	{
		GLint major, minor;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);
		if (major > 4 || (major == 4 && minor >= 3))
			// calls glDebugMessageCallback which is only available since 4.3 (macOS caps at 4.1!)
			enableReportGlErrors();
	}

	std::string vertexSource = ParseSingleShader("res/shaders/Basic.vert");
	std::string fragmentSource = ParseSingleShader("res/shaders/Basic.frag");
	unsigned int shader = CreateShader(vertexSource, fragmentSource);

	/* Draw */
	/*
		We scope the code so `ib` and `vb` are deleted before we call `glfwTerminate()`,
		which could result in an error (OpenGL would not be accessible anymore and GLCall would loop on error)
	*/
	{
		// clang-format off
	/* 
		The rectangle we want to draw
		3 ---- 2
		 |    |
		 |    |
		0 ---- 1
	*/
	float positions[] = {
			-0.5f,-0.5f, // index 0
			0.5f,-0.5f, // index 1
			0.5f,0.5f,  // index 2
			-0.5f,0.5f,  // index 3
	};
	// indices can be chars or shorts, but MUST be unsigned
	unsigned int indices[] = {
		0,1,2, // bottom right triangle
		2,3,0  // top left triangle
	};
		// clang-format on

		// When using core profile, we need to create vertex array buffer
		// With compatibility profile, there is one vao created that stores everything. Since it stores everything,
		// all attrib layouts need to be re-specified every time together with the array buffer
		// --
		// This allows us to bind the array buffer and set a layout to it (vertex attributes)
		VertexArray va;
		VertexBuffer vb{positions, 4 * 2 * sizeof(float)};

		VertexBufferLayout layout;
		layout.Push<float>(2);
		va.Addbuffer(vb, layout);

		IndexBuffer ib{indices, 6};

		// with shaders in one file
		// ShaderProgramSource source = ParseShader("res/shaders/Basic.glsl");
		// unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);

		GLCall(glUseProgram(shader));

		// when shader is created, OpenGL assigns id to every uniform variable, here we retrieve that location
		GLCall(int uColorLocation = glGetUniformLocation(shader, "u_Color"));
		// Check uniform was found
		// Even if we specify the uniform in shader, but we do not use the value, OpenGL may strip the value away!
		ASSERT(uColorLocation != -1);
		// assign value to uColorLocation
		GLCall(glUniform4f(uColorLocation, 0.8f, 0.3f, 0.8f, 1.0f));

		// unbind everything for the purpose of showing how to re-bind everything again
		va.Unbind();
		GLCall(glUseProgram(0));
		vb.Unbind();
		ib.Unbind();

		float r = 0.0f;
		float increment = 0.05;

		while (!glfwWindowShouldClose(window))
		{
			GLCall(glClear(GL_COLOR_BUFFER_BIT));

			// Draw triangle using legacy API
			/*
			glBegin(GL_TRIANGLES);
			glVertex2f(-0.5f, -0.5f);
			glVertex2f(0.0f, 0.5f);
			glVertex2f(0.5f, -0.5f);
			glEnd();
			*/

			// Draw triangle using OpenGL 3.3+ API
			// glBindBuffer(GL_ARRAY_BUFFER, buffer);
			// glDrawArrays(GL_TRIANGLES, 0, 6);

			// rebind everything
			GLCall(glUseProgram(shader));
			va.Bind();
			// no need to bind vertex buffer and specify the attributes layout, that is already linked with the vao
			ib.Bind();

			// assign value to uColorLocation
			GLCall(glUniform4f(uColorLocation, r, 0.3f, 0.8f, 1.0f));
			// 6 = 6 indices
			GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));

			if (r > 1.0f)
			{
				increment = -0.05;
			}
			else if (r < 0.0f)
			{
				increment = 0.05;
			}
			r += increment;

			/* Swap front and back buffers */
			glfwSwapBuffers(window);

			/* Poll for and process events */
			glfwPollEvents();
		}
	}

	// cleanup
	GLCall(glDeleteProgram(shader));
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
