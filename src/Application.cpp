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
#include "Texture.h"

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
	float vertices[] = {
			-0.5f,-0.5f, 0.0f, 0.0f, // index 0
			0.5f,-0.5f, 1.0f, 0.0f, // index 1
			0.5f,0.5f, 1.0f, 1.0f,  // index 2
			-0.5f,0.5f, 0.0f, 1.0f  // index 3
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
		// enable blending
		GLCall(glEnable(GL_BLEND));
		/*
			Tells OpenGL how to blend (combined) colors
			This function tells OpenGL how to calculate the source (first arg) and destination (second arg) color
			source is the color we want to render, destination is the color that pixel already has.

			By default, this is glBlendFunc(GL_ONE, GL_ZERO);
			This would mean for e.g. R component (in RGBA), it is (Rsrc*1) + (Rdest*0) = Rsrc -> render source, ignore destination.
			Also that we add (+) src and dest color is determined by glBlendEquation(mode), which by default is FL_FUNC_ADD.
			Here we set glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);,
			which means R = (Rsrc * alpha) + (Rdest * (1-alpha)), for alpha = 0, that is R = Rdest - uses just destination,
			for alpha 1: R = (Rsrc*1) + (Rdest*0) = Rsrc - uses just source,
			for alpha 0.1: R = (Rsrc*0.1) + (Rdest*0.9). - combines
		*/
		GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

		// When using core profile, we need to create vertex array buffer
		// With compatibility profile, there is one vao created that stores everything. Since it stores everything,
		// all attrib layouts need to be re-specified every time together with the array buffer
		// --
		// This allows us to bind the array buffer and set a layout to it (vertex attributes)
		VertexArray va;
		VertexBuffer vb{vertices, 4 * 4 * sizeof(float)};

		VertexBufferLayout layout;
		layout.Push<float>(2); // locations
		layout.Push<float>(2); // texture coords
		va.Addbuffer(vb, layout);

		// 6 = 6 indices
		IndexBuffer ib{indices, 6};

		// with shaders in one file
		// Shader shader{"res/shaders/Basic.glsl"};
		Shader shader{"res/shaders/Basic.vert", "res/shaders/Basic.frag"};
		shader.Bind();
		shader.SetUniform4f("u_Color", 0.8f, 0.3f, 0.8f, 1.0f);

		Texture texture{"res/textures/pizza.png"};
		texture.Bind();											 // by default binds at texture slot 0
		shader.SetUniform1i("u_Texture", 0); // 0 is the slot this texture is bound to

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
