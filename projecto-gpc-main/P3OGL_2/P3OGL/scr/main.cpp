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

// Variables movimiento ratón
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 250.0f;
float lastY = 250.0f;
bool firstMouse = true;

// Matrices
glm::mat4 proj = glm::mat4(1.0f);
glm::mat4 view = glm::mat4(1.0f);
glm::mat4 model = glm::mat4(1.0f);

// OpenGL variables
unsigned int vshader, fshader;
unsigned int vao, attrVBO, indexVBO;
unsigned int colorTexId, emiTexId;

unsigned int vao2, attrVBO2, indexVBO2;

// Cámara
glm::vec3 COP = glm::vec3(0.0f, 0.0f, 6.0f);
glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

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

// --- Prototipos ---
void renderFunc();
void resizeWin(int width, int height);
void idleFunc();
void shooterMov(unsigned char key, int x, int y);
void mouseMov(int x, int y);
void initContext(int argc, char** argv);
void initOGL();
void initShader(const char* vname, const char* fname, ShaderProgram& program);
void initObj();
void destroy();
GLuint loadShader(const char* fileName, GLenum type);
unsigned int loadTex(const char* fileName);

// --- Main ---
int main(int argc, char** argv)
{
	std::locale::global(std::locale("spanish"));
	proj = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 50.0f);

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
	glutInitWindowSize(500, 500);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("Prácticas OGL");

	if (glewInit() != GLEW_OK) {
		std::cerr << "Error al iniciar GLEW" << std::endl;
		exit(-1);
	}

	glutReshapeFunc(resizeWin);
	glutDisplayFunc(renderFunc);
	glutIdleFunc(idleFunc);
	glutKeyboardFunc(shooterMov);
	glutPassiveMotionFunc(mouseMov);
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
	glDetachShader(program1.program, vshader);
	glDetachShader(program2.program, vshader);
	glDetachShader(program1.program, fshader);
	glDetachShader(program2.program, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	glDeleteProgram(program1.program);
	glDeleteProgram(program2.program);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &attrVBO);
	glDeleteBuffers(1, &indexVBO);
}

void initShader(const char* vname, const char* fname, ShaderProgram& program)
{
	vshader = loadShader(vname, GL_VERTEX_SHADER);
	fshader = loadShader(fname, GL_FRAGMENT_SHADER);

	program.program = glCreateProgram();
	glAttachShader(program.program, vshader);
	glAttachShader(program.program, fshader);
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
	auto objects = loadMesh("models/box.obj");
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
			vo->textCoord[i].x, vo->textCoord[i].y
			});
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &attrVBO);
	glBindBuffer(GL_ARRAY_BUFFER, attrVBO);
	glBufferData(GL_ARRAY_BUFFER, attr.size() * sizeof(float), attr.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &indexVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vo->idx.size() * sizeof(unsigned int), vo->idx.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 3)); // Color
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 6)); // Normal
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 9)); // Textura
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	auto objects2 = loadMesh("models/sphere.obj");
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
			vo2->textCoord[i].x, vo2->textCoord[i].y
			});
	}

	glGenVertexArrays(1, &vao2);
	glBindVertexArray(vao2);

	glGenBuffers(1, &attrVBO2);
	glBindBuffer(GL_ARRAY_BUFFER, attrVBO2);
	glBufferData(GL_ARRAY_BUFFER, attr2.size() * sizeof(float), attr2.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &indexVBO2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vo2->idx.size() * sizeof(unsigned int), vo2->idx.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 6));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 11, (void*)(sizeof(float) * 9));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	colorTexId = loadTex("../img/color2.png");
	emiTexId = loadTex("../img/emissive.png");
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

	GLuint uDirLightDir = glGetUniformLocation(program1.program, "dirLightDir");
	glUniform3f(uDirLightDir, 1.0f, -1.0f, 1.0f);  // dirección desde donde viene la luz

	GLuint uDirLightId = glGetUniformLocation(program1.program, "dirLightId");
	glUniform3f(uDirLightId, 1.0f, 1.0f, 1.0f);  // color/intensidad difusa

	GLuint uDirLightIs = glGetUniformLocation(program1.program, "dirLightIs");
	glUniform3f(uDirLightIs, 1.0f, 1.0f, 1.0f);  // color/intensidad especular


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program1.program);

	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, colorTexId);
	glUniform1i(program1.uColorTex, 3);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, emiTexId);
	glUniform1i(program1.uEmiTex, 1);

	glBindVertexArray(vao);

	glUseProgram(program2.program);

	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, colorTexId);
	glUniform1i(program2.uColorTex, 3);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, emiTexId);
	glUniform1i(program2.uEmiTex, 1);

	glBindVertexArray(vao);

	glm::mat4 viewMat = glm::lookAt(COP, COP + lookAt, up);

	static int lastTime = 0;
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (currentTime - lastTime) / 1000.0f;
	static float angle = 0.0f;
	angle += deltaTime * 1.0f;
	if (angle > 2 * 3.141592f)
		angle -= 2 * 3.141592f;
	lastTime = currentTime;

	int uCubeId = glGetUniformLocation(program1.program, "cubeId");


	for (int i = -1; i <= 2; ++i) {
		glm::mat4 localModel = glm::mat4(1.0f);
		int cubeId = i + 1;

		// pendiente -> 

		if (i == -1) {
			float splineValue = 1 + sin(angle);
			float y = 0;

			localModel = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f + splineValue, y, 0.0f));


			if (splineValue <= 1.0f) {
				y = 1.5f * splineValue - 0.5f * pow(splineValue, 3);
			}
			else {
				float t = splineValue - 1.0f;
				y = 1 - 1.5f * pow(t, 2) + 0.5f * pow(t, 3);
			}
			localModel = glm::translate(localModel, glm::vec3(splineValue - 5.f, y, 0.0f));
		}
		else if (i == 0) {
			localModel = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, 0.0f));
			localModel = glm::rotate(localModel, angle, glm::vec3(1.0f, 1.0f, 0.0f));
		}
		else if (i == 1) {
			float radius = 4.0f;
			float orbitSpeed = 1.0f;  // puedes ajustar esto

			float x = radius * cos(angle * orbitSpeed);
			float z = radius * sin(angle * orbitSpeed);
			float y = 3.5f;  // Altura de la esfera central

			// Traslada respecto al centro de la esfera
			localModel = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
		}

		else if (i == 2) {
			localModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.5f, 0.0f));


			glUseProgram(program1.program);
			glUniform3f(glGetUniformLocation(program1.program, "Ka"), 0.1f, 1.0f, 0.0f); // rojo ambiental
			glUniform3f(glGetUniformLocation(program1.program, "Kd"), 0.0f, 0.0f, 1.0f); // rojo difuso intenso
			glUniform3f(glGetUniformLocation(program1.program, "Ks"), 1.0f, 0.8f, 0.3f); // reflejo especular gris
			glUniform1f(glGetUniformLocation(program1.program, "alpha"), 32.0f);         // brillo especular


			glm::mat4 modelView = viewMat * localModel;
			glm::mat4 modelViewProj = proj * modelView;
			glm::mat4 normal = glm::transpose(glm::inverse(modelView));

			glBindVertexArray(vao2);
			glUniformMatrix4fv(program1.uModelViewMat, 1, GL_FALSE, &modelView[0][0]);
			glUniformMatrix4fv(program1.uModelViewProjMat, 1, GL_FALSE, &modelViewProj[0][0]);
			glUniformMatrix4fv(program1.uNormal, 1, GL_FALSE, &normal[0][0]);
			glUniformMatrix4fv(program1.uCameraPos, 1, GL_FALSE, &COP[0]);
			glUniformMatrix4fv(program1.uModel, 1, GL_FALSE, &localModel[0][0]);

			glDrawElements(GL_TRIANGLES, vo2->idx.size(), GL_UNSIGNED_INT, 0);
			continue;
		}



		if (cubeId == 3) {
			glUseProgram(program2.program);
			float currentTimeSeconds = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
			glUniform1f(program2.uTime, currentTimeSeconds);
		}
		else {
			glUseProgram(program1.program);

			// FUENTE DE LUZ DIRECCIONAL CON TRAYECTORIA CÍCLICA
			// pendiente -> La linterna de FPS es muy intensa de cerca
			// cambiar la velocidad de movimiento (que sea "suave"?)
			
			float angle = glutGet(GLUT_ELAPSED_TIME) / 428.0f;
			glm::vec3 dirLightDir = glm::normalize(glm::vec3(sin(angle), -1.0f, cos(angle)));
			glm::vec3 dirLightId = glm::vec3(1.0f, 0.7f, 0.4f);
			glm::vec3 dirLightIs = glm::vec3(1.0f, 0.6f, 0.2f);

			glUseProgram(program1.program);
			glUniform3fv(glGetUniformLocation(program1.program, "dirLightDir"), 1, glm::value_ptr(dirLightDir));
			glUniform3fv(glGetUniformLocation(program1.program, "dirLightId"), 1, glm::value_ptr(dirLightId));
			glUniform3fv(glGetUniformLocation(program1.program, "dirLightIs"), 1, glm::value_ptr(dirLightIs));
		}

		int uCubeId = glGetUniformLocation((cubeId == 3 ? program2.program : program1.program), "cubeId");
		glUniform1i(uCubeId, cubeId);

		glm::mat4 modelView = viewMat * localModel;
		glm::mat4 modelViewProj = proj * modelView;
		glm::mat4 normal = glm::transpose(glm::inverse(modelView));

		const ShaderProgram& activeProg = (cubeId == 3 ? program2 : program1);

		glUniformMatrix4fv(activeProg.uModelViewMat, 1, GL_FALSE, &modelView[0][0]);
		glUniformMatrix4fv(activeProg.uModelViewProjMat, 1, GL_FALSE, &modelViewProj[0][0]);
		glUniformMatrix4fv(activeProg.uNormal, 1, GL_FALSE, &normal[0][0]);
		glUniformMatrix4fv(activeProg.uCameraPos, 1, GL_FALSE, &COP[0]);
		glUniformMatrix4fv(activeProg.uModel, 1, GL_FALSE, &localModel[0][0]);


		glDrawElements(GL_TRIANGLES, vo->idx.size(), GL_UNSIGNED_INT, 0);
	}

	glutSwapBuffers();
}



void shooterMov(unsigned char key, int x, int y)
{
	const float moveSpeed = 0.5f;
	const float rotateAngle = glm::radians(5.0f);
	glm::vec3 right = glm::normalize(glm::cross(lookAt, up));

	switch (key) {
	case 'a': COP -= right * moveSpeed; break;
	case 'w': COP += lookAt * moveSpeed; break;
	case 's': COP -= lookAt * moveSpeed; break;
	case 'd': COP += right * moveSpeed; break;
	case 'q': lookAt = glm::mat3(glm::rotate(glm::mat4(1.0f), rotateAngle, up)) * lookAt; break;
	case 'e': lookAt = glm::mat3(glm::rotate(glm::mat4(1.0f), -rotateAngle, up)) * lookAt; break;
	}

	lookAt = glm::normalize(lookAt);
	yaw = glm::degrees(atan2(lookAt.z, lookAt.x));
	pitch = glm::degrees(asin(lookAt.y));

	glutPostRedisplay();
}

void mouseMov(int x, int y) //EXTRA
{
	if (firstMouse) {
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	float xoffset = x - lastX;
	float yoffset = lastY - y;
	lastX = x;
	lastY = y;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 direction;
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	lookAt = glm::normalize(direction);

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
