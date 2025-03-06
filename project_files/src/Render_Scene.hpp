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
#include "Model_Loader.hpp"
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
	GLuint ground;
	GLuint house;

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
GLuint programTest;
GLuint programDepth;
GLuint programPBR;
GLuint programNormal;
Core::Shader_Loader shaderLoader;

Core::RenderContext shipContext;
Core::RenderContext sphereContext;
Core::RenderContext groundContext;

glm::vec3 cameraPos = glm::vec3(-8.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);

glm::vec3 spaceshipPos = glm::vec3(-8.f, 0, 0);
glm::vec3 spaceshipDir = glm::vec3(1.f, 0.f, 0.f);
GLuint VAO, VBO;

int Boids_count = 20;
float width = 10.0, heigth = 10.0, depth = 10.0;
float aspectRatio = 1.f;
Skybox* skybox = nullptr;

int isNormal = 0, isShadow = 0;

//for shadow mapping and PBR

namespace models {
	Core::RenderContext testContext;
	Core::RenderContext windowContext;
	Core::RenderContext roomContext;
	Core::RenderContext deskContext;
}

GLuint depthMapFBO;
GLuint depthMap;

glm::vec3 sunPos = glm::vec3(-4.740971f, 2.149999f, 0.369280f);
glm::vec3 sunDir = glm::vec3(-0.93633f, 0.351106, 0.003226f);
glm::vec3 sunColor = glm::vec3(0.9f, 0.9f, 0.7f) * 5;

float exposition = 1.f;

glm::vec3 pointlightPos = glm::vec3(0, 2, 0);
glm::vec3 pointlightColor = glm::vec3(0.9, 0.6, 0.6);

glm::vec3 spotlightPos = glm::vec3(0, 0, 0);
glm::vec3 spotlightConeDir = glm::vec3(0, 0, 0);
glm::vec3 spotlightColor = glm::vec3(0.4, 0.4, 0.9) * 3;
float spotlightPhi = 3.14 / 4;

int WIDTH = 1000, HEIGHT = 1000;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;



float lastTime = -1.f;
float deltaTime = 0.f;


struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

glm::mat4 createCameraMatrix()
{
	glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraSide, cameraDir));
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
		0.,0.,(f + n) / (n - f),2 * f * n / (n - f),
		0.,0.,-1.,0.,
		});

	perspectiveMatrix = glm::transpose(perspectiveMatrix);

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
	glUniform3f(glGetUniformLocation(program, "lightPos"), 0, 0, 0);
	Core::DrawContext(context);
}

void drawObjectTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint textureID)
{
	glUseProgram(programTex);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	// Use programTex here instead of program!
	glUniformMatrix4fv(glGetUniformLocation(programTex, "transformation"), 1, GL_FALSE, glm::value_ptr(transformation));
	glUniformMatrix4fv(glGetUniformLocation(programTex, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
	glUniform3f(glGetUniformLocation(programTex, "lightPos"), 0, 0, 0);
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

void drawObjectPBR(Core::RenderContext& context, glm::mat4 modelMatrixPBR, glm::vec3 color, float roughness, float metallic) {


	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformationPBR = viewProjectionMatrix * modelMatrixPBR;
	glUniformMatrix4fv(glGetUniformLocation(programPBR, "transformation"), 1, GL_FALSE, (float*)&transformationPBR);
	glUniformMatrix4fv(glGetUniformLocation(programPBR, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrixPBR);

	glUniform1f(glGetUniformLocation(programPBR, "exposition"), exposition);

	glUniform1f(glGetUniformLocation(programPBR, "roughness"), roughness);
	glUniform1f(glGetUniformLocation(programPBR, "metallic"), metallic);

	glUniform3f(glGetUniformLocation(programPBR, "color"), color.x, color.y, color.z);

	glUniform3f(glGetUniformLocation(programPBR, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glUniform3f(glGetUniformLocation(programPBR, "sunDir"), sunDir.x, sunDir.y, sunDir.z);
	glUniform3f(glGetUniformLocation(programPBR, "sunColor"), sunColor.x, sunColor.y, sunColor.z);

	glUniform3f(glGetUniformLocation(programPBR, "lightPos"), pointlightPos.x, pointlightPos.y, pointlightPos.z);
	glUniform3f(glGetUniformLocation(programPBR, "lightColor"), pointlightColor.x, pointlightColor.y, pointlightColor.z);

	glUniform3f(glGetUniformLocation(programPBR, "spotlightConeDir"), spotlightConeDir.x, spotlightConeDir.y, spotlightConeDir.z);
	glUniform3f(glGetUniformLocation(programPBR, "spotlightPos"), spotlightPos.x, spotlightPos.y, spotlightPos.z);
	glUniform3f(glGetUniformLocation(programPBR, "spotlightColor"), spotlightColor.x, spotlightColor.y, spotlightColor.z);
	glUniform1f(glGetUniformLocation(programPBR, "spotlightPhi"), spotlightPhi);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glm::mat4 lightVPPBR = glm::ortho(-10.f, 10.f, -10.f, 10.f, 1.0f, 30.f) * glm::lookAt(sunPos, sunPos - sunDir, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(programPBR, "lightVP"), 1, GL_FALSE, (float*)&lightVPPBR);

	Core::DrawContext(context);

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

//normal mapping
void drawObjectNormal(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint textureID, GLuint normalID) {
	glUseProgram(programNormal);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programNormal, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(programNormal, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(programNormal, "lightPos"), 0, 0, 0);
	Core::SetActiveTexture(textureID, "colorTexture", programNormal, 0);
	Core::SetActiveTexture(normalID, "normalSampler", programNormal, 1);
	Core::DrawContext(context);

}

//shadow mapping

void initDepthMap() {
	glGenFramebuffers(1, &depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawObjectDepth(Core::RenderContext& renderContext, glm::mat4 viewProjection, glm::mat4 model) {
	glUseProgram(programDepth);
	glUniformMatrix4fv(glGetUniformLocation(programDepth, "modelMatrix"), 1, GL_FALSE, (float*)&model);
	glUniformMatrix4fv(glGetUniformLocation(programDepth, "viewProjectionMatrix"), 1, GL_FALSE, (float*)&viewProjection);
	Core::DrawContext(renderContext);
}

void renderShadowMapSun() {
	float time = glfwGetTime();
	//uzupelnij o renderowanie glebokosci do tekstury
	// 
	//ustawianie przestrzeni rysowania 
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	//bindowanie FBO
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	//czyszczenie mapy głębokości 
	glClear(GL_DEPTH_BUFFER_BIT);
	//ustawianie programu
	glUseProgram(programDepth);

	glm::mat4 lightProjection = glm::ortho(-10.f, 10.f, -10.f, 10.f, 1.0f, 30.0f);
	glm::mat4 modelMatrix = glm::lookAt(sunPos, sunPos - sunDir, glm::vec3(0, 1, 0));
	glm::mat4 lightVP = glm::ortho(-10.f, 10.f, -10.f, 10.f, 1.0f, 30.f) * glm::lookAt(sunPos, sunPos - sunDir, glm::vec3(0, 1, 0));

	if (isShadow == 0) {
		drawObjectDepth(sphereContext, lightVP, glm::mat4());
		drawObjectDepth(sphereContext, lightVP, glm::eulerAngleY(time / 3) *
			glm::translate(glm::vec3(4.f, 0, 0)) *
			glm::scale(glm::vec3(0.3f)));
		drawObjectDepth(sphereContext, lightVP, glm::eulerAngleY(time / 3) *
			glm::translate(glm::vec3(4.f, 0, 0)) *
			glm::eulerAngleY(time) *
			glm::translate(glm::vec3(1.f, 0, 0)) *
			glm::scale(glm::vec3(0.1f)));
		drawObjectDepth(models::roomContext, lightVP, glm::translate(glm::vec3(2.f, -3.f, 5.0f)));
		drawObjectDepth(models::windowContext, lightVP, glm::translate(glm::vec3(2.f, -3.f, 5.0f)));
		drawObjectDepth(models::deskContext, lightVP, glm::translate(glm::vec3(2.f, -3.f, 5.0f)));

		glm::mat4 shipRotationMatrix = computeShipRotationMatrix();
		glm::mat4 shipModelPos = glm::translate(spaceshipPos) * shipRotationMatrix;
		drawObjectDepth(shipContext, lightVP, shipModelPos);
	}




	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WIDTH, HEIGHT);
}

void renderScene(GLFWwindow* window, float currentTime)
{
	glClearColor(0.0f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//zapisanie mapy głębokości, shadow mapping
	renderShadowMapSun();

	glm::mat4 grassModel = glm::translate(glm::vec3(0.f, -3.f, 0.f)) *
		glm::scale(glm::vec3(50.f, 1.f, 50.f));
	drawObjectTexture(groundContext, grassModel, texture::ground);
	

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
	//drawObjectNormal(shipContext, shipModel, texture::ship, texture::shipNormal);

	if (isNormal == 0) {
		drawObjectNormal(shipContext, shipModel, texture::ship, texture::shipNormal);
	}
	else {
		drawObjectTexture(shipContext, shipModel, texture::ship);
	}

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

	//drawing with shadow mapping

	glUseProgram(programPBR);
	//drawObjectPBR(groundContext, grassModel, glm::vec3(0.9f, 0.0f, 0.9f), 0.8f, 0.0f);

	//uncomment 3 lines below to see shadow mapping
	drawObjectPBR(models::roomContext, glm::translate(glm::vec3(2.f, -3.f, 5.0f)), glm::vec3(0.9f, 0.9f, 0.9f), 0.8f, 0.0f);
	drawObjectPBR(models::windowContext, glm::translate(glm::vec3(2.f, -3.f, 5.0f)), glm::vec3(0.402978f, 0.120509f, 0.057729f), 0.2f, 0.0f);
	drawObjectPBR(models::deskContext, glm::translate(glm::vec3(2.f, -3.f, 5.0f)), glm::vec3(0.428691f, 0.08022f, 0.036889f), 0.2f, 0.0f);
	//drawObjectPBR(models::roomContext, glm::translate(glm::vec3(4.f, 0, 8.0f)), glm::vec3(0.9f, 0.9f, 0.9f), 0.8f, 0.0f);

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

	//test depth buffer - for shadow mapping
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glUseProgram(programTest);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, depthMap);
	//Core::DrawContext(models::testContext);

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
	programDepth = shaderLoader.CreateProgram("shaders/shader_depth.vert", "shaders/shader_depth.frag");
	programTest = shaderLoader.CreateProgram("shaders/test.vert", "shaders/test.frag");
	programPBR = shaderLoader.CreateProgram("shaders/shader_5_1_with_shadows.vert", "shaders/shader_5_1_with_shadows.frag");
	programNormal = shaderLoader.CreateProgram("shaders/shader_normal.vert", "shaders/shader_normal.frag");

	loadModelToContext("./models/sphere.obj", sphereContext);
	loadModelToContext("./models/spaceship.obj", shipContext);
	loadModelToContext("./models/ground.obj", groundContext);
	loadModelToContext("./models/test.obj", models::testContext);
	loadModelToContext("./models/room.obj", models::roomContext);
	loadModelToContext("./models/window.obj", models::windowContext);
	loadModelToContext("./models/desk.obj", models::deskContext);

	texture::earth = Core::LoadTexture("textures/earth.png");
	texture::ship = Core::LoadTexture("textures/spaceship.jpg");
	texture::shipNormal = Core::LoadTexture("textures/spaceship_normal.jpg");
	texture::clouds = Core::LoadTexture("textures/clouds.jpg");
	texture::moon = Core::LoadTexture("textures/moon.jpg");
	texture::grid = Core::LoadTexture("textures/grid.png");
	texture::sun = Core::LoadTexture("textures/sun.jpg");
	texture::ground = Core::LoadTexture("textures/ground.png");

	std::vector<std::string> skyboxFaces = {
		"textures/skybox/DaylightBox_Right.bmp", // right
		"textures/skybox/DaylightBox_Left.bmp", // left
		"textures/skybox/DaylightBox_Top.bmp", // top
		"textures/skybox/DaylightBox_Bottom.bmp", // bottom
		"textures/skybox/DaylightBox_Front.bmp", // front
		"textures/skybox/DaylightBox_Back.bmp"  // back
	};
	skybox = new Skybox(skyboxFaces);

	//inicjowanie mapy głębokości, shadow mapping
	initDepthMap();
}

void computeCollisionBoxes(GLFWwindow* window, float currentTime)
{

}

//obsluga wejscia
void processInput(GLFWwindow* window, float currentTime)
{
	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::vec3(0.f, 1.f, 0.f);
	float angleSpeed = 0.05f;
	float moveSpeed = 0.010f;

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

	//normal mapping switching
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
		isNormal = 0;
	}
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
		isNormal = 1;
	}
	//shadow mapping switching
	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
		isShadow = 0;
	}
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
		isShadow = 1;
	}

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

	glm::vec3 correction(0.f);
	if (checkAABBCollision(shipAABB, earthAABB)) {
		correction += computeMTV(shipAABB, earthAABB);
	}
	if (checkAABBCollision(shipAABB, moonAABB)) {
		correction += computeMTV(shipAABB, moonAABB);
	}
	if (checkAABBCollision(shipAABB, sunAABB)) {
		correction += computeMTV(shipAABB, sunAABB);
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

	//initBoids(50, 1000, 1000);
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