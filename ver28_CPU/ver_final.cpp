// for instance drawing and frustum culling
// https://www.reddit.com/r/opengl/comments/13z9gs8/instanced_drawing_frustum_culling/

#include "version.h"

#if VER == 4
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <chrono>

#include "Shader.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "BVH.hpp"
#include "Scene.hpp"


#include <list> //std::list
#include <memory> //std::unique_ptr



void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);


// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera
Camera camera(glm::vec3(0.0f, 10.0f, 0.0f));
// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

vec3 pos = { 0,0,0 };

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Frustum Culling", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);


	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}


	// stbi_set_flip_vertically_on_load(true);
	glfwSwapInterval(0);


	glEnable(GL_DEPTH_TEST);

	glfwSwapInterval(0);

	camera.MovementSpeed = 20.f;


	Shader lineShader("shaders/vs.vs", "shaders/line.fs");
	Shader instanceShader("shaders/instance_ssbo.vs", "shaders/fs.fs");

	Scene scene;
	Model model = Model("./objects/cup.obj");
	std::unique_ptr<Entity> ourEntity = make_unique<Entity>(model, true);


	ourEntity->transform.setLocalPosition({ 0, 0, 0 });
	const float scale = 1.0;
	ourEntity->transform.setLocalScale({ scale, scale, scale });


	Entity* lastEntity = ourEntity.get();

	for (unsigned int y = 0; y < NUM; y++) {
		for (unsigned int x = 0; x < NUM; ++x)
		{
			for (unsigned int z = 0; z < NUM; ++z)
			{
				ourEntity->addChild(model, true);
				lastEntity = ourEntity->children.back().get();

				//Set transform values
				lastEntity->transform.setLocalPosition({ x * 10.f ,  y * 10.f, z * 10.f });
			}
		}
	}

	camera.Position = glm::vec3(-85.3798, 24.2075, -94.0037);
	camera.Front = glm::vec3(0.664853, 0.142629, 0.733231);
	camera.updateCamera();

	ourEntity->updateSelfAndChild();
	scene.add(ourEntity.get());
	scene.initialize();
	
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		// render
		// ------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		instanceShader.use();

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, FAR);
		const Frustum camFrustum = createFrustumFromCamera(camera, (float)SCR_WIDTH / (float)SCR_HEIGHT, glm::radians(camera.fov), 0.1f, FAR);


		glm::mat4 view = camera.GetViewMatrix();

		instanceShader.setMat4("projection", projection);
		instanceShader.setMat4("view", view);

		// draw our scene graph
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


		scene.draw_instanced_with_ssbo(instanceShader, camFrustum);


		//ourEntity->transform.setLocalPosition(pos);
		// this is slow for large scene, diable it cuz no thing would move in this scene
		//ourEntity->updateSelfAndChild();


#if SHOW_BOX == 1
		// draw bounding box
		lineShader.use();
		lineShader.setMat4("projection", projection);
		lineShader.setMat4("view", view);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		scene.drawBoundingBox(lineShader);
#endif

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}


float lightspeed = 10;
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_KP_8) == GLFW_PRESS)

		pos.z -= deltaTime * lightspeed;
	if (glfwGetKey(window, GLFW_KEY_KP_5) == GLFW_PRESS)

		pos.z += deltaTime * lightspeed;
	if (glfwGetKey(window, GLFW_KEY_KP_4) == GLFW_PRESS)

		pos.x -= deltaTime * lightspeed;
	if (glfwGetKey(window, GLFW_KEY_KP_6) == GLFW_PRESS)

		pos.x += deltaTime * lightspeed;
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)

		pos.y += deltaTime * lightspeed;
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)

		pos.y -= deltaTime * lightspeed;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	camera.ProcessMouseMovement(xpos, ypos);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}



#endif