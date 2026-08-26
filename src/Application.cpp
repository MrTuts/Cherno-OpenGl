#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <iostream>
#include <string>
// #include <gl2d/gl2d.h>
#include <openglErrorReporting.h>
#include "Renderer.h"
#include "cherno/scenes/SceneClearColor.h"
#include "cherno/scenes/ScenePizza.h"
#include "cherno/scenes/SceneTexture2D.h"
#include "cherno/scenes/SceneBatchColor.h"
#include "cherno/scenes/SceneBatchTexture.h"
#include "cherno/scenes/SceneBatchDynamic.h"
#include "jamieKing/scenes/BasicTriangle/SceneBasicTriangle.h"
#include "jamieKing/scenes/DepthBuffer/SceneDepthBuffer.h"
#include "Scene.h"
#include "SceneMenu.h"

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

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow *window;
	window = glfwCreateWindow(640, 480, "OpenGL", NULL, NULL);
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

	/* Draw */
	/*
		We scope the code so `ib` and `vb` are deleted before we call `glfwTerminate()`,
		which could result in an error (OpenGL would not be accessible anymore and GLCall would loop on error)
	*/
	{
		// Enable color blending
		GLCall(glEnable(GL_BLEND));
		/*
			Tells OpenGL how to blend (combine) colors
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

		float r = 0.0f;
		float increment = 0.05;

		scene::Scene *currentScene = nullptr;
		scene::SceneMenu *sceneMenu = new scene::SceneMenu(currentScene);
		currentScene = sceneMenu;

		sceneMenu->RegisterChernoScene<cherno::scene::SceneClearColor>("Clear color");
		sceneMenu->RegisterChernoScene<cherno::scene::ScenePizza>("Pizza");
		sceneMenu->RegisterChernoScene<cherno::scene::SceneTexture2D>("2D Texture");
		sceneMenu->RegisterChernoScene<cherno::scene::SceneBatchColor>("Batch color render");
		sceneMenu->RegisterChernoScene<cherno::scene::SceneBatchTexture>("Batch texture render");
		sceneMenu->RegisterChernoScene<cherno::scene::SceneBatchDynamic>("Batch dynamic render");
		sceneMenu->RegisterJKingScene<jking::scene::BasicTriangleScene>("Basic triangle");
		sceneMenu->RegisterJKingScene<jking::scene::DepthBufferScene>("Depth buffer");

		double currentFrameTime = glfwGetTime();
		double deltaTime;
		double lastFrameTime;

#pragma region imgui
		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
#pragma endregion

		while (!glfwWindowShouldClose(window))
		{
			GLCall(glClear(GL_COLOR_BUFFER_BIT));

			currentFrameTime = glfwGetTime();
			deltaTime = currentFrameTime - lastFrameTime;
			lastFrameTime = currentFrameTime;

			if (currentScene)
			{
				currentScene->OnUpdate(deltaTime);
				currentScene->OnRender();
			}

#pragma region imgui
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			if (currentScene)
			{
				ImGui::Begin("Scene");
				// this renders the button, the if condition is true if button is clicked
				if (currentScene != sceneMenu && ImGui::Button("<-"))
				{
					delete currentScene;
					currentScene = sceneMenu;
				}
				currentScene->OnImGuiRender();
				float framerate = ImGui::GetIO().Framerate;
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / framerate, framerate);
				ImGui::End();
			}

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#pragma endregion

			/* Swap front and back buffers */
			glfwSwapBuffers(window);

			/* Poll for and process events */
			glfwPollEvents();
		}
		delete currentScene;
		if (currentScene != sceneMenu)
		{
			delete sceneMenu;
		}
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// cleanup
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
