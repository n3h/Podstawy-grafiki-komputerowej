#define _CRT_SECURE_NO_WARNINGS

// Include standard headers
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <string>
#include <GL/glew.h>
#include <glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader.hpp"
#include "texture.hpp"
#include "controls.hpp"
#include "objloader.hpp"
#include "vboindexer.hpp"

GLFWwindow* window;
using namespace glm;

// Constants

// Structures
struct object {
	std::string name;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	GLuint vertexbuffer;
	GLuint uvbuffer;
	GLuint normalbuffer;
	GLuint elementbuffer;

	object(const char* n) : name(n) {
		// Read our .obj file
		bool res = loadOBJ(name.c_str(), vertices, uvs, normals);
		indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

		// Load it into a VBO
		glGenBuffers(1, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

		glGenBuffers(1, &uvbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

		glGenBuffers(1, &normalbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

		// Generate a buffer for the indices as well
		glGenBuffers(1, &elementbuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
	}
};

// Variables
int currentShader = 0;
int currentObject = 0;
int isUVon = 1;
int isPHONGon = 1;

std::vector <object> all_objects;

extern mat4 ProjectionMatrix;
extern mat4 ViewMatrix;
mat4 ModelMatrix;
mat4 MVP;

GLuint Texture;

//uniforms, shaders
GLuint programID;
GLuint VertexArrayID;
GLuint MatrixID;
GLuint ViewMatrixID;
GLuint ModelMatrixID;
GLuint LightID;
GLuint TextureID;
GLuint UVonID;
GLuint PHONGonID;

//Functions
int initGame();
int initObjects(int argc, char* argv[]);
void initShaders();
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void calculateMVP();
void sendUniforms();
void drawObject();

int main(int argc, char *argv[]) {
	if (initGame() != 0)
		return -1;

	if (initObjects(argc, argv) != 0)
		return -1;

	initShaders();

	// Load the texture
	Texture = loadDDS("soilCracked.dds");

	glUseProgram(programID);

	glfwSetKeyCallback(window, key_callback);
	do{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		calculateMVP();
		sendUniforms();
		drawObject();

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&	glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	for (int i = 0; i < all_objects.size(); i++) {
		glDeleteBuffers(1, &all_objects[i].vertexbuffer);
		glDeleteBuffers(1, &all_objects[i].uvbuffer);
		glDeleteBuffers(1, &all_objects[i].normalbuffer);
		glDeleteBuffers(1, &all_objects[i].elementbuffer);
	}
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

int initGame() {
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(800, 600, "Terrain, Kamil Bebenek, 258340", NULL, NULL);
	if (window == NULL){
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetCursorPos(window, 800 / 2, 600 / 2);

	//hide cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	// background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	return 0;
}

int initObjects(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Please enter name of OBJECT as argument, example: zad6.exe suzanne.obj \n";
		return -1;
	}

	for (int i = 1; i < argc; i++)
		all_objects.push_back(argv[i]);

	return 0;
}

void initShaders() {
	// Create and compile our GLSL program from the shaders
	switch (currentShader) {
	case 0:
		programID = LoadShaders("phong.vertexshader", "phong.fragmentshader");
		glDisable(GL_CULL_FACE);
		isUVon = 1;
		isPHONGon = 1;
		break;
	case 1:
		programID = LoadShaders("silhouette.vertexshader", "silhouette.fragmentshader");
		glEnable(GL_CULL_FACE);
		isUVon = 0;
		isPHONGon = 0;
		break;
	case 2:
		programID = LoadShaders("cartoons.vertexshader", "cartoons.fragmentshader");
		glDisable(GL_CULL_FACE);
		break;
	}

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	TextureID = glGetUniformLocation(programID, "myTextureSampler");
	UVonID = glGetUniformLocation(programID, "UVon");
	PHONGonID = glGetUniformLocation(programID, "PHONGon");
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		currentShader = 0;
		initShaders();
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		currentShader = 1;
		initShaders();
	}
	if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		currentShader = 2;
		initShaders();
	}
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
		currentShader++;
		currentShader %= 3;
		initShaders();
	}
	if (key == GLFW_KEY_CAPS_LOCK && action == GLFW_PRESS) {
		(isUVon == 1) ? isUVon = 0 : isUVon = 1;
		glUniform1i(UVonID, isUVon);
	}
	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) {
		(isPHONGon == 1) ? isPHONGon = 0 : isPHONGon = 1;
		glUniform1i(PHONGonID, isPHONGon);
	}
	if ((key == GLFW_KEY_GRAVE_ACCENT || key == GLFW_KEY_ENTER) && action == GLFW_PRESS) {
		currentObject++;
		currentObject %= all_objects.size();
	}
}

void calculateMVP() {
	// Compute the MVP matrix from keyboard and mouse input
	computeMatricesFromInputs();
	ModelMatrix = mat4(1.0);
	ModelMatrix = rotate(
		ModelMatrix,
		getRotations().y,
		vec3(-1.0f, 0.0f, 0.0f));
	ModelMatrix = rotate(
		ModelMatrix,
		getRotations().x,
		vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix = rotate(
		ModelMatrix,
		getRotations().z,
		vec3(0.0f, 0.0f, -1.0f));
	MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
}

void sendUniforms() {
	// Send our transformation to the currently bound shader, in the "MVP" uniform
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

	// effects on/off
	glUniform1i(UVonID, isUVon);
	glUniform1i(PHONGonID, isPHONGon);

	glm::vec3 lightPos = glm::vec3(4, 4, 4);
	glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	// Set our "myTextureSampler" sampler to user Texture Unit 0
	glUniform1i(TextureID, 0);
}

void drawObject() {
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, all_objects[currentObject].vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 2nd attribute buffer : UVs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, all_objects[currentObject].uvbuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 3rd attribute buffer : normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, all_objects[currentObject].normalbuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, all_objects[currentObject].elementbuffer);

	// Draw the triangles !
	glDrawElements(GL_TRIANGLES, all_objects[currentObject].indices.size(), GL_UNSIGNED_SHORT, (void*)0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}