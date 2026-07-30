#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
// #include <gl2d/gl2d.h>
#include <openglErrorReporting.h>
#include "Renderer.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "VertexArray.h"
#include "Shader.h"

static void GLFW_error_callback(int error, const char *description)
{
	std::cout << "Error: " << description << "\n";
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

	/* Draw */
	/*
		We scope the code so `ib` and `vb` are deleted before we call `glfwTerminate()`,
		which could result in an error (OpenGL would not be accessible anymore and GLCall would loop on error)
	*/
	{
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

		// 6 = 6 indices
		IndexBuffer ib{indices, 6};

		// with shaders in one file
		// Shader shader{"res/shaders/Basic.glsl"};
		Shader shader{"res/shaders/Basic.vert", "res/shaders/Basic.frag"};
		shader.Bind();
		shader.SetUniform4f("u_Color", 0.8f, 0.3f, 0.8f, 1.0f);

		// unbind everything for the purpose of showing how to re-bind everything again
		va.Unbind();
		shader.Unbind();
		vb.Unbind();
		ib.Unbind();

		float r = 0.0f;
		float increment = 0.05;

		Renderer renderer;
		while (!glfwWindowShouldClose(window))
		{
			renderer.Clear();

			shader.Bind();
			shader.SetUniform4f("u_Color", r, 0.3f, 0.8f, 1.0f);

			renderer.Draw(va, ib, shader);

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
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
