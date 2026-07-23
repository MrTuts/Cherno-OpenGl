#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
// #include <gl2d/gl2d.h>
#include <openglErrorReporting.h>

static void error_callback(int error, const char *description)
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
	unsigned int id = glCreateShader(type);
	const char *src = source.c_str(); // pointer to the beginning of our data. Alternative &source[0]
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	// /* Error handling */
	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	// GL_FALSE is just 0, so we could also write if(!result){...}
	if (result == GL_FALSE)
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		// char message[length];
		// alloca allows to allocate on stack dynamically. C++ may complain about allocating memory on stack with variable length `char message[length]`.
		char *message = (char *)alloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id);
		return 0;
	}

	return id;
}

static unsigned int CreateShader(const std::string &vertexShader, const std::string &fragmentShader)
{
	unsigned int program = glCreateProgram();
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	// The program is created, we can delete intermediate shaders (like .obj files)
	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

int main(void)
{

	glfwSetErrorCallback(error_callback);

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	GLFWwindow *window;
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;

	/* Draw */

	// From YT comment "Modern OpenGL requires a VAO be defined and bound if you are using the core profile"
	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// clang-format off
	float positions[] = {
			-0.5f,-0.5f,
			0.5f,-0.5f,
			0.5f,0.5f,
			
			0.5f,0.5f,
			-0.5f,0.5f,
			-0.5f,-0.5f,
	};
	// clang-format on

	unsigned int buffer;
	glGenBuffers(1, &buffer);																												 // create a buffer and store its ID in buffer
	glBindBuffer(GL_ARRAY_BUFFER, buffer);																					 // tells OpenGL we are working with data of this buffer
	glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), positions, GL_STATIC_DRAW); // upload data to the buffer. Can be done whenever (but the  buffer has to be bound)

	glEnableVertexAttribArray(0);
	// specify how the data is laid out in the buffer.
	// 0 is the index of the attribute,
	// 2 is the number of components,
	// GL_FLOAT is the type of each component,
	// GL_FALSE means we don't want to normalize the data (transforming 0-255 to 0-1),
	// sizeof(float) * 2 is the stride (the distance between consecutive attributes)
	// 0 is the offset (the starting point of the first attribute).
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

	std::string vertexSource = ParseSingleShader("res/shaders/Basic.vert");
	std::string fragmentSource = ParseSingleShader("res/shaders/Basic.frag");
	unsigned int shader = CreateShader(vertexSource, fragmentSource);

	// with shaders in one file
	// ShaderProgramSource source = ParseShader("res/shaders/Basic.glsl");
	// unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);

	glUseProgram(shader);

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw triangle using legacy API
		/*
		glBegin(GL_TRIANGLES);
		glVertex2f(-0.5f, -0.5f);
		glVertex2f(0.0f, 0.5f);
		glVertex2f(0.5f, -0.5f);
		glEnd();
		*/

		// Draw triangle using OpenGL 3.3+ API
		glDrawArrays(GL_TRIANGLES, 0, 6);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	// cleanup
	glDeleteProgram(shader);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
