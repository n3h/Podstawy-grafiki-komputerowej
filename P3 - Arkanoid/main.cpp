// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <random>
#include "shader.hpp"

GLFWwindow* window;

using namespace glm;

// Constants
const double PI = 3.14159265358979323846;

// Variables
bool endGame = false;
double lastTime = glfwGetTime();
double dTime = 0;

GLuint tVector; // translation vector
GLuint tColour; // dynamic change colour

//data
int numberOfBallMiniTriangles = 32;
int numberOfBackgroundMiniTriangles = 6;

//ball
GLuint ball_vertexbuffer;
GLuint ball_colourbuffer;
static GLfloat ball_vertex_buffer_data[102];
static GLfloat ball_colour_buffer_data[102];
vec2 ballPosition = vec2(0.0, 0.0);
double ballRadius = 0.03f;
double angle = 270;
double speed = 0.012f;

// background
GLuint background_vertexbuffer;
GLuint background_colourbuffer;
static GLfloat background_vertex_buffer_data[24];
static GLfloat background_colour_buffer_data[24];

GLuint walls_vertexbuffer;
GLuint walls_colourbuffer;
static GLfloat walls_vertex_buffer_data[42];
static GLfloat walls_colour_buffer_data[42];

//BRICKS
GLuint back_brick_vertexbuffer;
GLuint back_brick_colourbuffer;
static GLfloat back_brick_vertex_buffer_data[3000];
static GLfloat back_brick_colour_buffer_data[3000];
vec2 brickPosition;

vec2 firstBrickPos = vec2(-0.48f, 0.15f);
vec2 bricksShift = vec2(0.19f, 0.15f);
double brickRadius = 0.07f;
double brickHeight = brickRadius * sqrt(3) / 2;
const int numberOfBricksRows = 4;
const int numberOfBricksColumns = 6;

bool isVisible[numberOfBricksRows][numberOfBricksColumns];

double bothRadius = ballRadius + brickRadius;

//paddle
GLuint paddle_vertexbuffer;
GLuint paddle_colourbuffer;
static GLfloat paddle_vertex_buffer_data[12];
static GLfloat paddle_colour_buffer_data[12];
vec2 paddlePosition = vec2(0, 0.0f);
float paddleSpeed = 1.0f;

//shaders
GLuint programID;

GLuint VertexArrayID;

// Functions declarations
int generateRandomNumber(int a, int b);
void ballData();
void initWalls();
void initRectangle(GLfloat *table, GLfloat *tableC, GLfloat x, GLfloat y, GLfloat height, GLfloat width, GLfloat r = 0, GLfloat g = 0, GLfloat b = 0);
void initPolygon(GLfloat *table, GLfloat *tableC, int numberOfVertices, GLfloat x, GLfloat y, GLfloat radius, GLfloat r = 0, GLfloat g = 0, GLfloat b = 0);
void initBuffers();
void drawWalls();
void drawPolygon(GLint bVertex, GLint bColour, int numberOfVertices, GLfloat x = 0, GLfloat y = 0);
void drawBall();
void drawRectangle(GLint bVertex, GLint bColour, GLfloat x = 0, GLfloat y = 0);
void drawBricks();
void drawBackround();
void draw();
void keyboard();
void changeBallPosition();
void checkAnswer(); // check whether we destroy all bricks
void paddleCollision();
void brickCollision();
void wallCollision();
void cleanUp();

int main(void)
{
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
	window = glfwCreateWindow(668, 668, "Arkanoid, Kamil Bebenek", NULL, NULL);
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

	// background
	glClearColor(0.0f, 0.0f, 1.0f, 0.0f);

	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("VertexShader.vertexshader", "FragmentShader.fragmentshader");

	tVector = glGetUniformLocation(programID, "translationVector");
	tColour = glGetUniformLocation(programID, "translationColour");

	initWalls();
	initPolygon(ball_vertex_buffer_data, ball_colour_buffer_data, numberOfBallMiniTriangles,
		0.0f, 0.0f, ballRadius, 0.6f, 0.6f, 1.0f);
	initPolygon(background_vertex_buffer_data, background_colour_buffer_data, numberOfBackgroundMiniTriangles,
		-1.0f, -1.0f, 0.08f, 0.1f, 0.1f, 0.8f);
	initPolygon(back_brick_vertex_buffer_data, back_brick_colour_buffer_data, numberOfBackgroundMiniTriangles,
		firstBrickPos.x, firstBrickPos.y, brickRadius, 0.0f, 0.0f, 0.0f);
	initRectangle(paddle_vertex_buffer_data, paddle_colour_buffer_data, -0.2f, -0.85f, 0.05f, 0.4f, 0.6f, 0.6f, 0.8f); //paddle

	for (int i = 0; i < numberOfBricksRows; i++)
		for (int j = 0; j < numberOfBricksColumns; j++)
			isVisible[i][j] = true;

	initBuffers();

	do{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		drawBackround();
		drawWalls();
		if (!endGame) {
			keyboard();
			drawBall();
		}
		drawRectangle(paddle_vertexbuffer, paddle_colourbuffer, paddlePosition.x, paddlePosition.y); // drawPaddle
		drawBricks();

		checkAnswer();

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwGetKey(window, GLFW_KEY_Q) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	cleanUp();

	return 0;
}

int generateRandomNumber(int a, int b) { // C++11
	std::random_device rd;

	static std::default_random_engine e{ rd() };
	static std::uniform_int_distribution<int> l{ a, b };
	return l(e);
}

void initPolygon(GLfloat *table, GLfloat *tableC, int numberOfVertices, GLfloat x, GLfloat y, GLfloat radius, GLfloat r, GLfloat g, GLfloat b) {
	table[0] = x;
	table[1] = y;
	table[2] = 0.0f;

	tableC[0] = 0.0f;
	tableC[1] = 0.0f;
	tableC[2] = 0.4f;

	float ballAngle = 2 * PI / numberOfVertices;

	for (int i = 0; i <= numberOfVertices; i++) {
		GLfloat alfa = ballAngle*i;
		GLfloat x_a = radius * cos(alfa);
		GLfloat y_a = radius * sin(alfa);

		table[i * 3 + 3] = x_a + table[0];
		table[i * 3 + 4] = y_a + table[1];
		table[i * 3 + 5] = 0.0f;

		tableC[i * 3 + 3] = r;
		tableC[i * 3 + 4] = g;
		tableC[i * 3 + 5] = b;
	}
}

void initWalls() {
	initRectangle(walls_vertex_buffer_data, walls_colour_buffer_data, -1.0f, -1.0f, 0.134, 2.0f, 0, 0, 1.0f);
	
	walls_vertex_buffer_data[12] = -1.0f;
	walls_vertex_buffer_data[13] = -0.866f;
	walls_vertex_buffer_data[15] = -0.5f;
	walls_vertex_buffer_data[16] = -0.866f;
	walls_vertex_buffer_data[18] = -1.0f;
	walls_vertex_buffer_data[19] = 0.0f;

	walls_vertex_buffer_data[21] = -1.0f;
	walls_vertex_buffer_data[22] = 0.866f;
	walls_vertex_buffer_data[24] = -0.5f;
	walls_vertex_buffer_data[25] = 0.866f;

	walls_vertex_buffer_data[27] = 0.5f;
	walls_vertex_buffer_data[28] = 0.866f;
	walls_vertex_buffer_data[30] = 1.0f;
	walls_vertex_buffer_data[31] = 0.866f;
	walls_vertex_buffer_data[33] = 1.0f;
	walls_vertex_buffer_data[34] = 0.0f;

	walls_vertex_buffer_data[36] = 1.0f;
	walls_vertex_buffer_data[37] = -0.866f;
	walls_vertex_buffer_data[39] = 0.5f;
	walls_vertex_buffer_data[40] = -0.866f;

	for (int i = 14; i < 42; i += 3)
		walls_vertex_buffer_data[i] = 0.0f; //z

	for (int i = 12; i < 40; i += 3) {
		walls_colour_buffer_data[i] = 0.0f;
		walls_colour_buffer_data[i + 1] = 0.0f;
		walls_colour_buffer_data[i + 2] = 1.0f;
	}
}

void initRectangle(GLfloat *table, GLfloat *tableC, GLfloat x, GLfloat y, GLfloat height, GLfloat width, GLfloat r, GLfloat g, GLfloat b) {
	table[0] = x;
	table[1] = y;
	table[3] = x;
	table[4] = y + height;
	table[6] = x + width;
	table[7] = y + height;
	table[9] = x + width;
	table[10] = y;

	table[2] = table[5] = table[8] = table[11] = 0.0f; //z

	for (int i = 0; i < 12; i += 3) {
		tableC[i] = r;
		tableC[i + 1] = g;
		tableC[i + 2] = b;
	}
}

void initBuffers() {
	//ball
	glGenBuffers(1, &ball_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, ball_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ball_vertex_buffer_data), ball_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &ball_colourbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, ball_colourbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ball_colour_buffer_data), ball_colour_buffer_data, GL_STATIC_DRAW);

	//background
	glGenBuffers(1, &background_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, background_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(background_vertex_buffer_data), background_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &background_colourbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, background_colourbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(background_colour_buffer_data), background_colour_buffer_data, GL_STATIC_DRAW);

	//walls
	glGenBuffers(1, &walls_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, walls_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(walls_vertex_buffer_data), walls_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &walls_colourbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, walls_colourbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(walls_colour_buffer_data), walls_colour_buffer_data, GL_STATIC_DRAW);

	//back bricks
	glGenBuffers(1, &back_brick_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, back_brick_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(back_brick_vertex_buffer_data), back_brick_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &back_brick_colourbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, back_brick_colourbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(back_brick_colour_buffer_data), back_brick_colour_buffer_data, GL_STATIC_DRAW);

	//paddle
	glGenBuffers(1, &paddle_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, paddle_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(paddle_vertex_buffer_data), paddle_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &paddle_colourbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, paddle_colourbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(back_brick_colour_buffer_data), paddle_colour_buffer_data, GL_STATIC_DRAW);
}

void drawWalls() {
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, walls_vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 2nd attribute buffer : colours
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, walls_colourbuffer);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glUniform3f(tVector, 0.0, 1.866f, 0.0f);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glUniform3f(tVector, 0.0f, 0.0f, 0.0f);

	glDrawArrays(GL_TRIANGLE_FAN, 4, 3);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 3);
	glDrawArrays(GL_TRIANGLE_FAN, 9, 3);
	glDrawArrays(GL_TRIANGLE_FAN, 11, 3);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void drawBall() {
	dTime = glfwGetTime() - lastTime;
	if (dTime > 0.01f) {
		changeBallPosition();
		lastTime += dTime;
	}

	drawPolygon(ball_vertexbuffer, ball_colourbuffer, numberOfBallMiniTriangles, ballPosition.x, ballPosition.y);
}

void drawPolygon(GLint bVertex, GLint bColour, int numberOfVertices, GLfloat x, GLfloat y) {
	glUniform3f(tVector, x, y, 0.0f);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, bVertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 2nd attribute buffer : colours
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, bColour);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLE_FAN, 0, numberOfVertices + 2);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glUniform3f(tVector, 0.0f, 0.0f, 0.0f);
}

void drawRectangle(GLint bVertex, GLint bColour, GLfloat x, GLfloat y) {
	glUniform3f(tVector, x, y, 0.0f);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, bVertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, bColour);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glUniform3f(tVector, 0.0f, 0.0f, 0.0f);
}

void drawBricks() {
	glUniform3f(tColour, 0.0f, 0.0f, 0.0f);
	for (int i = 0; i < numberOfBricksRows; i++) {
		for (int j = 0; j < numberOfBricksColumns; j++) {
			if (isVisible[i][j])
				drawPolygon(back_brick_vertexbuffer, back_brick_colourbuffer, numberOfBackgroundMiniTriangles, j * bricksShift.x, i * bricksShift.y);
		}
	}

	//front bricks
	for (int i = 0; i < numberOfBricksRows; i++) {
		if (i == 0) glUniform3f(tColour, 0.5f, 1.0f, 0.5f);
		else if (i == 1) glUniform3f(tColour, 1.0f, 0.4f, 0.8f);
		else if (i == 2) glUniform3f(tColour, 0.75f, 0.85f, 0.95f);
		else if (i == 3) glUniform3f(tColour, 1.0f, 1.0f, 0.0f);
		else if (i == 4) glUniform3f(tColour, 1.0f, 0.0f, 0.0f);
		for (int j = 0; j < numberOfBricksColumns; j++) {
			if (isVisible[i][j])
				drawPolygon(back_brick_vertexbuffer, back_brick_colourbuffer, numberOfBackgroundMiniTriangles, j * bricksShift.x + 0.01f, i * bricksShift.y + 0.01f);
		}
	}

	glUniform3f(tColour, 0.0f, 0.0f, 0.0f);
}

void keyboard(){
	static double lastTimeK = glfwGetTime();

	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTimeK);

	// Strafe right
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
		paddlePosition.x += deltaTime * paddleSpeed;
		if (paddlePosition.x > 0.29f) paddlePosition.x = 0.29f;
	}
	// Strafe left
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
		paddlePosition.x -= deltaTime * paddleSpeed;
		if (paddlePosition.x < -0.29f) paddlePosition.x = -0.29f; //307
	}

	lastTimeK = currentTime;
}

void drawBackround() {
	float shift;
	for (int i = 0; i < 30; i++) {
		(i % 2 == 0) ? shift = 0 : shift = 0.12f;
		for (int j = 0; j < 9; j++) {
			drawPolygon(background_vertexbuffer, background_colourbuffer, numberOfBackgroundMiniTriangles, j*0.24f + shift, i * 0.07f);
		}
	}
}

void changeBallPosition() {
	// bottom edge collision
	if (ballPosition.y <= -0.836f)
		endGame = true;

	//top edge collision
	if (ballPosition.y >= 0.836f)
		angle = (-1) * angle + 360;

	paddleCollision();

	brickCollision();

	wallCollision();

	if (angle >= 360)
		angle -= 360;

	if (angle < 0)
		angle += 360;

	ballPosition.x += speed * cos(angle * PI / 180); //convert to radians
	ballPosition.y += speed * sin(angle * PI / 180);
}

void cleanUp() {
	// Cleanup VBO
	glDeleteBuffers(1, &ball_vertexbuffer);
	glDeleteBuffers(1, &ball_colourbuffer);
	glDeleteBuffers(1, &background_vertexbuffer);
	glDeleteBuffers(1, &background_colourbuffer);
	glDeleteBuffers(1, &walls_vertexbuffer);
	glDeleteBuffers(1, &walls_colourbuffer);
	glDeleteBuffers(1, &back_brick_vertexbuffer);
	glDeleteBuffers(1, &back_brick_colourbuffer);
	glDeleteBuffers(1, &paddle_vertexbuffer);
	glDeleteBuffers(1, &paddle_colourbuffer);

	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(programID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

void paddleCollision() {
	if (angle > 180 && angle < 360)
		if (ballPosition.y <= -0.77f && (ballPosition.x - (paddlePosition.x - 0.23f) > 0) && (ballPosition.x - (paddlePosition.x + 0.23f) < 0)) {
			angle = (-1) * angle + 360 + generateRandomNumber(-30, 30);
		}
}

void brickCollision() {
	for (int i = 0; i < numberOfBricksRows; i++) {
		for (int j = 0; j < numberOfBricksColumns; j++) {
			if (isVisible[i][j]) {
				//distance between two points
				brickPosition.x = firstBrickPos.x + j * bricksShift.x;
				brickPosition.y = firstBrickPos.y + i * bricksShift.y;
				double distance = glm::sqrt(pow(brickPosition.x - ballPosition.x, 2) + pow(brickPosition.y - ballPosition.y, 2));
				if (distance <= bothRadius) {
					if (ballPosition.x - (brickPosition.x - brickRadius / 2 - ballRadius / 2) >= 0 && ballPosition.x - (brickPosition.x + brickRadius / 2 + ballRadius / 2) <= 0) {
						if (distance <= (brickHeight + ballRadius)) {
							angle = (-1) * angle + 360;
							isVisible[i][j] = false;
						}
					}
					else if (ballPosition.x - (brickPosition.x - brickRadius / 2 - ballRadius / 2) <= 0) {
							isVisible[i][j] = false;
							if (ballPosition.y <= brickPosition.y) {
								angle = (-1) * angle + 240;
							}
							else {
								angle = (-1) * angle + 480;
							}
					}
					else {
						isVisible[i][j] = false;
						if (ballPosition.y <= brickPosition.y) {
							angle = (-1) * angle + 120;
						}
						else {
							angle = (-1) * angle + 240;
						}
					}
				}
			}
		}
	}
}

void wallCollision() {
	double a1, a2, a3, a4;
	double c1, c2, c3, c4;
	a1 = 1.732;
	c1 = 1.732;
	a2 = -a1;
	c2 = -c1;
	a3 = a1;
	c3 = -c1;
	a4 = a2;
	c4 = -c2;
	double distance;
	distance = abs(a1 * ballPosition.x + ballPosition.y + c1)/glm::sqrt(pow(a1,2) + 1);
	if (distance < ballRadius) {
		angle = (-1) * angle + 240;
		if (abs(angle - 30) <= 1)
			angle += generateRandomNumber(-5, 5);
	}

	distance = abs(a2 * ballPosition.x + ballPosition.y + c2) / glm::sqrt(pow(a2, 2) + 1);
	if (distance - 0.005f <= ballRadius) {
		angle = (-1) * angle + 120;
		if (abs(angle - 150) <= 1)
			angle += generateRandomNumber(-5, 5);
	}

	distance = abs(a3 * ballPosition.x + ballPosition.y + c3) / glm::sqrt(pow(a3, 2) + 1);
	if (distance < ballRadius) {
		angle = (-1) * angle + 240;
	}

	distance = abs(a4 * ballPosition.x + ballPosition.y + c4) / glm::sqrt(pow(a4, 2) + 1);
	if (distance - 0.005f <= ballRadius) {
		angle = (-1) * angle + 480;
	}
}

void checkAnswer() {
	for (int i = 0; i < numberOfBricksRows; i++)
		for (int j = 0; j < numberOfBricksColumns; j++)
			if (isVisible[i][j] == true)
				return;
	endGame = true;
}