bool keyboard = true;

// use when we have sprintf error/warning (text2D)
#ifdef _WIN32
	#define _CRT_SECURE_NO_DEPRECATE
#endif

// Include standard headers
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <random>
#include <GL/glew.h>
#include <glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader.hpp"
#include "text2D.hpp"

GLFWwindow* window;
using namespace glm;

// Constants
const double M_PI = 3.14159265358979323846;

// Structures declarations
struct bubble {
	vec3 vecT; // translation
	vec3 vecS; // scale
	vec3 vecC; // colour
	bubble() {}
	bubble(const vec3& t, const vec3& s, const vec3& c) : vecT(t), vecS(s), vecC(c) {}
};

// Variables
bool endGame = false;
int level = 0;

double lastTime = glfwGetTime();
double dTime = 0;

int numberOfBubbles = 40;
int drawFrontWall = false;

GLfloat playerRadius = 0.1f;
GLfloat sphereRadius = 0.06f;
vec3 aquariumSize = vec3(5.0f, 5.0f, 10.0f);
vec3 playerTranslation = vec3(0.0f, -0.2f, 10.0f - playerRadius);
vec3 lightPos = vec3(0, 6.0f, -5.0f);

//uniforms
GLuint programID;
GLuint MatrixID;
GLuint ViewMatrixID;
GLuint ModelMatrixID;
GLuint InverseTransposeModelMatrixID;
GLuint IsWallID;
GLuint ColourID;
GLuint LightID;
GLuint PlayerLightID;

mat4 ModelMatrix;
mat4 MVP;
mat4 ViewMatrix;
mat4 ProjectionMatrix;
bool isWallx = false;
vec3 colour;

//objects
std::vector < bubble > all_bubbles;
std::vector<vec3> sphere;
std::vector<vec3> player;
std::vector<vec3> aquarium; 
std::vector<vec3> aquariumNormals;
std::vector<vec3> aquariumFront;
std::vector<vec3> aquariumFrontNormals;
std::vector<vec3> sphereNormals;
std::vector<vec3> playerNormals;

//buffers
GLuint sphereBuffer;
GLuint playerBuffer;
GLuint aquariumBuffer;
GLuint aquariumBuffer2;
GLuint aquariumNormalsBuffer;
GLuint aquariumFrontBuffer;
GLuint aquariumFrontBuffer2;
GLuint aquariumFrontNormalsBuffer;
GLuint playerNormalsBuffer;

//controls
// Initial position : on +Z
vec3 position = vec3(0, 0.0f, playerTranslation.z + 1.0f);
// Initial horizontal angle : toward -Z
float horizontalAngle = 3.14f;
// Initial vertical angle : none
float verticalAngle = 0.0f; // -0.3
// Initial Field of View
float FoV = 45.0f;

float speed = 1.f; // x units / second
float mouseSpeed = 0.004f;

// Functions declarations
void initGame();
void initUniformsHandles();
void initObjects();
void initSphere(float r, float precision, std::vector<vec3>& points, std::vector<vec3>& normals);
void initAquarium();
void initFrontAquarium();
void drawAquarium();
void drawBubbles();
void drawPlayer();
void computeMatricesFromInputs();
void recalculateBubbles();
float generateRandomNumber(float a, float b);
void checkCollision();
void initBubbles(int);

int main(void) {
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
	window = glfwCreateWindow(668, 668, "Smiercionosne babelki, Kamil Bebenek", NULL, NULL);
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
	glfwSetCursorPos(window, 668 / 2, 668 / 2);

	//hide cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	// Dark blue background
	glClearColor(0.1f, 0.1f, 0.7f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Cull triangles which normal is not towards the camera
	//glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("VertexShader.vertexshader", "FragmentShader.fragmentshader");

	initUniformsHandles();
	initObjects();

	glUseProgram(programID);

	// light
	glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

	do {
		glDisable(GL_LIGHTING);

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		if (!endGame) {
			// Compute the MVP matrix from keyboard and mouse input
			computeMatricesFromInputs();
		}

		ModelMatrix = mat4(1.0);
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(InverseTransposeModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniform1i(IsWallID, 0);

		// player light
		glUniform3f(PlayerLightID, playerTranslation.x, playerTranslation.y, playerTranslation.z);

		drawAquarium();
		drawBubbles();
		drawPlayer();

		if (!endGame) {
			checkCollision();
			recalculateBubbles();
		}
		else {
			// calculate points
			int points = level * 1000;
			if (playerTranslation.z < 0) // check it!
				points += 500 + playerTranslation.z * (-50);
			else
				points += 500 - playerTranslation.z * 50;
			char text[256];
			sprintf(text, "Points: %i", points);
			printText2D(text, 100, 505, 30);
		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&	glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &aquariumBuffer);
	glDeleteBuffers(1, &aquariumFrontBuffer);
	glDeleteBuffers(1, &sphereBuffer);
	glDeleteBuffers(1, &playerBuffer);
	glDeleteBuffers(1, &playerNormalsBuffer);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Delete the text's VBO, the shader and the texture
	cleanupText2D();

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

void initUniformsHandles() {
	// Get a handle for our uniforms
	MatrixID = glGetUniformLocation(programID, "MVP");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	InverseTransposeModelMatrixID = glGetUniformLocation(programID, "itM");
	IsWallID = glGetUniformLocation(programID, "isWall");
	ColourID = glGetUniformLocation(programID, "colour");
	LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	PlayerLightID = glGetUniformLocation(programID, "PlayerLightPosition_worldspace");
}

void initObjects() {
	//bubbles / spheres
	initBubbles(numberOfBubbles);
	initSphere(sphereRadius, 40, sphere, sphereNormals);
	glGenBuffers(1, &sphereBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphereBuffer);
	glBufferData(GL_ARRAY_BUFFER, sphere.size() * sizeof(vec3), &sphere[0], GL_STATIC_DRAW);

	// player
	initSphere(playerRadius, 40, player, playerNormals);
	glGenBuffers(1, &playerBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, playerBuffer);
	glBufferData(GL_ARRAY_BUFFER, player.size() * sizeof(vec3), &player[0], GL_STATIC_DRAW);

	glGenBuffers(1, &playerNormalsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, playerNormalsBuffer);
	glBufferData(GL_ARRAY_BUFFER, playerNormals.size() * sizeof(vec3), &playerNormals[0], GL_STATIC_DRAW);

	// aquarium
	initAquarium();

	// resize aquarium
	for (int i = 0; i < 24; i++) {
		aquarium[i] *= aquariumSize;
		aquariumNormals.push_back(glm::normalize(aquarium[i]));
	}

	for (int i = 0; i < 6; i++) {
		aquariumFront[i] *= aquariumSize;
		aquariumFrontNormals.push_back(glm::normalize(aquariumFront[i]));
	}

	glGenBuffers(1, &aquariumBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, aquariumBuffer);
	glBufferData(GL_ARRAY_BUFFER, aquarium.size() * sizeof(vec3), &aquarium[0], GL_STATIC_DRAW);

	glGenBuffers(1, &aquariumNormalsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, aquariumNormalsBuffer);
	glBufferData(GL_ARRAY_BUFFER, aquariumNormals.size() * sizeof(vec3), &aquariumNormals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &aquariumFrontBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, aquariumFrontBuffer);
	glBufferData(GL_ARRAY_BUFFER, aquariumFront.size() * sizeof(vec3), &aquariumFront[0], GL_STATIC_DRAW);

	glGenBuffers(1, &aquariumFrontNormalsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, aquariumFrontNormalsBuffer);
	glBufferData(GL_ARRAY_BUFFER, aquariumFrontNormals.size() * sizeof(vec3), &aquariumFrontNormals[0], GL_STATIC_DRAW);

	// Initialize our little text library with the Holstein font
	initText2D("Holstein.DDS");
}

void initSphere(float r, float precision, std::vector<vec3>& points, std::vector<vec3>& normals) {
	float deltaRho = pi<float>() / precision;
	float deltaTheta = 2.0f * pi<float>() / precision;
	for (int i = 0; i < precision; ++i)	{
		float rho = i * deltaRho;
		vec3 vertices[4];
		for (int j = 0; j < precision; ++j)	{
			// Vertex 0 
			float theta = j * deltaTheta;
			vertices[0] = vec3(r * -sin(theta) * sin(rho), r * cos(theta) * sin(rho), r * cos(rho));
			// Vertex 1
			vertices[1] = vec3(r * -sin(theta) * sin(rho + deltaRho), r * cos(theta) * sin(rho + deltaRho), r * cos(rho + deltaRho));
			// Vertex 2
			theta = ((j + 1) == precision) ? 0.0f : (j + 1) * deltaTheta;
			vertices[2] = vec3(r * -sin(theta) * sin(rho), r * cos(theta) * sin(rho), r * cos(rho));
			// Vertex 3
			vertices[3] = vec3(r * -sin(theta) * sin(rho + deltaRho), r * cos(theta) * sin(rho + deltaRho), r * cos(rho + deltaRho));

			// Triangle 1
			points.push_back(vertices[0]);
			points.push_back(vertices[1]);
			points.push_back(vertices[2]);
			normals.push_back(normalize(vertices[0]));
			normals.push_back(normalize(vertices[1]));
			normals.push_back(normalize(vertices[2]));

			// Triangle 2
			points.push_back(vertices[1]);
			points.push_back(vertices[3]);
			points.push_back(vertices[2]);
			normals.push_back(normalize(vertices[1]));
			normals.push_back(normalize(vertices[3]));
			normals.push_back(normalize(vertices[2]));
		}
	}
}

void recalculateBubbles() {
	dTime = glfwGetTime() - lastTime;
	if (dTime > 0.01f) {
		// move and grow bubbles
		for (int i = 0; i < all_bubbles.size(); i++) {
			if (all_bubbles[i].vecT.y > aquariumSize.y) {
				all_bubbles[i].vecS.x = all_bubbles[i].vecS.y = all_bubbles[i].vecS.z = generateRandomNumber(1.0f, 1.2f);
				all_bubbles[i].vecT.x = generateRandomNumber(-aquariumSize.x + sphereRadius * all_bubbles[i].vecS.x,
					aquariumSize.x - sphereRadius * all_bubbles[i].vecS.x);
				all_bubbles[i].vecT.y = -aquariumSize.y + sphereRadius * all_bubbles[i].vecS.x;
				all_bubbles[i].vecT.z = generateRandomNumber(-aquariumSize.z + sphereRadius * all_bubbles[i].vecS.x, 
					aquariumSize.z - sphereRadius * all_bubbles[i].vecS.x);
			}
			else {
				all_bubbles[i].vecT += vec3(0.0f, 0.01f, 0.0f);
				all_bubbles[i].vecS += vec3(0.005f, 0.005f, 0.005f);
			}
		}
		lastTime += dTime;
	}
}

void initAquarium() {
	//bottom wall
	aquarium.push_back(vec3(-1.0f, -1.0f, -1.0f));
	aquarium.push_back(vec3(-1.0f, -1.0f, 1.0f));
	aquarium.push_back(vec3(1.0f, -1.0f, 1.0f));

	aquarium.push_back(vec3(1.0f, -1.0f, -1.0f));
	aquarium.push_back(vec3(-1.0f, -1.0f, -1.0f));
	aquarium.push_back(vec3(1.0f, -1.0f, 1.0f));

	// left-side
	aquarium.push_back(vec3(-1.0f, -1.0f, -1.0f));
	aquarium.push_back(vec3(-1.0f, -1.0f, 1.0f));
	aquarium.push_back(vec3(-1.0f, 1.0f, -1.0f));

	aquarium.push_back(vec3(-1.0f, 1.0f, -1.0f));
	aquarium.push_back(vec3(-1.0f, -1.0f, 1.0f));
	aquarium.push_back(vec3(-1.0f, 1.0f, 1.0f));

	// back
	aquarium.push_back(vec3(-1.0f, -1.0f, -1.0f));
	aquarium.push_back(vec3(1.0f, -1.0f, -1.0f));
	aquarium.push_back(vec3(1.0f, 1.0f, -1.0f));

	aquarium.push_back(vec3(-1.0f, -1.0f, -1.0f));
	aquarium.push_back(vec3(-1.0f, 1.0f, -1.0f));
	aquarium.push_back(vec3(1.0f, 1.0f, -1.0f));

	// front
	aquarium.push_back(vec3(-1.0f, -1.0f, 1.0f));
	aquarium.push_back(vec3(1.0f, -1.0f, 1.0f));
	aquarium.push_back(vec3(1.0f, 1.0f, 1.0f));

	aquarium.push_back(vec3(-1.0f, -1.0f, 1.0f));
	aquarium.push_back(vec3(-1.0f, 1.0f, 1.0f));
	aquarium.push_back(vec3(1.0f, 1.0f, 1.0f));

	//right-side
	aquariumFront.push_back(vec3(1.0f, -1.0f, -1.0f));
	aquariumFront.push_back(vec3(1.0f, -1.0f, 1.0f));
	aquariumFront.push_back(vec3(1.0f, 1.0f, -1.0f));
	aquariumFront.push_back(vec3(1.0f, 1.0f, -1.0f));
	aquariumFront.push_back(vec3(1.0f, -1.0f, 1.0f));
	aquariumFront.push_back(vec3(1.0f, 1.0f, 1.0f));
}

void drawAquarium() {
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
	glUniformMatrix4fv(InverseTransposeModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform1i(IsWallID, 1);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, aquariumBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

	//normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, aquariumNormalsBuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

	if (drawFrontWall == false)
		glDrawArrays(GL_TRIANGLES, 0, aquarium.size() - 6);
	else 
		glDrawArrays(GL_TRIANGLES, 0, aquarium.size());

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(2);

	if (!glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, aquariumFrontBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

		//normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, aquariumFrontNormalsBuffer);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

		glDrawArrays(GL_TRIANGLES, 0, aquariumFront.size());

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(2);
	}

	glUniform1i(IsWallID, 0);
}

void drawBubbles() {
	//vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, sphereBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

	//normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, playerNormalsBuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

	for (int i = 0; i < all_bubbles.size(); i++) {
		mat4 M2 = mat4(1.0) * translate(mat4(1.0f), all_bubbles[i].vecT) * scale(mat4(1.0f), all_bubbles[i].vecS);
		mat4 MVP2 = ProjectionMatrix * ViewMatrix * M2;
		mat4 itM2 = inverse(transpose(M2));

		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &M2[0][0]);
		glUniformMatrix4fv(InverseTransposeModelMatrixID, 1, GL_FALSE, &itM2[0][0]);
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP2[0][0]);
		glUniform3f(ColourID, all_bubbles[i].vecC.x, all_bubbles[i].vecC.y, all_bubbles[i].vecC.z);

		glDrawArrays(GL_TRIANGLES, 0, sphere.size());
	}
	glUniform3f(ColourID, 0, 0, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(2);
}

void drawPlayer() {
	mat4 Mp = mat4(1.0) * translate(mat4(1.0f), playerTranslation);
	mat4 itMp = inverse(transpose(Mp));
	mat4 MVPp = ProjectionMatrix * ViewMatrix * Mp;

	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Mp[0][0]);
	glUniformMatrix4fv(InverseTransposeModelMatrixID, 1, GL_FALSE, &itMp[0][0]);
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVPp[0][0]);

	// vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, playerBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

	// normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, playerNormalsBuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLES, 0, player.size());
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(2);
}

void checkCollision() {
	for (int i = 0; i < all_bubbles.size(); i++) {
		if (glm::sqrt(													// or glm::distance
			pow(playerTranslation.x - all_bubbles[i].vecT.x, 2) +
			pow(playerTranslation.y - all_bubbles[i].vecT.y, 2) +
			pow(playerTranslation.z - all_bubbles[i].vecT.z, 2))
			<= playerRadius + sphereRadius * all_bubbles[i].vecS.x) {
			endGame = true;
		}
	}
}

void initBubbles(int number) {
	for (int i = 0; i < number; i++) {
		float tmp1 = generateRandomNumber(-aquariumSize.x + sphereRadius, aquariumSize.x - sphereRadius);
		float tmp2 = generateRandomNumber(-aquariumSize.y + sphereRadius, aquariumSize.y - sphereRadius);
		float tmp3 = generateRandomNumber(-aquariumSize.z + sphereRadius, aquariumSize.z - sphereRadius);
		all_bubbles.push_back(bubble(vec3(tmp1, tmp2, tmp3), vec3(1.0f, 1.0f, 1.0f), vec3(generateRandomNumber(0, 1), generateRandomNumber(0, 1), generateRandomNumber(0, 1))));
	}
}

//controls
void computeMatricesFromInputs(){
	bool outView = false;
	// glfwGetTime is called only once, the first time this function is called
	static double lastTime = glfwGetTime();

	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	// Get mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Reset mouse position for next frame
	glfwSetCursorPos(window, 668 / 2, 668 / 2);

	// Compute new orientation
	horizontalAngle += mouseSpeed * float(668 / 2 - xpos);
	verticalAngle += mouseSpeed * float(668 / 2 - ypos);

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	vec3 direction(
		cos(verticalAngle) * sin(horizontalAngle),
		sin(verticalAngle),
		cos(verticalAngle) * cos(horizontalAngle)
		);

	// Right vector
	vec3 right = vec3(
		sin(horizontalAngle - 3.14f / 2.0f),
		0,
		cos(horizontalAngle - 3.14f / 2.0f)
		);

	// Up vector
	vec3 up = cross(right, direction);
	vec3 pos_before = position;
	vec3 player_before = playerTranslation;

	// Move forward
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
		if (!(playerTranslation.z + (direction * deltaTime * speed).z <= -aquariumSize.z + playerRadius)) {
			position += direction * deltaTime * speed;
			if (keyboard == true)
				playerTranslation += direction * deltaTime * speed;
		}
		else {
			//next level!
			level++;
			playerTranslation = vec3(0.0f, -0.2f, 10.0f);

			// Initial position : on +Z
			position = vec3(0, 0.0f, playerTranslation[2] + 1.0f);
			// Initial horizontal angle : toward -Z
			horizontalAngle = 3.14f;
			// Initial vertical angle : none
			verticalAngle = 0.0f; // -0.3

			initBubbles(30 + 10 * level);
			drawFrontWall = false;
		}

	}
	// Move backward
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
		if (!(playerTranslation.z - (direction * deltaTime * speed ).z + playerRadius >= aquariumSize.z)) {
			position -= direction * deltaTime * speed;
			if (keyboard == true)
				playerTranslation -= direction * deltaTime * speed;
		}
	}
	// Strafe right
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
		if (!(playerTranslation.x + (right * deltaTime * speed).x + playerRadius >= aquariumSize.x)) {
			position += right * deltaTime * speed;
			if (keyboard == true)
				playerTranslation += right * deltaTime * speed;
		}
	}
	// Strafe left
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
		if (!(playerTranslation.x - (right * deltaTime * speed).x - playerRadius <= -aquariumSize.x)) {
			position -= right * deltaTime * speed;
			if (keyboard == true)
				playerTranslation -= right * deltaTime * speed;
		}
	}
	// UP
	if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
		if (!(playerTranslation.y + (up * deltaTime * speed).y + playerRadius >= aquariumSize.y)) {
			position += up * deltaTime * speed;
			if (keyboard == true)
				playerTranslation += up * deltaTime * speed;
		}
	}
	// DOWN
	if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
		if (!(playerTranslation.y - (up * deltaTime * speed).y - playerRadius <= -aquariumSize.y)) {
			position -= up * deltaTime * speed;
			if (keyboard == true)
				playerTranslation -= up * deltaTime * speed;
		}
	}
	
	if ((playerTranslation.z <= -aquariumSize.z + playerRadius) ||
		(playerTranslation.x + playerRadius >= aquariumSize.x) ||
		(playerTranslation.x - playerRadius <= -aquariumSize.x) ||
		(playerTranslation.y + playerRadius >= aquariumSize.y) ||
		(playerTranslation.y - playerRadius <= -aquariumSize.y)) {
			playerTranslation = player_before;
			position = pos_before;
	}
	
	// 2nd view
	if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
		outView = true;
		ViewMatrix = lookAt(
			vec3(aquariumSize.x + 24.0f, 0.0f, 0.0f),           // Camera is here
			vec3(aquariumSize.x, 0.0f, 0.0f), // and looks here : at the same position, plus "direction"
			vec3(0,1,0)                  // Head is up (set to 0,-1,0 to look upside-down)
			);
	}

	if (position.z <= 10)
		drawFrontWall = true;
	else
		drawFrontWall = false;

	// zoom
	if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
		FoV -= 0.1f;
	if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
		FoV += 0.1f;

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	ProjectionMatrix = perspective(FoV, 3.0f / 3.0f, 0.1f, 100.0f);
	if (!outView) {
		// Camera matrix
		ViewMatrix = lookAt(
			position,           // Camera is here
			position + direction, // and looks here : at the same position, plus "direction"
			up                  // Head is up (set to 0,-1,0 to look upside-down)
			);
	}

	// For the next frame, the "last time" will be "now"
	lastTime = currentTime;
}

float generateRandomNumber(float a, float b) { // C++11
	std::random_device rd;
	static std::default_random_engine e{ rd() };
	std::uniform_real_distribution<float> l{ a, b };
	return l(e);
}