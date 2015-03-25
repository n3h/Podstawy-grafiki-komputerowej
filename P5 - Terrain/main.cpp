#define _CRT_SECURE_NO_WARNINGS

// Include standard headers
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <fstream>
#include <GL/glew.h>
#include <glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader.hpp"

GLFWwindow* window;
using namespace glm;

// Constants
const int edgeSize = 1201;
const int mapSize = edgeSize*edgeSize;

// Variables
short mapElements[360][180];
unsigned int LOD = 3;
const int numberOfLODs = 5;
bool autoLOD = false;
const double minFPS = 10;
bool view2d = true;
bool fromRange = false;

int numberOfObjects;
std::vector<std::string> fileNames;
int minNS = 90, maxNS = -90, minEW = 180, maxEW = -180;
std::vector<std::pair<int, int> > coords;
std::vector<GLuint> numberOfIndices;
std::vector<GLfloat> grid;

double last_time, last_reset;
int FPScounter = 0;

float speed = 1.0f;

//uniforms
GLuint programID, programID3d;

GLuint MVP_ID;
//GLuint ColourID;
GLuint CoordinatesID;

GLuint MVP_ID3d;
//GLuint ColourID3d;
GLuint CoordinatesID3d;

float FoV;
float mouseSpeed = 0.004f;
vec3 cameraPosition;
vec2 cameraAngle;
mat4 ModelMatrix;
mat4 ViewMatrix;
mat4 ProjectionMatrix;
mat4 MVP;

//VAO
//GLuint VAO;
GLuint* VAO;

//buffers
GLuint* verticesBuffer;
GLuint* indicesBuffer;
GLuint gridBuffer;

//Functions
int initFiles(int argc, char* argv[]);
int initGame();
void initShaders();
void initObjects();
void initVAO();
void initMap();
void initVertices();
void initIndices();
void initGrid();
void initViews();
vec3 direction();
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void moveControls();
void countFPS();
void calculateMVP();
void drawObjects();
void drawGrid();

int main(int argc, char *argv[]) {
	if (initFiles(argc, argv) != 0)
		return -1;

	if (initGame() != 0)
		return -1;

	initShaders();
	initVAO();
	initMap();
	initVertices();
	initIndices();
	initGrid();
	initViews();

	glfwSetKeyCallback(window, key_callback);
	last_time = glfwGetTime();
	last_reset = last_time;
	do{
		moveControls();
		countFPS();

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		(view2d) ? glUseProgram(programID) : glUseProgram(programID3d);

		calculateMVP();
		drawObjects();
		if (!view2d)
			drawGrid();

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&	glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteProgram(programID);
	glDeleteProgram(programID3d);
	glDeleteBuffers(numberOfObjects, verticesBuffer);
	glDeleteBuffers(numberOfLODs, indicesBuffer);
	glDeleteVertexArrays(numberOfObjects, VAO);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

int initFiles(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Please enter name of map-file as argument, example: zad5.exe n50e014.hgt OR zad5.exe -range W E S N \n";
		return -1;
	}

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-') {
			fileNames.push_back(std::string(argv[i]));
			//std::cout << argv[i];
			//std::cout << fileNames.size();
		}
		else if (std::string(argv[i]) == "-range") {
			//std::cout << "OK";
			fromRange = true;
			break;
		}
	}

	if (!fromRange) {
		numberOfObjects = fileNames.size();
		coords.resize(numberOfObjects);
	}
	else {
		int w = atoi(argv[2]), e = atoi(argv[3]), s = atoi(argv[4]), n = atoi(argv[5]);
		//std::cout << w << " " << e << " " << s << " " << n;
		for (int i = w; i <= e; i++) {
			std::string ii = std::to_string(i);
			while (ii.size() < 3)
				ii = "0" + ii;
			for (int j = s; j <= n; j++) {
				std::string jj = std::to_string(j);
				while (jj.size() < 2)
					jj = "0" + jj;
				if (i < 0)
					if (j < 0)
						fileNames.push_back("s" + jj + "w" + ii + ".hgt");
					else
						fileNames.push_back("n" + jj + "w" + ii + ".hgt");
				else
					if (j < 0)
						fileNames.push_back("s" + jj + "e" + ii + ".hgt");
					else
						fileNames.push_back("n" + jj + "e" + ii + ".hgt");
			}
		}
		//for (int i = 0; i < fileNames.size(); i++)
		//std::cout << fileNames[i];

		numberOfObjects = fileNames.size();
		coords.resize(numberOfObjects);
	}
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
	window = glfwCreateWindow(700, 700, "Terrain, Kamil Bebenek, 258340", NULL, NULL);
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

	//hide cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	// background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	return 0;
}

void initShaders() {
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("vertexShader2d.vertexshader", "FragmentShader.fragmentshader");
	programID3d = LoadShaders("vertexShader3d.vertexshader", "FragmentShader.fragmentshader");

	// Get a handle for our "MVP" uniform
	MVP_ID = glGetUniformLocation(programID, "MVP");
	//ColourID = glGetUniformLocation(programID, "colour");
	CoordinatesID = glGetUniformLocation(programID, "coordinates");
	MVP_ID3d = glGetUniformLocation(programID3d, "MVP");
	//ColourID3d = glGetUniformLocation(programID3d, "colour");
	CoordinatesID3d = glGetUniformLocation(programID3d, "coordinates");
}

void initVAO() {
	VAO = new GLuint[numberOfObjects + 1];
	glGenVertexArrays(numberOfObjects + 1, VAO);
}

void initMap() {
	for (int i = 0; i<360; i++)
		for (int y = 0; y < 180; y++)
			mapElements[i][y] = -1;
}

void initVertices() {
	verticesBuffer = new GLuint[numberOfObjects];
	glGenBuffers(numberOfObjects, verticesBuffer);
	for (unsigned int i = 0; i < numberOfObjects; i++)
	{
		std::vector< int > vertexPositionsVec(3 * mapSize); //(x, y, h)
		std::ifstream file(fileNames[i].c_str(), std::ios::in | std::ios::binary);
		if (!file) {
			std::cout << "Error opening: " << fileNames[i] << std::endl;
		}
		else {
			unsigned char buffer[2];
			for (int j = 0, k = 0; j < mapSize; j++, k += 3) {
				if (!file.read(reinterpret_cast<char*>(buffer), sizeof(buffer))) {
					std::cout << "Error reading: " << fileNames[j] << std::endl;
				}
				vertexPositionsVec[k] = j % edgeSize;
				vertexPositionsVec[k + 1] = j / edgeSize;
				vertexPositionsVec[k + 2] = (buffer[0] << 8) | buffer[1];
				if (vertexPositionsVec[k + 2] < -500 || vertexPositionsVec[k + 2] > 9000)
					vertexPositionsVec[k + 2] = 0;
			}
		}
		file.close();

		char c1, c2;
		sscanf(fileNames[i].c_str(), "%c%d%c%d", &c1, &coords[i].first, &c2, &coords[i].second);
		if (c1 == 'S' || c1 == 's')
			coords[i].first *= -1;
		if (c2 == 'W' || c2 == 'w')
			coords[i].second *= -1;

		if (coords[i].first < minNS)
			minNS = coords[i].first;
		if (coords[i].first > maxNS)
			maxNS = coords[i].first;
		if (coords[i].second < minEW)
			minEW = coords[i].second;
		if (coords[i].second > maxEW)
			maxEW = coords[i].second;

		glBindVertexArray(VAO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, verticesBuffer[i]);
		//size_t dataOffset = sizeof(int) * 3 * edgeSize*edgeSize;
		glVertexAttribPointer(0, 3, GL_INT, GL_FALSE, 0, (void*)0);
		glBufferData(GL_ARRAY_BUFFER, sizeof(int) * 3 * mapSize, &vertexPositionsVec[0], GL_STATIC_DRAW);
		mapElements[coords[i].second + 180][coords[i].first + 90] = i;
		//std::cout << coords[i].second + 180 << " " << coords[i].first + 90 << std::endl;
	}
	//std::cout << "minNS: " << minNS << ", maxNS: " << maxNS << ", minEW: " << minEW << ", maxEW: " << maxEW << std::endl;
}

void initIndices() {
	indicesBuffer = new GLuint[numberOfLODs];
	glGenBuffers(numberOfLODs, indicesBuffer);

	numberOfIndices.resize(numberOfLODs);

	for (int i = 0, density = 1; i < numberOfLODs; i++, density *= 2) { // good for 5 numberOfLODs max 
		numberOfIndices[i] = (edgeSize - 1) / density;
		numberOfIndices[i] = 6 * numberOfIndices[i] * numberOfIndices[i];
		GLuint* indices = new GLuint[numberOfIndices[i]];
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer[i]);
		int j = 0;
		for (int y = edgeSize - 1 - density, lasty = edgeSize - 1; y >= 0; y -= density) {
			for (int lastx = 0, x = density; x < edgeSize; x += density, j += 6) {
				indices[j] = lasty * edgeSize + lastx;
				indices[j + 1] = y * edgeSize + lastx;
				indices[j + 2] = lasty * edgeSize + x;
				indices[j + 3] = lasty * edgeSize + x;
				indices[j + 4] = y * edgeSize + lastx;
				indices[j + 5] = y * edgeSize + x;
				lastx = x;
			}
			lasty = y;
		}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*numberOfIndices[i], indices, GL_STATIC_DRAW);
	}
}

void initGrid() {
	for (float y = -90.0f; y < 90.0f; y += 5.0f) {
		for (float x = -180.0f; x < 180.0; x += 5.0f) {
			grid.push_back(x * 1200);
			grid.push_back(y * 1200);
			grid.push_back(3000);
		}
	}

	glGenBuffers(1, &gridBuffer);
	glBindVertexArray(VAO[numberOfObjects]);
	glBindBuffer(GL_ARRAY_BUFFER, gridBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * grid.size(), grid.data(), GL_STATIC_DRAW);
}

void initViews() {
	if (view2d)
		cameraPosition = 0.5f * vec3((minEW + maxEW), (minNS + maxNS), glm::max(abs(maxEW - minEW), abs(maxNS - minNS)));
	else {
		FoV = 45.0f;
		cameraPosition = vec3(0.0f, -3.14f, 0.0f);
		cameraAngle = vec2(1.57f, 0.0f);
	}
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		LOD = 0;
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		LOD = 1;
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
		LOD = 2;
	if (key == GLFW_KEY_4 && action == GLFW_PRESS)
		LOD = 3;
	if (key == GLFW_KEY_5 && action == GLFW_PRESS)
		LOD = 4;
	if (key == GLFW_KEY_0 && action == GLFW_PRESS)
		autoLOD = !autoLOD;
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
		view2d = !view2d;
		initViews();
	}
}

void moveControls() {
	static double lastTime = glfwGetTime();
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);
	float delta = 1.0f * deltaTime * speed;
	if (view2d) {
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			if (cameraPosition.z - delta >= 0.0f)
				cameraPosition.z -= delta;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			cameraPosition.x -= delta;
		}
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			cameraPosition.y += delta;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			cameraPosition.y -= delta;
		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			cameraPosition.z += delta;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			cameraPosition.x += delta;
		}
	}
	else {
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			cameraPosition -= direction() * delta * speed;
			//std::cout << cameraPosition.x << " " << cameraPosition.y << " " << cameraPosition.z << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			//if (cameraAngle.x + delta < 3.14f)
			cameraAngle += vec2(delta, 0.0f);
			//std::cout << cameraAngle.x << " " << cameraAngle.y << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			if (cameraAngle.y + delta < 1.57f)
				cameraAngle += vec2(0.0f, delta);
			//std::cout << cameraAngle.x << " " << cameraAngle.y << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			if (cameraAngle.y - delta > -1.57f)
				cameraAngle -= vec2(0.0f, delta);
			//std::cout << cameraAngle.x << " " << cameraAngle.y << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			cameraPosition += direction() * delta * speed;
			//std::cout << cameraPosition.x << " " << cameraPosition.y << " " << cameraPosition.z << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			//if (cameraAngle.x + delta > 0.0f)
			cameraAngle -= vec2(delta, 0.0f);
			//std::cout << cameraAngle.x << " " << cameraAngle.y << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
			FoV += 0.01f;
		if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
			FoV -= 0.01f;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		speed += 0.01f;
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		if (speed > 0.011f)
			speed -= 0.01f;
	}

	// Get mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Reset mouse position for next frame
	glfwSetCursorPos(window, 800 / 2, 800 / 2);

	// Compute new orientation
	//std::cout << cameraAngle.x << " " << cameraAngle.y << std::endl;
	cameraAngle.x += mouseSpeed * float(800 / 2 - xpos);
	double mouseDeltaY = mouseSpeed * float(800 / 2 - ypos);
	if (cameraAngle.y + mouseDeltaY < 1.57f && cameraAngle.y + mouseDeltaY > -1.57f)
		cameraAngle.y += mouseDeltaY;
	//std::cout << cameraAngle.x << " " << cameraAngle.y << std::endl;

	lastTime = currentTime;
}

vec3 direction() {
	return vec3(
		std::cos(cameraAngle.y) * std::cos(cameraAngle.x),
		std::cos(cameraAngle.y) * std::sin(cameraAngle.x),
		std::sin(cameraAngle.y)
		);
}

void countFPS() {
	FPScounter++;
	double cur_time = glfwGetTime();
	if (cur_time - last_reset >= 1.0) {
		double FPS = (float)FPScounter / (cur_time - last_reset);
		std::cout << "FPS: " << FPS << ", LOD: " << LOD + 1 << ", Triangles:" << numberOfObjects * numberOfIndices[LOD] / 3 << std::endl;
		if (autoLOD) {
			if (FPS < minFPS && LOD < numberOfLODs)
				LOD++;
			if (FPS > 5 * minFPS && LOD > 0)
				LOD--;
		}
		last_reset = cur_time;
		FPScounter = 0;
	}
	last_time = cur_time;
}

void calculateMVP() {
	if (view2d) {
		GLfloat angle = cos(radians(abs((minNS + maxNS) * 0.5f)));
		ProjectionMatrix = ortho(-cameraPosition.z, cameraPosition.z, -cameraPosition.z * angle, cameraPosition.z * angle); //ortho (T const &left, T const &right, T const &bottom, T const &top)
		ViewMatrix = translate(mat4(1.0f), vec3(-vec2(cameraPosition), 0.0f)); // or glm::lookAt
		ModelMatrix = scale(glm::mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));
	}
	else {
		ProjectionMatrix = perspective(FoV, 1.0f, 0.001f, 100000.0f);
		ViewMatrix = lookAt(cameraPosition * 6378.0f, cameraPosition * 6378.0f + direction(), vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = glm::mat4(1.0f);
	}

	MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
}

void drawObjects() {
	(view2d) ? glUniformMatrix4fv(MVP_ID, 1, GL_FALSE, &MVP[0][0]) : glUniformMatrix4fv(MVP_ID3d, 1, GL_FALSE, &MVP[0][0]);

	for (int i = 0; i < coords.size(); i++) {
		if (view2d)
			glUniform2f(CoordinatesID, coords[i].second, coords[i].first);
		else
			glUniform2f(CoordinatesID3d, coords[i].second, coords[i].first);

		int index = mapElements[coords[i].second + 180][coords[i].first + 90];

		glBindVertexArray(VAO[index]);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, verticesBuffer[index]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer[LOD]);
		glDrawElements(GL_TRIANGLES, numberOfIndices[LOD], GL_UNSIGNED_INT, 0);
		glDisableVertexAttribArray(0);
	}
}

void drawGrid() {
	glUniformMatrix4fv(MVP_ID3d, 1, GL_FALSE, &MVP[0][0]);
	glUniform2f(CoordinatesID3d, 0, 0);
	glBindVertexArray(VAO[numberOfObjects]);
	glBindBuffer(GL_ARRAY_BUFFER, gridBuffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glDrawArrays(GL_POINTS, 0, grid.size());
	glDisableVertexAttribArray(0);
}