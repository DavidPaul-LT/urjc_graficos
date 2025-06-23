#include "auxiliar.h"

#include <gl/glew.h>
#define SOLVE_FGLUT_WARNING
#include <gl/freeglut.h> 

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include "MeshLoader.hpp"

#include <glm/gtc/type_ptr.hpp>

#ifndef PI
#define PI 3.14159265f
#endif

// Variables de movimiento del ratón
float yaw = -90.0f;
float pitch = 0.0f;
float mouseLastX = 250.0f;
float mouseLastY = 250.0f;
bool mouseJustPressed = true;

// Matrices
glm::mat4 proj = glm::mat4(1.0f);
glm::mat4 view = glm::mat4(1.0f);
glm::mat4 model = glm::mat4(1.0f);

// Variables de OpenGL
unsigned int vertexShaderId, fragmentShaderId;
unsigned int vaoId, vertexBufferId, indexBufferId;
unsigned int vaoId2, vertexBufferId2, indexBufferId2;
unsigned int diffuseTextureId, emissiveTextureId;

// Cámara
glm::vec3 COP = glm::vec3(0.0f, 0.0f, 15.0f);
glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

// Vectores de interpolación para un movimiento más suave de la cámara
glm::vec3 targetCOP = COP;
glm::vec3 targetLookAt = lookAt;

VirtualObject* vo = nullptr;
VirtualObject* vo2 = nullptr;


struct ShaderProgram {
	GLuint program;
	int uModel;
	int uCameraPos;
	int uModelViewMat;
	int uModelViewProjMat;
	int uNormal;
	int uColorTex;
	int uEmiTex;
	int uTime;
};

ShaderProgram program1, program2;


// --- Encabezados ---
void renderFunc();
void resizeWin(int width, int height);
void idleFunc();
void keyBoardMovement(unsigned char key, int x, int y);
void mouseMovement(int x, int y);
void initContext(int argc, char** argv);
void initOGL();
void initShader(const char* vname, const char* fname, ShaderProgram& program);
void initObj();
void destroy();
GLuint loadShader(const char* fileName, GLenum type);
unsigned int loadTex(const char* fileName);


int main(int argc, char** argv)
{
	std::locale::global(std::locale("spanish"));
	proj = glm::perspective(glm::radians(100.0f), 1.0f, 0.1f, 5.0f);

	initContext(argc, argv);
	initOGL();
	initShader("../shaders_P3/shader.v1.vert", "../shaders_P3/shader.v1.frag", program1);
	initShader("../shaders_P3/shader.v0.vert", "../shaders_P3/shader.v0.frag", program2);
	initObj();

	glutMainLoop();
	destroy();
	return 0;
}

void initContext(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitWindowSize(650, 400);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("Práctica Gráficos");

	if (glewInit() != GLEW_OK) {
		std::cerr << "Error al iniciar GLEW" << std::endl;
		exit(-1);
	}

	glutReshapeFunc(resizeWin);
	glutDisplayFunc(renderFunc);
	glutIdleFunc(idleFunc);
	glutKeyboardFunc(keyBoardMovement);
	glutPassiveMotionFunc(mouseMovement);
}

void initOGL()
{
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glFrontFace(GL_CCW);
}

void destroy()
{
	glDetachShader(program1.program, vertexShaderId);
	glDetachShader(program2.program, vertexShaderId);
	glDetachShader(program1.program, fragmentShaderId);
	glDetachShader(program2.program, fragmentShaderId);
	glDeleteShader(vertexShaderId);
	glDeleteShader(fragmentShaderId);
	glDeleteProgram(program1.program);
	glDeleteProgram(program2.program);
	glDeleteVertexArrays(1, &vaoId);
	glDeleteBuffers(1, &vertexBufferId);
	glDeleteBuffers(1, &indexBufferId);
}

void initShader(const char* vname, const char* fname, ShaderProgram& program)
{
	vertexShaderId = loadShader(vname, GL_VERTEX_SHADER);
	fragmentShaderId = loadShader(fname, GL_FRAGMENT_SHADER);

	program.program = glCreateProgram();
	glAttachShader(program.program, vertexShaderId);
	glAttachShader(program.program, fragmentShaderId);
	glLinkProgram(program.program);


	int linked;
	glGetProgramiv(program.program, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint logLen;
		glGetProgramiv(program.program, GL_INFO_LOG_LENGTH, &logLen);
		char* logString = new char[logLen];
		glGetProgramInfoLog(program.program, logLen, NULL, logString);
		std::cerr << "Link error: " << logString << std::endl;
		delete[] logString;
		exit(-1);
	}

	program.uModelViewMat = glGetUniformLocation(program.program, "modelView");
	program.uModelViewProjMat = glGetUniformLocation(program.program, "modelViewProj");
	program.uNormal = glGetUniformLocation(program.program, "normal");
	program.uColorTex = glGetUniformLocation(program.program, "colorTex");
	program.uEmiTex = glGetUniformLocation(program.program, "emiTex");
	program.uTime = glGetUniformLocation(program.program, "uTime");
	program.uCameraPos = glGetUniformLocation(program.program, "cameraPos");
	program.uModel = glGetUniformLocation(program.program, "model");
}

void initObj()
{
	auto objects = loadModelMeshes("models/box.obj");
	if (objects.empty()) {
		std::cerr << "Error cargando modelo." << std::endl;
		exit(-1);
	}
	vo = objects[0];


	std::vector<float> attr;
	for (size_t i = 0; i < vo->pos.size(); ++i) {
		attr.insert(attr.end(), {
			vo->pos[i].x, vo->pos[i].y, vo->pos[i].z,
			vo->color[i].r, vo->color[i].g, vo->color[i].b,
			vo->normal[i].x, vo->normal[i].y, vo->normal[i].z,
			vo->texCoords[i].x, vo->texCoords[i].y
			});
	}

	glGenVertexArrays(1, &vaoId);
	glBindVertexArray(vaoId);

	glGenBuffers(1, &vertexBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
	glBufferData(GL_ARRAY_BUFFER, attr.size() * sizeof(float), attr.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &indexBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vo->index.size() * sizeof(unsigned int), vo->index.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 3)); // Color
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 6)); // Normal
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 9)); // Textura
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	auto objects2 = loadModelMeshes("models/sphere.obj");
	if (objects2.empty()) {
		std::cerr << "Error cargando sphere.obj" << std::endl;
		exit(-1);
	}
	vo2 = objects2[0];

	std::vector<float> attr2;
	for (size_t i = 0; i < vo2->pos.size(); ++i) {
		attr2.insert(attr2.end(), {
			vo2->pos[i].x, vo2->pos[i].y, vo2->pos[i].z,
			vo2->color[i].r, vo2->color[i].g, vo2->color[i].b,
			vo2->normal[i].x, vo2->normal[i].y, vo2->normal[i].z,
			vo2->texCoords[i].x, vo2->texCoords[i].y
			});
	}

	glGenVertexArrays(1, &vaoId2);
	glBindVertexArray(vaoId2);

	glGenBuffers(1, &vertexBufferId2);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId2);
	glBufferData(GL_ARRAY_BUFFER, attr2.size() * sizeof(float), attr2.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &indexBufferId2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vo2->index.size() * sizeof(unsigned int), vo2->index.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 6));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 9));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	diffuseTextureId = loadTex("../img/color2.png");
	emissiveTextureId = loadTex("../img/emissive.png");
}

GLuint loadShader(const char* fileName, GLenum type)
{
	unsigned int fileLen;
	char* source = loadStringFromFile(fileName, fileLen);

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, (const GLint*)&fileLen);
	glCompileShader(shader);
	delete[] source;

	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint logLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
		char* logString = new char[logLen];
		glGetShaderInfoLog(shader, logLen, NULL, logString);
		std::cerr << fileName << "\nError: " << logString << std::endl;
		delete[] logString;
		glDeleteShader(shader);
		exit(-1);
	}
	return shader;
}

unsigned int loadTex(const char* fileName)
{
	unsigned char* map;
	unsigned int w, h;

	map = loadTexture(fileName, w, h);
	if (!map) {
		std::cerr << "Error cargando textura: " << fileName << std::endl;
		exit(-1);
	}

	unsigned int texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, map);
	glGenerateMipmap(GL_TEXTURE_2D);
	delete[] map;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return texId;
}

void renderFunc()
{

	// Dirección de la que viene la luz
	GLuint uDirLightDir = glGetUniformLocation(program1.program, "dirLightDir");
	glUniform3f(uDirLightDir, 1.0f, -1.0f, 1.0f);  // dirección desde donde viene la luz

	// Color e intensidad difusos
	GLuint uDirLightId = glGetUniformLocation(program1.program, "dirLightId");
	glUniform3f(uDirLightId, 1.0f, 1.0f, 1.0f);

	// Color e intensidad especulares
	GLuint uDirLightIs = glGetUniformLocation(program1.program, "dirLightIs");
	glUniform3f(uDirLightIs, 1.0f, 1.0f, 1.0f);


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program1.program);

	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, diffuseTextureId);
	glUniform1i(program1.uColorTex, 3);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, emissiveTextureId);
	glUniform1i(program1.uEmiTex, 1);

	glBindVertexArray(vaoId);

	glUseProgram(program2.program);

	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, diffuseTextureId);
	glUniform1i(program2.uColorTex, 3);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, emissiveTextureId);
	glUniform1i(program2.uEmiTex, 1);

	glBindVertexArray(vaoId);

	float smoothing = 0.1f;
	COP = glm::mix(COP, targetCOP, smoothing);
	lookAt = glm::mix(lookAt, targetLookAt, smoothing);
	glm::mat4 viewMat = glm::lookAt(COP, COP + lookAt, up);

	static int lastTime = 0;
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (currentTime - lastTime) / 1000.0f;
	static float angle = 0.0f;
	angle += deltaTime * 1.0f;
	if (angle > 2 * PI)
		angle -= 2 * PI;
	lastTime = currentTime;

	int uCubeId = glGetUniformLocation(program1.program, "objectId");


	for (int i = -1; i <= 2; ++i) {
		glm::mat4 modelMatrixLocal = glm::mat4(1.0f);
		int objectId = i + 1;

		// órbita con movimiento senoidal
		if (i == -1) {
			// propiedades de la órbita
			glm::vec3 center(3.0f, -3.0f, 3.0f); 

			float radius = 1.0f;
			float freq = 3.0f;
			float verticalAmplitude = 1.0f;

			// Movimiento en órbita circular (plano XZ)
			float x = (radius) * cos(angle * freq);
			float z = (radius) * sin(angle * freq);

			// Movimiento senoidal en eje Y
			float y = verticalAmplitude * sin(angle * freq); // doble frecuencia en vertical

			glm::vec3 offset(x, y, z);
			modelMatrixLocal = glm::translate(glm::mat4(1.0f), center + offset);
		}
		// cubo rotativo que describe una elipse
		else if (i == 0) {

			// se considera esto para evitar que de un salto repentino en la traslación
			float rawTime = glutGet(GLUT_ELAPSED_TIME) / 560.0f;
			float angle = fmodf(rawTime, 2.0f * PI);

			float radiusX = 5.0f;
			float radiusZ = 3.0f;
			float radiusY = 10.0f;

			float x = radiusX * cos(angle);
			float z = radiusZ * sin(angle);
			float y = radiusY * cos(angle);
			
			modelMatrixLocal = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
			modelMatrixLocal = glm::rotate(modelMatrixLocal, rawTime * 3.0f, glm::vec3(1.0f, 1.0f, 0.0f));
		}
		// Órbita plana en el eje XZ
		else if (i == 1) {
			float orbitRadius = 4.0f;
			float orbitSpeed = 1.0f; 

			float x = orbitRadius * cos(angle * orbitSpeed);
			float z = orbitRadius * sin(angle * orbitSpeed);
			float y = 3.5f;  // Altura de la esfera central

			modelMatrixLocal = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
		}
		// Esfera estática
		else if (i == 2) {
			modelMatrixLocal = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.5f, 0.0f));


			glUseProgram(program1.program);
			glUniform3f(glGetUniformLocation(program1.program, "Ka"), 0.1f, 1.0f, 0.0f); 
			glUniform3f(glGetUniformLocation(program1.program, "Kd"), 0.0f, 0.0f, 1.0f); 
			glUniform3f(glGetUniformLocation(program1.program, "Ks"), 1.0f, 0.8f, 0.3f); 
			glUniform1f(glGetUniformLocation(program1.program, "alpha"), 32.0f);         


			glm::mat4 modelView = viewMat * modelMatrixLocal;
			glm::mat4 modelViewProj = proj * modelView;
			glm::mat4 normal = glm::transpose(glm::inverse(modelView));

			glBindVertexArray(vaoId2);
			glUniformMatrix4fv(program1.uModelViewMat, 1, GL_FALSE, &modelView[0][0]);
			glUniformMatrix4fv(program1.uModelViewProjMat, 1, GL_FALSE, &modelViewProj[0][0]);
			glUniformMatrix4fv(program1.uNormal, 1, GL_FALSE, &normal[0][0]);
			glUniformMatrix4fv(program1.uCameraPos, 1, GL_FALSE, &COP[0]);
			glUniformMatrix4fv(program1.uModel, 1, GL_FALSE, &modelMatrixLocal[0][0]);

			glDrawElements(GL_TRIANGLES, vo2->index.size(), GL_UNSIGNED_INT, 0);
			continue;
		}



		if (objectId == 3) {
			glUseProgram(program2.program);
			float currentTimeSeconds = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
			glUniform1f(program2.uTime, currentTimeSeconds);
		}
		else {
			glUseProgram(program1.program);
			float angle = glutGet(GLUT_ELAPSED_TIME) / 428.0f;
			glm::vec3 dirLightDir = glm::normalize(glm::vec3(sin(angle), -1.0f, cos(angle)));
			glm::vec3 dirLightId = glm::vec3(1.0f, 0.7f, 0.4f);
			glm::vec3 dirLightIs = glm::vec3(1.0f, 0.6f, 0.2f);

			glUseProgram(program1.program);
			glUniform3fv(glGetUniformLocation(program1.program, "dirLightDir"), 1, glm::value_ptr(dirLightDir));
			glUniform3fv(glGetUniformLocation(program1.program, "dirLightId"), 1, glm::value_ptr(dirLightId));
			glUniform3fv(glGetUniformLocation(program1.program, "dirLightIs"), 1, glm::value_ptr(dirLightIs));
		}

		int uCubeId = glGetUniformLocation((objectId == 3 ? program2.program : program1.program), "objectId");
		glUniform1i(uCubeId, objectId);

		glm::mat4 modelView = viewMat * modelMatrixLocal;
		glm::mat4 modelViewProj = proj * modelView;
		glm::mat4 normal = glm::transpose(glm::inverse(modelView));

		const ShaderProgram& activeProg = (objectId == 3 ? program2 : program1);

		glUniformMatrix4fv(activeProg.uModelViewMat, 1, GL_FALSE, &modelView[0][0]);
		glUniformMatrix4fv(activeProg.uModelViewProjMat, 1, GL_FALSE, &modelViewProj[0][0]);
		glUniformMatrix4fv(activeProg.uNormal, 1, GL_FALSE, &normal[0][0]);
		glUniformMatrix4fv(activeProg.uCameraPos, 1, GL_FALSE, &COP[0]);
		glUniformMatrix4fv(activeProg.uModel, 1, GL_FALSE, &modelMatrixLocal[0][0]);


		glDrawElements(GL_TRIANGLES, vo->index.size(), GL_UNSIGNED_INT, 0);
	}

	glutSwapBuffers();
}



void keyBoardMovement(unsigned char key, int x, int y)
{
	const float moveSpeed = 1.0f;
	const float rotateAngle = glm::radians(10.0f);
	glm::vec3 right = glm::normalize(glm::cross(lookAt, up));

	switch (key) {
	case 'a': targetCOP -= right * moveSpeed; break;
	case 'w': targetCOP += lookAt * moveSpeed; break;
	case 's': targetCOP -= lookAt * moveSpeed; break;
	case 'd': targetCOP += right * moveSpeed; break;
	case 'q': targetLookAt = glm::mat3(glm::rotate(glm::mat4(1.0f), rotateAngle, up)) * targetLookAt; break;
	case 'e': targetLookAt = glm::mat3(glm::rotate(glm::mat4(1.0f), -rotateAngle, up)) * targetLookAt; break;
	}

	lookAt = glm::normalize(lookAt);
	yaw = glm::degrees(atan2(lookAt.z, lookAt.x));
	pitch = glm::degrees(asin(lookAt.y));

	glutPostRedisplay();
}

void mouseMovement(int x, int y)
{
	if (mouseJustPressed) {
		mouseLastX = x;
		mouseLastY = y;
		mouseJustPressed = false;
	}

	float xoffset = x - mouseLastX;
	float yoffset = mouseLastY - y;
	mouseLastX = x;
	mouseLastY = y;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	targetLookAt.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	targetLookAt.y = sin(glm::radians(pitch));
	targetLookAt.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	targetLookAt = glm::normalize(targetLookAt);

	glutPostRedisplay();
}

void resizeWin(int width, int height) {
	glViewport(0, 0, width, height);
	float aspect = (float)width / (float)height;
	proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 50.0f);
}


void idleFunc()
{
	glutPostRedisplay();
}
