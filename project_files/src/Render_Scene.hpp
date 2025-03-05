#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <cfloat>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Texture.h"

#include "Camera.h"
#include "Boids.hpp"
#include "Box.cpp"
#include "Skybox.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include "SOIL/SOIL.h"


namespace texture {
	GLuint earth;
	GLuint clouds;
	GLuint moon;
	GLuint ship;
	GLuint sun;

	GLuint grid;

	GLuint earthNormal;
	GLuint asteroidNormal;
	GLuint shipNormal;
}

GLuint program;
GLuint programSun;
GLuint programTex;
GLuint programEarth;
GLuint programProcTex;
Core::Shader_Loader shaderLoader;

Core::RenderContext shipContext;
Core::RenderContext sphereContext;

glm::vec3 cameraPos = glm::vec3(-8.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);

glm::vec3 spaceshipPos = glm::vec3(-8.f, 0, 0);
glm::vec3 spaceshipDir = glm::vec3(1.f, 0.f, 0.f);
GLuint VAO,VBO;

float aspectRatio = 1.f;
Skybox* skybox = nullptr;

int Boids_count = 20;
float width = 10.0, heigth = 10.0, depth = 10.0;

struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

glm::mat4 createCameraMatrix()
{
	glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir,glm::vec3(0.f,1.f,0.f)));
	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraSide,cameraDir));
	glm::mat4 cameraRotrationMatrix = glm::mat4({
		cameraSide.x,cameraSide.y,cameraSide.z,0,
		cameraUp.x,cameraUp.y,cameraUp.z ,0,
		-cameraDir.x,-cameraDir.y,-cameraDir.z,0,
		0.,0.,0.,1.,
		});
	cameraRotrationMatrix = glm::transpose(cameraRotrationMatrix);
	glm::mat4 cameraMatrix = cameraRotrationMatrix * glm::translate(-cameraPos);

	return cameraMatrix;
}

glm::mat4 createPerspectiveMatrix()
{
	glm::mat4 perspectiveMatrix;
	float n = 0.05;
	float f = 20.;
	float a1 = glm::min(aspectRatio, 1.f);
	float a2 = glm::min(1 / aspectRatio, 1.f);
	perspectiveMatrix = glm::mat4({
		1,0.,0.,0.,
		0.,aspectRatio,0.,0.,
		0.,0.,(f+n) / (n - f),2*f * n / (n - f),
		0.,0.,-1.,0.,
		});

	perspectiveMatrix=glm::transpose(perspectiveMatrix);

	return perspectiveMatrix;
}

void drawObjectColor(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color) 
{
	glUseProgram(program);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(program, "color"), color.x, color.y, color.z);
	glUniform3f(glGetUniformLocation(program, "lightPos"), 0,0,0);
	Core::DrawContext(context);
}

void drawObjectTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint textureID) 
{
	glUseProgram(programTex);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(program, "lightPos"), 0, 0, 0);
	Core::SetActiveTexture(textureID, "colorTexture", programTex, 0);
	Core::DrawContext(context);
}

void drawSunTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint textureID) {
	GLuint prog = programSun;
	glUseProgram(prog);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;

	// Pass the transformation matrix to the shader.
	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, glm::value_ptr(transformation));

	// Bind the sun texture to texture unit 0.
	Core::SetActiveTexture(textureID, "colorTexture", prog, 0);

	// Draw the sun geometry.
	Core::DrawContext(context);

	glUseProgram(0);
}

glm::mat4 computeShipRotationMatrix() {
	// Oblicz wektory orientacji statku
	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::normalize(glm::cross(spaceshipSide, spaceshipDir));
	// Ustawiamy "przód" statku jako spaceshipDir (bez ujemnego znaku)
	glm::mat4 rotationMatrix = glm::mat4({
		spaceshipSide.x, spaceshipSide.y, spaceshipSide.z, 0.f,
		spaceshipUp.x,   spaceshipUp.y,   spaceshipUp.z,   0.f,
		spaceshipDir.x,  spaceshipDir.y,  spaceshipDir.z,  0.f,
		0.f,             0.f,             0.f,             1.f
		});
	// Transponujemy, bo GLM domyślnie używa kolumnowego układu pamięci
	return glm::transpose(rotationMatrix);
}

AABB computeTransformedAABB(const AABB& localBox, const glm::mat4& modelMatrix) {
	glm::vec3 corners[8] = {
		glm::vec3(localBox.min.x, localBox.min.y, localBox.min.z),
		glm::vec3(localBox.max.x, localBox.min.y, localBox.min.z),
		glm::vec3(localBox.min.x, localBox.max.y, localBox.min.z),
		glm::vec3(localBox.min.x, localBox.min.y, localBox.max.z),
		glm::vec3(localBox.max.x, localBox.max.y, localBox.min.z),
		glm::vec3(localBox.min.x, localBox.max.y, localBox.max.z),
		glm::vec3(localBox.max.x, localBox.min.y, localBox.max.z),
		localBox.max
	};

	AABB worldBox;
	worldBox.min = glm::vec3(FLT_MAX);
	worldBox.max = glm::vec3(-FLT_MAX);
	for (int i = 0; i < 8; ++i) {
		glm::vec3 transformed = glm::vec3(modelMatrix * glm::vec4(corners[i], 1.0f));
		worldBox.min = glm::min(worldBox.min, transformed);
		worldBox.max = glm::max(worldBox.max, transformed);
	}
	return worldBox;
}

glm::vec3 computeMTV(const AABB& a, const AABB& b) {
	// Compute overlap distances on each axis.
	float overlapX = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
	float overlapY = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
	float overlapZ = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

	// Choose the axis with the smallest penetration.
	float minOverlap = overlapX;
	glm::vec3 mtv((a.min.x < b.min.x) ? -overlapX : overlapX, 0.0f, 0.0f);

	if (overlapY < minOverlap) {
		minOverlap = overlapY;
		mtv = glm::vec3(0.0f, (a.min.y < b.min.y) ? -overlapY : overlapY, 0.0f);
	}
	if (overlapZ < minOverlap) {
		minOverlap = overlapZ;
		mtv = glm::vec3(0.0f, 0.0f, (a.min.z < b.min.z) ? -overlapZ : overlapZ);
	}
	return mtv;
}

bool checkAABBCollision(const AABB& a, const AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}

void drawBoid(const Boid& b) {
	float yaw = atan2(b.vx, b.vz);
	float pitch = atan2(b.vy, glm::length(glm::vec2(b.vx, b.vz)));
	drawObjectTexture(shipContext,
		glm::translate(glm::vec3(b.x, b.y, b.z)) *
		glm::scale(glm::vec3(1.0f)) * 
		glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0, 1, 0)) * 
		glm::rotate(glm::mat4(1.0f), -pitch, glm::vec3(1.0f, 0.0f, 0.0f)),
		texture::moon);
}

void renderScene(GLFWwindow* window, float currentTime) //-----------------------------------------------------------------
{
	glClearColor(0.0f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawObjectTexture(sphereContext,
		glm::eulerAngleY(currentTime / 3) *
		glm::translate(glm::vec3(4.f, 0, 0)) *
		glm::scale(glm::vec3(0.3f)),
		texture::earth);

	drawObjectTexture(sphereContext,
		glm::eulerAngleY(currentTime / 3) *
		glm::translate(glm::vec3(4.f, 0, 0)) *
		glm::eulerAngleY(currentTime) *
		glm::translate(glm::vec3(1.f, 0, 0)) *
		glm::scale(glm::vec3(0.1f)),
		texture::moon);

	glm::mat4 shipRotationMatrix = computeShipRotationMatrix();
	glm::mat4 shipModel = glm::translate(spaceshipPos) * shipRotationMatrix;
	drawObjectTexture(shipContext, shipModel, texture::ship);

	glm::mat4 sunModelMatrix = glm::translate(glm::vec3(0.f, 0.f, 0.f)) * glm::scale(glm::vec3(1.5f));
	drawSunTexture(sphereContext, sunModelMatrix, texture::sun);

	// Disable writing to the depth buffer so the skybox doesn't overwrite depth values
	glDepthMask(GL_FALSE);

	// Set depth function to allow the skybox to be drawn correctly
	glDepthFunc(GL_LEQUAL);

	// Draw your skybox
	if (skybox) {
		// Make sure to use a view matrix with translation removed.
		glm::mat4 view = glm::mat4(glm::mat3(createCameraMatrix()));
		glm::mat4 projection = createPerspectiveMatrix();
		skybox->Draw(view, projection);
	}

	// Restore default depth function and re-enable depth writing
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);


	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1000, 1000, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Disable depth testing if you want the boids to be drawn on top.
	glDisable(GL_DEPTH_TEST);

	// Update and draw boids
	for (const auto& b : boids) {
		drawBoid(b);
	}
	update_boids();

	// Restore previous projection and modelview matrices
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glUseProgram(0);
	glfwSwapBuffers(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	aspectRatio = width / float(height);
	glViewport(0, 0, width, height);
}

void loadModelToContext(std::string path, Core::RenderContext& context)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	context.initFromAssimpMesh(scene->mMeshes[0]);
}

void init(GLFWwindow* window)
{
	//create boids
	initBoids(Boids_count, heigth, width, depth);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glEnable(GL_DEPTH_TEST);
	program = shaderLoader.CreateProgram("shaders/shader_5_1.vert", "shaders/shader_5_1.frag");
	programTex = shaderLoader.CreateProgram("shaders/shader_5_1_tex.vert", "shaders/shader_5_1_tex.frag");
	programEarth = shaderLoader.CreateProgram("shaders/shader_5_1_tex.vert", "shaders/shader_5_1_tex.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_sun.vert", "shaders/shader_sun.frag");
	programProcTex = shaderLoader.CreateProgram("shaders/shader_5_1_tex.vert", "shaders/shader_5_1_tex.frag");

	loadModelToContext("./models/sphere.obj", sphereContext);
	loadModelToContext("./models/spaceship.obj", shipContext);

	texture::earth = Core::LoadTexture("textures/earth.png");
	texture::ship = Core::LoadTexture("textures/spaceship.jpg");
	texture::clouds = Core::LoadTexture("textures/clouds.jpg");
	texture::moon = Core::LoadTexture("textures/moon.jpg");
	texture::grid = Core::LoadTexture("textures/grid.png");
	texture::sun = Core::LoadTexture("textures/sun.jpg");

	std::vector<std::string> skyboxFaces = {
		"textures/skybox/space_lf.png", // left
		"textures/skybox/space_rt.png", // right
		"textures/skybox/space_dn.png", // bottom
		"textures/skybox/space_up.png", // top
		"textures/skybox/space_ft.png", // front
		"textures/skybox/space_bk.png"  // back
	};
	skybox = new Skybox(skyboxFaces);
}

//obsluga wejscia
void processInput(GLFWwindow* window, float currentTime)
{
	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::vec3(0.f, 1.f, 0.f);
	float angleSpeed = 0.0025f;
	float moveSpeed = 0.005f;

	// Używamy zmiennej pomocniczej proposedPos, zaczynając od aktualnej pozycji
	glm::vec3 proposedPos = spaceshipPos;

	// Uaktualniamy proposedPos odpowiadająco do wejścia
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		proposedPos += spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		proposedPos -= spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		proposedPos += spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
		proposedPos -= spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		proposedPos += spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		proposedPos -= spaceshipUp * moveSpeed;

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::eulerAngleY(angleSpeed) * glm::vec4(spaceshipDir, 0));
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::eulerAngleY(-angleSpeed) * glm::vec4(spaceshipDir, 0));

	// Ustal lokalny AABB dla statku
	AABB sphereLocalBox;
	sphereLocalBox.min = glm::vec3(-1.0f);
	sphereLocalBox.max = glm::vec3(1.0f);

	// Dla Ziemi – używamy właściwej skali modelu zamiast [-1,1]  
	AABB earthLocalBox;
	earthLocalBox.min = glm::vec3(-0.1f);
	earthLocalBox.max = glm::vec3(0.1f);

	// Dla Księżyca – analogicznie, jeśli model jest skalowany do 0.1
	AABB moonLocalBox;
	moonLocalBox.min = glm::vec3(-0.1f);
	moonLocalBox.max = glm::vec3(0.1f);

	AABB sunLocalBox;
	sunLocalBox.min = glm::vec3(-0.5f);
	sunLocalBox.max = glm::vec3(0.5f);

	// Obliczamy modele dla obiektów, z którymi sprawdzamy kolizję
	// Dla Ziemi – używamy tej samej kolejności co przy renderowaniu
	glm::mat4 earthModel = glm::eulerAngleY(currentTime / 3) *
		glm::translate(glm::vec3(4.f, 0, 0)) *
		glm::scale(glm::vec3(0.3f));
	AABB earthAABB = computeTransformedAABB(earthLocalBox, earthModel);

	// Dla Księżyca
	glm::mat4 moonModel = glm::eulerAngleY(currentTime / 3) *
		glm::translate(glm::vec3(4.f, 0, 0)) *
		glm::eulerAngleY(currentTime) *
		glm::translate(glm::vec3(1.f, 0, 0)) *
		glm::scale(glm::vec3(0.1f));
	AABB moonAABB = computeTransformedAABB(moonLocalBox, moonModel);

	// Dla Słońca
	glm::mat4 sunModel = glm::translate(glm::vec3(0.f, 0.f, 0.f)) * glm::scale(glm::vec3(1.5f));
	AABB sunAABB = computeTransformedAABB(sunLocalBox, sunModel);

	glm::mat4 shipRotationMatrix = computeShipRotationMatrix();
	glm::mat4 proposedShipModel = glm::translate(proposedPos) * shipRotationMatrix;
	AABB shipAABB = computeTransformedAABB(sphereLocalBox, proposedShipModel);

	glm::vec3 delta = proposedPos - spaceshipPos;

	bool collision = checkAABBCollision(shipAABB, earthAABB) ||
		checkAABBCollision(shipAABB, moonAABB) ||
		checkAABBCollision(shipAABB, sunAABB);

	if (collision) {
		if (glm::dot(delta, spaceshipDir) < 0.0f) {
			spaceshipPos = proposedPos;
		}
		else {
			std::cout << "Kolizja - ruch zablokowany!" << std::endl;
		}
	}
	else {
		spaceshipPos = proposedPos;
	}

	glm::vec3 correction(0.f);
	if (checkAABBCollision(shipAABB, earthAABB)) {
		correction += computeMTV(shipAABB, earthAABB);
	}
	if (checkAABBCollision(shipAABB, moonAABB)) {
		correction += computeMTV(shipAABB, moonAABB);
	}

	if (glm::length(correction) > 0.f) {
		spaceshipPos += correction;
		std::cout << "Collision resolved, correction applied: ("
			<< correction.x << ", " << correction.y << ", " << correction.z << ")\n";
	}
	else {
		spaceshipPos = proposedPos;
	}

	cameraPos = spaceshipPos - 1.5f * spaceshipDir + glm::vec3(0.f, 1.f, 0.f) * 0.5f;
	cameraDir = glm::normalize(spaceshipPos - cameraPos);
}

void shutdown(GLFWwindow* window)
{
	if (skybox) {
		delete skybox;
		skybox = nullptr;
	}

	shaderLoader.DeleteProgram(program);
}

// funkcja jest glowna petla
void renderLoop(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window))
	{
		float currentTime = glfwGetTime();
		processInput(window, currentTime);
		renderScene(window, currentTime);
		glfwPollEvents();
	}
}
