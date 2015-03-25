// use when we have sprintf error/warning (text2D)
#ifdef _WIN32
	#define _CRT_SECURE_NO_DEPRECATE
#endif

// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>

using namespace glm;

#include "shader.hpp"

#include "text2D.hpp"

#include <iostream>

// random numbers generator
#include <random>

//sleep
#include <chrono>
#include <thread>

// Variables
int goodSolution[4][4]; // 1..8 - colours, 0 - guessed
int second[4][4] = { 0 }; 
bool endGame = false;
int currentAttempt = 0;
int currentColumn = 0; //current position
int currentRow = 0; //current position
int answer1_x, answer1_y, answer2_x, answer2_y;
int goodAnswers = 0; // if == 8 then end game
int selected = 0; //number of selected squares by user
double lastTime = glfwGetTime();
bool isAnimating = false;
bool visibleShapes[17] = { 0 };

GLfloat colours[9][3] = { 
	0.0f, 0.4f, 0.8f, // background colour
	1.0f, 0.0f, 0.0f, // 1
	0.0f, 0.5f, 0.0f,
	0.0f, 0.0f, 0.1f,
	1.0f, 0.5f, 0.1f, // 4
	1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	0.1f, 1.0f, 0.2f, // 7
	1.0f, 1.0f, 0.1f
};

GLuint vertexbuffer;
GLuint colourbuffer;
GLuint programID;

// DATA
// 0 - 191: main squares
// 192 - 206: border aroud squares
// 207 - 263: lines (board)
// : shapes
// 264 - 272 ::: 1st triangle
// 273 - 281 ::: 2nd
// 282 - 293 ::: 1st plus
// 294 - 305 ::: 2nd
// 306 - 311 ::: 1st minus
// 312 - 317 ::: 2nd
// 318 - 329 ::: 1st parallelogram
// 330 - 341 ::: 2nd
// 342 - 353 ::: 1st diamond
// 354 - 365 ::: 2nd
// 366 - 377 ::: 1st /|/
// 378 - 389 ::: 2nd
// 390 - 395 ::: 1st '\'
// 396 - 401 ::: 2nd
// 402 - 407 ::: 1st '/'
// 408 - 413 ::: 2nd

static GLfloat g_vertex_buffer_data[414];
static GLfloat g_colour_buffer_data[414];

// Functions declarations
int generateRandomNumber();

void generateSolution(int solution[4][4]);

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

void checkAnswer();
void borderVertices();
void linesVertices(); 
void shapesVertices();
void calculateShapes();
void colourSquare(int y, int x, int value);
void graySquare(int y, int x);
void draw();
void animation(int value, int y, int x, double dTime, bool action);

int main(void)
{
	generateSolution(goodSolution);

	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(668, 668, "Memory-pairs by Kamil Bebenek, 258340", NULL, NULL);
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

	// blue background
	glClearColor(0.0f, 0.4f, 0.8f, 0.0f);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("VertexShader.vertexshader", "FragmentShader.fragmentshader");

	// grey squares
	for (int i = 0; i < 192; i++) g_colour_buffer_data[i] = 0.5f;

	// main squares verticles
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			// bottom-left (x,y,z)
			g_vertex_buffer_data[(i * 48) + (j * 12)] = -0.75f + (j * 0.4f);
			g_vertex_buffer_data[(i * 48) + (j * 12) + 1] = -0.9f + (i * 0.4f);
			g_vertex_buffer_data[(i * 48) + (j * 12) + 2] = 0.0f;

			// top-left (x,y,z)
			g_vertex_buffer_data[(i * 48) + (j * 12) + 3] = -0.75f + (j * 0.4f);
			g_vertex_buffer_data[(i * 48) + (j * 12) + 4] = -0.6f + (i * 0.4f);
			g_vertex_buffer_data[(i * 48) + (j * 12) + 5] = 0.0f;

			// top-right (x,y,z)
			g_vertex_buffer_data[(i * 48) + (j * 12) + 6] = -0.45f + (j * 0.4f);
			g_vertex_buffer_data[(i * 48) + (j * 12) + 7] = -0.6f + (i * 0.4f);
			g_vertex_buffer_data[(i * 48) + (j * 12) + 8] = 0.0f;

			// bottom-right (x,y,z)
			g_vertex_buffer_data[(i * 48) + (j * 12) + 9] = -0.45f + (j * 0.4f);
			g_vertex_buffer_data[(i * 48) + (j * 12) + 10] = -0.9f + (i * 0.4f);
			g_vertex_buffer_data[(i * 48) + (j * 12) + 11] = 0.0f;
		}
	}

	// borders around squares
	borderVertices();

	// black border colour
	for (int i = 192; i <= 206; i++) g_colour_buffer_data[i] = 0.0f;

	// board lines verticles
	linesVertices();

	// white lines colour
	for (int i = 207; i <= 263; i++) g_colour_buffer_data[i] = 1.0f;

	// shapes
	shapesVertices();

	calculateShapes();

	// white shapes colour
	for (int i = 264; i <= 401; i++) g_colour_buffer_data[i] = 1.0f;

	// black '/' shape
	for (int i = 402; i <= 413; i++) g_colour_buffer_data[i] = 0.0f;

	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &colourbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colourbuffer); //
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_colour_buffer_data), g_colour_buffer_data, GL_STATIC_DRAW);
	
	// keyboard
	glfwSetKeyCallback(window, key_callback);

	// Initialize our little text library with the Holstein font
	initText2D("Holstein.DDS");

	do{
		draw();
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &colourbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(programID);

	// Delete the text's VBO, the shader and the texture
	cleanupText2D();

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

int generateRandomNumber() {
	std::random_device rd; // seed

	static std::default_random_engine e{ rd() };
	static std::uniform_int_distribution<int> l{ 1, 8 };
	return l(e);
}

void generateSolution(int solution[4][4]) {
	int randomTmp[9] = { 0 }; // help generate board
	int randomNumber;
	bool correctNumber;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			correctNumber = false;
			do {
				randomNumber = generateRandomNumber();
				if (randomTmp[randomNumber] < 2) {
					solution[i][j] = randomNumber;

					if (randomTmp[randomNumber] == 1)
						second[i][j] = 1;

					randomTmp[randomNumber]++;
					correctNumber = true;
				}
			} while (!correctNumber);
		}
	}

#ifdef _DEBUG
	// print solution in console
	std::cout << "Solution: " << std::endl;
	for (int i = 3; i >= 0; i--) {
		for (int j = 0; j < 4; j++)
			std::cout << solution[i][j] << " ";
		std::cout << std::endl;
	}
#endif
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	// Strafe right
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS){
		currentColumn = (currentColumn + 1) % 4;
		borderVertices();
	}

	// Strafe left
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS){
		currentColumn = (currentColumn - 1) % 4;
		if (currentColumn < 0) 
			currentColumn += 4;
		borderVertices();
	}

	// Strafe up
	if (key == GLFW_KEY_UP && action == GLFW_PRESS){
		currentRow = (currentRow + 1) % 4;
		borderVertices();
	}

	// Strafe down
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS){
		currentRow = (currentRow - 1) % 4;
		if (currentRow < 0) currentRow += 4;
		borderVertices();
	}

	// Strafe space
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS &&!isAnimating){
		if ((goodSolution[currentRow][currentColumn] == 0) || (selected == 1 && answer1_x == currentColumn && answer1_y == currentRow)) //non-selected earlier
			return;
			if (selected == 0) {
				answer1_x = currentColumn;
				answer1_y = currentRow;
				colourSquare(answer1_y, answer1_x, goodSolution[answer1_y][answer1_x]); // with animation
			}
			if (selected == 1) {
				answer2_x = currentColumn;
				answer2_y = currentRow;
				colourSquare(answer2_y, answer2_x, goodSolution[answer2_y][answer2_x]); // with animation
				//draw();
			}
			
			selected++;

			if (selected == 2) {
				std::this_thread::sleep_for(std::chrono::milliseconds(400)); //sleep
				checkAnswer();
				currentAttempt++; //increase number of attempts
				selected = 0; // reset number of selected squares
			}
	}

	// Strafe enter
	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS){
		if (endGame) {
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					second[i][j] = 0;

			generateSolution(goodSolution);
			shapesVertices();
			calculateShapes();
			endGame = false;
			currentAttempt = 0;
			goodAnswers = 0;
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					graySquare(i, j);
		}
	}
}

void borderVertices() {
	// BORDER (around squares)
	// bottom-left (x,y,z)
	g_vertex_buffer_data[192] = -0.76f + (currentColumn * 0.4f);
	g_vertex_buffer_data[193] = -0.91f + (currentRow * 0.4f);
	g_vertex_buffer_data[194] = 0.0f;

	// top-left (x,y,z)
	g_vertex_buffer_data[195] = -0.76f + (currentColumn * 0.4f);
	g_vertex_buffer_data[196] = -0.59f + (currentRow * 0.4f);
	g_vertex_buffer_data[197] = 0.0f;

	// top-right (x,y,z)
	g_vertex_buffer_data[198] = -0.44f + (currentColumn * 0.4f);
	g_vertex_buffer_data[199] = -0.59f + (currentRow * 0.4f);
	g_vertex_buffer_data[200] = 0.0f;

	// bottom-right (x,y,z)
	g_vertex_buffer_data[201] = -0.44f + (currentColumn * 0.4f);
	g_vertex_buffer_data[202] = -0.91f + (currentRow * 0.4f);
	g_vertex_buffer_data[203] = 0.0f;

	//LINE_LOOP or fifth vertex
	g_vertex_buffer_data[204] = -0.76f + (currentColumn * 0.4f);
	g_vertex_buffer_data[205] = -0.91f + (currentRow * 0.4f);
	g_vertex_buffer_data[206] = 0.0f;

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
}

void checkAnswer() {
	if (goodSolution[answer1_y][answer1_x] == goodSolution[answer2_y][answer2_x]) {
		int value = goodSolution[answer1_y][answer1_x];
		goodSolution[answer1_y][answer1_x] = 0;
		goodSolution[answer2_y][answer2_x] = 0;
		colourSquare(answer1_y, answer1_x, value);
		colourSquare(answer2_y, answer2_x, value);
		goodAnswers++;
		if (goodAnswers == 8)
			endGame = true;
	} else {
		graySquare(answer1_y, answer1_x);
		graySquare(answer2_y, answer2_x);
	}
}

void colourSquare(int y, int x, int value) {
	animation(value, y, x, 0.002f, 1);

	for (int i = 0; i < 4; i++) {
		g_colour_buffer_data[(y * 48) + (x * 12) + (i * 3)] = colours[goodSolution[y][x]][0];
		g_colour_buffer_data[(y * 48) + (x * 12) + (i * 3) + 1] = colours[goodSolution[y][x]][1];
		g_colour_buffer_data[(y * 48) + (x * 12) + (i * 3) + 2] = colours[goodSolution[y][x]][2];
	}

	glBindBuffer(GL_ARRAY_BUFFER, colourbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_colour_buffer_data), g_colour_buffer_data, GL_STATIC_DRAW);

	animation(value, y, x, 0.002f, 0);
}

void graySquare(int y, int x) {
	animation(goodSolution[y][x], y, x, 0.0001f, 1);

	// new colour
	for (int i = 0; i < 4; i++) {
		g_colour_buffer_data[(y * 48) + (x * 12) + (i * 3)] = 0.5f;
		g_colour_buffer_data[(y * 48) + (x * 12) + (i * 3) + 1] = 0.5f;
		g_colour_buffer_data[(y * 48) + (x * 12) + (i * 3) + 2] = 0.5f;
	}

	glBindBuffer(GL_ARRAY_BUFFER, colourbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_colour_buffer_data), g_colour_buffer_data, GL_STATIC_DRAW);

	animation(goodSolution[y][x], y, x, 0.0001f, 0);
}

void draw() {
	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT);

	// Use our shader
	glUseProgram(programID);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);

	// 2nd attribute buffer : colours
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, colourbuffer);
	glVertexAttribPointer(
		1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);

	if (!endGame) {
		// DRAW !
		// squares
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				glDrawArrays(GL_TRIANGLE_FAN, (i * 16) + (j * 4), 4); // 3 indices starting at 0 -> 1 triangle
			}
		}
		
		// borders
		glDrawArrays(GL_LINE_STRIP, 64, 5); //GL_LINE_LOOP

		// lines (board)
		glDrawArrays(GL_LINE_STRIP, 69, 5); //GL_LINE_LOOP
		for (int i = 0; i < 4; i++)
			glDrawArrays(GL_LINES, 74 + (i * 2), 2);
		for (int i = 0; i < 3; i++)
			glDrawArrays(GL_LINES, 82 + (i * 2), 2);

		// shapes
		//triangles
		if (visibleShapes[1])
			glDrawArrays(GL_LINE_LOOP, 88, 3);
		if (visibleShapes[2])
			glDrawArrays(GL_LINE_LOOP, 91, 3);

		//pluses
		if (visibleShapes[3]) {
			glDrawArrays(GL_LINES, 94, 2);
			glDrawArrays(GL_LINES, 96, 2);
		}

		if (visibleShapes[4]) {
			glDrawArrays(GL_LINES, 98, 2);
			glDrawArrays(GL_LINES, 100, 2);
		}

		//minuses
		if (visibleShapes[5])
			glDrawArrays(GL_LINES, 102, 2);
		if (visibleShapes[6])
			glDrawArrays(GL_LINES, 104, 2);

		//
		if (visibleShapes[7])
			glDrawArrays(GL_LINE_LOOP, 106, 4);
		if (visibleShapes[8])
			glDrawArrays(GL_LINE_LOOP, 110, 4);

		// diamond
		if (visibleShapes[9])
			glDrawArrays(GL_LINE_LOOP, 114, 4);
		if (visibleShapes[10])
			glDrawArrays(GL_LINE_LOOP, 118, 4);

		// /|/
		if (visibleShapes[11])
			glDrawArrays(GL_LINE_STRIP, 122, 4);
		if (visibleShapes[12])
			glDrawArrays(GL_LINE_STRIP, 126, 4);

		// '/'
		if (visibleShapes[13])
			glDrawArrays(GL_LINES, 130, 2);
		if (visibleShapes[14])
			glDrawArrays(GL_LINES, 132, 2);

		// '\'
		if (visibleShapes[15])
			glDrawArrays(GL_LINES, 134, 2);
		if (visibleShapes[16])
			glDrawArrays(GL_LINES, 136, 2);
	}

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	// PoolEvents()
	// TEXT
	char text[256];
	sprintf(text, "Tries: %i", currentAttempt);
	printText2D(text, 100, 505, 70);

	if (endGame) {
		char text2[256];
		sprintf(text2, "Press Enter to play again");
		printText2D(text2, 45, 300, 28);
	}

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void linesVertices() {
	// LINES (BOARD)

	//first line_strip
	// bottom-left (x,y,z)
	g_vertex_buffer_data[207] = -0.8f;
	g_vertex_buffer_data[208] = -0.95f;
	g_vertex_buffer_data[209] = 0.0f;

	// top-left (x,y,z)
	g_vertex_buffer_data[210] = -0.8f;
	g_vertex_buffer_data[211] = 0.95f;
	g_vertex_buffer_data[212] = 0.0f;

	// top-right (x,y,z)
	g_vertex_buffer_data[213] = 0.8f;
	g_vertex_buffer_data[214] = 0.95f;
	g_vertex_buffer_data[215] = 0.0f;

	// bottom-right (x,y,z)
	g_vertex_buffer_data[216] = 0.8f;
	g_vertex_buffer_data[217] = -0.95f;
	g_vertex_buffer_data[218] = 0.0f;

	//LINE_LOOP or fifth vertex
	g_vertex_buffer_data[219] = -0.8f;
	g_vertex_buffer_data[220] = -0.95f;
	g_vertex_buffer_data[221] = 0.0f;

	//horizontal lines
	for (int i = 0; i < 4; i++) {
		//left
		g_vertex_buffer_data[(i * 6) + 222] = -0.8f;
		g_vertex_buffer_data[(i * 6) + 223] = -0.55f + (i * 0.4f);
		g_vertex_buffer_data[(i * 6) + 224] = 0.0f;

		//right
		g_vertex_buffer_data[(i * 6) + 225] = 0.8f;
		g_vertex_buffer_data[(i * 6) + 226] = -0.55f + (i * 0.4f);
		g_vertex_buffer_data[(i * 6) + 227] = 0.0f;
	}

	//vertical lines
	for (int i = 0; i < 3; i++) {
		//bottom
		g_vertex_buffer_data[(i * 6) + 246] = -0.4f + (i * 0.4f);
		g_vertex_buffer_data[(i * 6) + 247] = -0.95f;
		g_vertex_buffer_data[(i * 6) + 248] = 0.0f;

		//top
		g_vertex_buffer_data[(i * 6) + 249] = -0.4f + (i * 0.4f);
		g_vertex_buffer_data[(i * 6) + 250] = 0.65f;
		g_vertex_buffer_data[(i * 6) + 251] = 0.0f;
	}

}

void animation(int value, int y, int x, double dTime, bool action) {
	if (action == 1)
		isAnimating = true;

	// animationShape
	int x1, x2;

	int whichShape;
	if (!second[y][x])
		whichShape = (value * 2) - 1;
	else
		whichShape = value * 2;

	switch (value) {
	case 1:
		x1 = 264 + (second[y][x] * 9);
		x2 = 270 + (second[y][x] * 9);
		break;
	case 2:
		x1 = 282 + (second[y][x] * 12);
		x2 = 285 + (second[y][x] * 12);
		break;
	case 3:
		x1 = 306 + (second[y][x] * 6);
		x2 = 309 + (second[y][x] * 6);
		break;
	case 4:
		x1 = 321 + (second[y][x] * 12);
		x2 = 327 + (second[y][x] * 12);
		break;
	case 5:
		x1 = 345 + (second[y][x] * 12);
		x2 = 351 + (second[y][x] * 12);
		break;
	case 6:
		x1 = 366 + (second[y][x] * 12);
		x2 = 375 + (second[y][x] * 12);
		break;
	case 7:
		x1 = 390 + (second[y][x] * 6);
		x2 = 393 + (second[y][x] * 6);
		break;
	case 8:
		x1 = 402 + (second[y][x] * 6);
		x2 = 405 + (second[y][x] * 6);
		break;
	}
	//

	for (int d = 0; d < 100; d++) {
		for (int i = 0; i < 2; i++) {
			if (action == true)
				g_vertex_buffer_data[(y * 48) + (x * 12) + (i * 3)] += 0.0015f;
			else
				g_vertex_buffer_data[(y * 48) + (x * 12) + (i * 3)] -= 0.0015f;
		}
		for (int i = 0; i < 2; i++) {
			if (action == true)
				g_vertex_buffer_data[(y * 48) + (x * 12) + (i * 3) + 6] -= 0.0015f;
			else
				g_vertex_buffer_data[(y * 48) + (x * 12) + (i * 3) + 6] += 0.0015f;
		}


	// animation SHAPE
		if (action == true)
			g_vertex_buffer_data[x1] += 0.001f;
		else
			g_vertex_buffer_data[x1] -= 0.001f;

		if (action == true)
			g_vertex_buffer_data[x2] -= 0.001f;
		else
			g_vertex_buffer_data[x2] += 0.001f;



		double xTime;
		do {
			xTime = glfwGetTime() - lastTime;
		} while (xTime < dTime);

		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

		draw();
		lastTime += xTime;
	}

	if (action == 1) {
		if (dTime == 0.002f)
			visibleShapes[whichShape] = true;
		else 
			visibleShapes[whichShape] = false;
	}

	if (goodSolution[y][x] == 0)
		visibleShapes[whichShape] = false;

	if (action == 0)
		isAnimating = false;
}

void shapesVertices() {
	//traingles
	for (int i = 0; i < 2; i++) {
		// bottom-left (x,y,z)
		g_vertex_buffer_data[264 + (i * 9)] = 0.05f;
		g_vertex_buffer_data[265 + (i * 9)] = 0.05f;
		g_vertex_buffer_data[266 + (i * 9)] = 0.0f;

		// top (x,y,z)
		g_vertex_buffer_data[267 + (i * 9)] = 0.15f;
		g_vertex_buffer_data[268 + (i * 9)] = 0.25f;
		g_vertex_buffer_data[269 + (i * 9)] = 0.0f;

		// bottom-right (x,y,z)
		g_vertex_buffer_data[270 + (i * 9)] = 0.25f;
		g_vertex_buffer_data[271 + (i * 9)] = 0.05f;
		g_vertex_buffer_data[272 + (i * 9)] = 0.0f;
	}

	//plus
	for (int i = 0; i < 2; i++) {
		// left (x,y,z)
		g_vertex_buffer_data[282 + (i * 12)] = 0.05f;
		g_vertex_buffer_data[283 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[284 + (i * 12)] = 0.0f;

		// right (x,y,z)
		g_vertex_buffer_data[285 + (i * 12)] = 0.25f;
		g_vertex_buffer_data[286 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[287 + (i * 12)] = 0.0f;

		// top (x,y,z)
		g_vertex_buffer_data[288 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[289 + (i * 12)] = 0.25f;
		g_vertex_buffer_data[290 + (i * 12)] = 0.0f;

		// bottom (x,y,z)
		g_vertex_buffer_data[291 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[292 + (i * 12)] = 0.05f;
		g_vertex_buffer_data[293 + (i * 12)] = 0.0f;
	}

	//minus
	for (int i = 0; i < 2; i++) {
		// left (x,y,z)
		g_vertex_buffer_data[306 + (i * 6)] = 0.05f;
		g_vertex_buffer_data[307 + (i * 6)] = 0.15f;
		g_vertex_buffer_data[308 + (i * 6)] = 0.0f;

		// right (x,y,z)
		g_vertex_buffer_data[309 + (i * 6)] = 0.25f;
		g_vertex_buffer_data[310 + (i * 6)] = 0.15f;
		g_vertex_buffer_data[311 + (i * 6)] = 0.0f;
	}

	// parallelogram
	for (int i = 0; i < 2; i++) {
		// bottom (x,y,z)
		g_vertex_buffer_data[318 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[319 + (i * 12)] = 0.05f;
		g_vertex_buffer_data[320 + (i * 12)] = 0.0f;

		// left (x,y,z)
		g_vertex_buffer_data[321 + (i * 12)] = 0.05f;
		g_vertex_buffer_data[322 + (i * 12)] = 0.25f;
		g_vertex_buffer_data[323 + (i * 12)] = 0.0f;

		// top (x,y,z)
		g_vertex_buffer_data[324 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[325 + (i * 12)] = 0.25f;
		g_vertex_buffer_data[326 + (i * 12)] = 0.0f;

		// right (x,y,z)
		g_vertex_buffer_data[327 + (i * 12)] = 0.25f;
		g_vertex_buffer_data[328 + (i * 12)] = 0.05f;
		g_vertex_buffer_data[329 + (i * 12)] = 0.0f;
	}

	// diamond
	for (int i = 0; i < 2; i++) {
		// bottom (x,y,z)
		g_vertex_buffer_data[342 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[343 + (i * 12)] = 0.05f;
		g_vertex_buffer_data[344 + (i * 12)] = 0.0f;

		// left (x,y,z)
		g_vertex_buffer_data[345 + (i * 12)] = 0.05f;
		g_vertex_buffer_data[346 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[347 + (i * 12)] = 0.0f;

		// top (x,y,z)
		g_vertex_buffer_data[348 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[349 + (i * 12)] = 0.25f;
		g_vertex_buffer_data[350 + (i * 12)] = 0.0f;

		// right (x,y,z)
		g_vertex_buffer_data[351 + (i * 12)] = 0.25f;
		g_vertex_buffer_data[352 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[353 + (i * 12)] = 0.0f;
	}

	// /|/
	for (int i = 0; i < 2; i++) {
		// left (x,y,z)
		g_vertex_buffer_data[366 + (i * 12)] = 0.05f;
		g_vertex_buffer_data[367 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[368 + (i * 12)] = 0.0f;

		// top (x,y,z)
		g_vertex_buffer_data[369 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[370 + (i * 12)] = 0.25f;
		g_vertex_buffer_data[371 + (i * 12)] = 0.0f;

		// bottom (x,y,z)
		g_vertex_buffer_data[372 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[373 + (i * 12)] = 0.05f;
		g_vertex_buffer_data[374 + (i * 12)] = 0.0f;

		// right (x,y,z)
		g_vertex_buffer_data[375 + (i * 12)] = 0.25f;
		g_vertex_buffer_data[376 + (i * 12)] = 0.15f;
		g_vertex_buffer_data[377 + (i * 12)] = 0.0f;
	}

	// '\'
	for (int i = 0; i < 2; i++) {
		// left (x,y,z)
		g_vertex_buffer_data[390 + (i * 6)] = 0.05f;
		g_vertex_buffer_data[391 + (i * 6)] = 0.25f;
		g_vertex_buffer_data[392 + (i * 6)] = 0.0f;

		// right (x,y,z)
		g_vertex_buffer_data[393 + (i * 6)] = 0.25f;
		g_vertex_buffer_data[394 + (i * 6)] = 0.05f;
		g_vertex_buffer_data[395 + (i * 6)] = 0.0f;
	}

	// '/'
	for (int i = 0; i < 2; i++) {
		// left (x,y,z)
		g_vertex_buffer_data[402 + (i * 6)] = 0.05f;
		g_vertex_buffer_data[403 + (i * 6)] = 0.05f;
		g_vertex_buffer_data[404 + (i * 6)] = 0.0f;

		// right (x,y,z)
		g_vertex_buffer_data[405 + (i * 6)] = 0.25f;
		g_vertex_buffer_data[406 + (i * 6)] = 0.25f;
		g_vertex_buffer_data[407 + (i * 6)] = 0.0f;
	}
}

void calculateShapes() {
	int countShapes[9] = { 0 };
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			switch (goodSolution[i][j]) {
			case 1:
				for (int k = 0; k < 3; k++)
					g_vertex_buffer_data[264 + (9 * countShapes[1]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12)];

				for (int k = 0; k < 3; k++)
					g_vertex_buffer_data[265 + (9 * countShapes[1]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12) + 1];

				countShapes[1]++;
				break;

			case 2:
				for (int k = 0; k < 4; k++)
					g_vertex_buffer_data[282 + (12 * countShapes[2]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12)];

				for (int k = 0; k < 4; k++)
					g_vertex_buffer_data[283 + (12 * countShapes[2]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12) + 1];

				countShapes[2]++;
				break;

			case 3:
				for (int k = 0; k < 2; k++)
					g_vertex_buffer_data[306 + (6 * countShapes[3]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12)];

				for (int k = 0; k < 2; k++)
					g_vertex_buffer_data[307 + (6 * countShapes[3]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12) + 1];

				countShapes[3]++;
				break;

			case 4:
				for (int k = 0; k < 4; k++)
					g_vertex_buffer_data[318 + (12 * countShapes[4]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12)];

				for (int k = 0; k < 4; k++)
					g_vertex_buffer_data[319 + (12 * countShapes[4]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12) + 1];

				countShapes[4]++;
				break;

			case 5:
				for (int k = 0; k < 4; k++)
					g_vertex_buffer_data[342 + (12 * countShapes[5]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12)];

				for (int k = 0; k < 4; k++)
					g_vertex_buffer_data[343 + (12 * countShapes[5]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12) + 1];

				countShapes[5]++;
				break;

			case 6:
				for (int k = 0; k < 4; k++)
					g_vertex_buffer_data[366 + (12 * countShapes[6]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12)];

				for (int k = 0; k < 4; k++)
					g_vertex_buffer_data[367 + (12 * countShapes[6]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12) + 1];

				countShapes[6]++;
				break;
			case 7:
				for (int k = 0; k < 2; k++)
					g_vertex_buffer_data[390 + (6 * countShapes[7]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12)];

				for (int k = 0; k < 2; k++)
					g_vertex_buffer_data[391 + (6 * countShapes[7]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12) + 1];

				countShapes[7]++;
				break;
			case 8:
				for (int k = 0; k < 2; k++)
					g_vertex_buffer_data[402 + (6 * countShapes[8]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12)];

				for (int k = 0; k < 2; k++)
					g_vertex_buffer_data[403 + (6 * countShapes[8]) + (k * 3)] += g_vertex_buffer_data[(i * 48) + (j * 12) + 1];

				countShapes[8]++;
				break;
			}
		}
	}
}