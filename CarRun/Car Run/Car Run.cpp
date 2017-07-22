// Include standard headers
#include <stdlib.h>
#include <iostream>
#include <vector>

// Include GLEW
#include "GL\glew.h"
// Include GLFW
#include "GLFW\glfw3.h"
GLFWwindow* window;

// Include GLM
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/norm.hpp"
using namespace glm;
//Include bullet physics
#include "PLANE.h"

#include "SHADOW_MODEL.h"
// Include AntTweakBarw
//#include <AntTweakBar.h>

#include "common\shader.hpp"
#include "common\texture.hpp"
#include "common\ghost.h"
#include "common\controls.hpp"
#include "common\objloader.hpp"
#include "common\vboindexer.hpp"
#include "common\text2D.hpp"
#include "common\quaternion_utils.hpp"

SHADOW_MODEL models[3];
int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1366, 766, "RUNNING CAR", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// We would expect width and height to be 1024 and 768
	int windowWidth = 1366;
	int windowHeight = 766;
	// But on MacOS X with a retina screen it'll be 1024*2 and 768*2, so we get the actual framebuffer size:
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited mouvement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, 1366 / 2, 766 / 2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	//depth
	glClearDepth(1.0f);
	glClearStencil(0);
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);//去掉背面的显示

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	/*========================================用于显示结果的着色器的定义=====================================*/
	GLuint quad_programID = LoadShaders("Passthrough.vertexshader", "SimpleTexture.fragmentshader");
	GLuint coltexID = glGetUniformLocation(quad_programID, "simpletexture");
	GLuint inftexID = glGetUniformLocation(quad_programID, "infortexture");
	GLuint britexID = glGetUniformLocation(quad_programID, "brightexture");
	GLuint SpeedinQuadID = glGetUniformLocation(quad_programID, "speed");

	/*========================================用于阴影映射的着色器的定义=====================================*/
	GLuint depthProgramID = LoadShaders("DepthRTT.vertexshader", "DepthRTT.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint depthMatrixID = glGetUniformLocation(depthProgramID, "depthMVP");

	/*========================================用于高斯水平模糊的着色器的定义=====================================*/
	GLuint HGBProgramID = LoadShaders("Passthrough.vertexshader", "HorizGB.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint srcGBtexID = glGetUniformLocation(HGBProgramID, "srcGBtexture");
	GLuint blurBiasID = glGetUniformLocation(HGBProgramID, "horizBias");
	GLuint blurDirecID = glGetUniformLocation(HGBProgramID, "blurDire");
	/*========================================用于绘制场景的着色器的定义=====================================*/
	GLuint programID = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");
	// Get a handle for our "MVP" uniform
	GLuint MVPMatrixID = glGetUniformLocation(programID, "MVP");
	GLuint DepthBiasMVPID = glGetUniformLocation(programID, "DepthBiasMVP");
	GLuint DepthMVPID = glGetUniformLocation(programID, "DepthMVP");
	GLuint DepthMVID = glGetUniformLocation(programID, "DepthMV");
	GLuint ZNearID = glGetUniformLocation(programID, "ZNear");
	GLuint ZFarID = glGetUniformLocation(programID, "ZFar");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	GLuint isShaderID = glGetUniformLocation(programID, "isShader");
	GLuint haveTorchID = glGetUniformLocation(programID, "haveTorch");
	GLuint classNrID = glGetUniformLocation(programID, "classNr");
	GLuint timeID = glGetUniformLocation(programID, "time");
	// Get a handle for our buffers
	GLuint vertexPosition_modelspaceID = glGetAttribLocation(programID, "vertexPosition_modelspace");
	GLuint vertexUVID = glGetAttribLocation(programID, "vertexUV");
	GLuint vertexNormal_modelspaceID = glGetAttribLocation(programID, "vertexNormal_modelspace");
	GLuint EyePosID = glGetUniformLocation(programID, "eyePosWP");
	// Load the texture
	GLuint TextureEnvi = loadBMP_custom("pure.bmp");
	GLuint Texturecar = loadBMP_custom("purecar.bmp");
	GLuint Texturetyre = loadBMP_custom("tyre.bmp");
	GLuint Texturelightgun = loadBMP_custom("purewhite.bmp");
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	GLuint ShadowMapID = glGetUniformLocation(programID, "shadowMap");
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	GLuint CameraID = glGetUniformLocation(programID, "CameraPos_worldspace");
	GLuint TorchID = glGetUniformLocation(programID, "TorchPosition_worldspace");
	GLuint TorchDirecID = glGetUniformLocation(programID, "TorchDirection_worldspace");
	GLuint SpeedID = glGetUniformLocation(programID, "speed");
	GLuint specDegreeID = glGetUniformLocation(programID, "specDegree");
	GLuint diffDegreeID = glGetUniformLocation(programID, "diffDegree");
	GLuint distScanID = glGetUniformLocation(programID, "distScan");


	glBindFragDataLocation(programID, 0, "color");
	glBindFragDataLocation(programID, 1, "information");
	// Read our hogwards.obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	std::vector<glm::vec3> track_vertices;
	std::vector<glm::vec2> track_uvs;
	std::vector<glm::vec3> track_normals;
	bool res = loadOBJ("test.obj", false, vertices, uvs, normals);
	//bool track = loadOBJ("test.obj", false, track_vertices, track_uvs, track_normals);
	//models[1].GenerateAnything("colorplayground.obj");
	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	std::vector<unsigned short> track_indices;
	std::vector<glm::vec3> indexed_track_vertices;
	std::vector<glm::vec2> indexed_track_uvs;
	std::vector<glm::vec3> indexed_track_normals;
	indexVBO(track_vertices, track_uvs, track_normals, track_indices, indexed_track_vertices, indexed_track_uvs, indexed_track_normals);

	// Read our car.obj file
	std::vector<glm::vec3> vertices_car;
	std::vector<glm::vec2> uvs_car;
	std::vector<glm::vec3> normals_car;
	bool res_car = loadOBJ("car.obj", false, vertices_car, uvs_car, normals_car);
	//	models[0].GenerateAnything("car.obj");
	std::vector<unsigned short> indices_car;
	std::vector<glm::vec3> indexed_vertices_car;
	std::vector<glm::vec2> indexed_uvs_car;
	std::vector<glm::vec3> indexed_normals_car;
	indexVBO(vertices_car, uvs_car, normals_car, indices_car, indexed_vertices_car, indexed_uvs_car, indexed_normals_car);

	std::vector<glm::vec3> vertices_tyre;
	std::vector<glm::vec2> uvs_tyre;
	std::vector<glm::vec3> normals_tyre;
	bool res_tyre = loadOBJ("tyre.obj", false, vertices_tyre, uvs_tyre, normals_tyre);
	//	models[0].GenerateAnything("tyre.obj");
	std::vector<unsigned short> indices_tyre;
	std::vector<glm::vec3> indexed_vertices_tyre;
	std::vector<glm::vec2> indexed_uvs_tyre;
	std::vector<glm::vec3> indexed_normals_tyre;
	indexVBO(vertices_tyre, uvs_tyre, normals_tyre, indices_tyre, indexed_vertices_tyre, indexed_uvs_tyre, indexed_normals_tyre);

	std::vector<glm::vec3> vertices_lightgun;
	std::vector<glm::vec2> uvs_lightgun;
	std::vector<glm::vec3> normals_lightgun;
	bool res_lightgun = loadOBJ("lightgun.obj", true, vertices_lightgun, uvs_lightgun, normals_lightgun);
	//	models[0].GenerateAnything("lightgun.obj");
	std::vector<unsigned short> indices_lightgun;
	std::vector<glm::vec3> indexed_vertices_lightgun;
	std::vector<glm::vec2> indexed_uvs_lightgun;
	std::vector<glm::vec3> indexed_normals_lightgun;
	indexVBO(vertices_lightgun, uvs_lightgun, normals_lightgun, indices_lightgun, indexed_vertices_lightgun, indexed_uvs_lightgun, indexed_normals_lightgun);
	/*------------------------------------------------------------------------------------*/
	glm::vec3 sunPos = glm::vec3(50, 10, 50);
	glm::vec3 lightPos;
	vec3 lightPos_modelSpace;
	initText2D("Holstein.DDS");
	float lastTime = glfwGetTime();
	int nbFrames = 0;
	int nbF4motionblur = 0;
	int motionblurDeep = 20;
	float currentTime = 0.0, spf = 100.0;
	float loopTime = 0.0;
	float subTime = 0.0;
	float startTime = 0.0;
	float endTime = 0.0;
	int hit_times = 0;
	float OldXpos = 0, OldYpos = 0;
	float zFarCmra = 0.0;
	float zNearCmra = 0.0;
	glm::vec3 position = glm::vec3(-22, 1, 0);
	vec3 camera_org;
	glm::mat4 ViewMatrix;
	glm::mat4 ViewMatrix_backup;
	mat4 ModelMatrix_car;
	mat4 ModelMatrixLightGun;
	mat4 MVP_car;
	mat4 MVP_lightgun;

	mat4 * ModelMatrixTyreArray = new mat4[4];
	mat4 * MVPtyreArray = new mat4[4];


	vec3 v_car;
	bool lightOn = false;
	bool useSV = false;
	std::vector<glm::vec3> normal_surface;
	std::vector<vec3> Point_Surface_rposition;
	/*------------------------------------------------------------------------------------*/

	// Load it into a VBO

	GLuint vertexbuffer;
	GLuint uvbuffer;
	GLuint normalbuffer;
	GLuint elementbuffer;
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

	GLuint track_vertexbuffer;
	GLuint track_uvbuffer;
	GLuint track_normalbuffer;
	GLuint track_elementbuffer;
	glGenBuffers(1, &track_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, track_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_track_vertices.size() * sizeof(glm::vec3), &indexed_track_vertices[0], GL_STATIC_DRAW);



	glGenBuffers(1, &track_uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, track_uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_track_uvs.size() * sizeof(glm::vec2), &indexed_track_uvs[0], GL_STATIC_DRAW);


	glGenBuffers(1, &track_normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, track_normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_track_normals.size() * sizeof(glm::vec3), &indexed_track_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well

	glGenBuffers(1, &track_elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, track_elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, track_indices.size() * sizeof(unsigned short), &track_indices[0], GL_STATIC_DRAW);


	//Generate buffers for the car as well/////////////////
	GLuint vertexbuffer_car;
	glGenBuffers(1, &vertexbuffer_car);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_car);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices_car.size() * sizeof(glm::vec3), &indexed_vertices_car[0], GL_DYNAMIC_DRAW);
	GLuint uvbuffer_car;
	glGenBuffers(1, &uvbuffer_car);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_car);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs_car.size() * sizeof(glm::vec2), &indexed_uvs_car[0], GL_DYNAMIC_DRAW);

	GLuint normalbuffer_car;
	glGenBuffers(1, &normalbuffer_car);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_car);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals_car.size() * sizeof(glm::vec3), &indexed_normals_car[0], GL_DYNAMIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer_car;
	glGenBuffers(1, &elementbuffer_car);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_car);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_car.size() * sizeof(unsigned short), &indices_car[0], GL_DYNAMIC_DRAW);

	//Generate buffers for the tyre as well/////////////////
	GLuint vertexbuffer_tyre;
	glGenBuffers(1, &vertexbuffer_tyre);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_tyre);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices_tyre.size() * sizeof(glm::vec3), &indexed_vertices_tyre[0], GL_DYNAMIC_DRAW);



	GLuint uvbuffer_tyre;
	glGenBuffers(1, &uvbuffer_tyre);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_tyre);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs_tyre.size() * sizeof(glm::vec2), &indexed_uvs_tyre[0], GL_DYNAMIC_DRAW);

	GLuint normalbuffer_tyre;
	glGenBuffers(1, &normalbuffer_tyre);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_tyre);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals_tyre.size() * sizeof(glm::vec3), &indexed_normals_tyre[0], GL_DYNAMIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer_tyre;
	glGenBuffers(1, &elementbuffer_tyre);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_tyre);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_tyre.size() * sizeof(unsigned short), &indices_tyre[0], GL_DYNAMIC_DRAW);

	GLuint vertexbuffer_lightgun;
	glGenBuffers(1, &vertexbuffer_lightgun);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_lightgun);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices_lightgun.size() * sizeof(glm::vec3), &indexed_vertices_lightgun[0], GL_DYNAMIC_DRAW);

	GLuint uvbuffer_lightgun;
	glGenBuffers(1, &uvbuffer_lightgun);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_lightgun);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs_lightgun.size() * sizeof(glm::vec2), &indexed_uvs_lightgun[0], GL_DYNAMIC_DRAW);

	GLuint normalbuffer_lightgun;
	glGenBuffers(1, &normalbuffer_lightgun);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_lightgun);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals_lightgun.size() * sizeof(glm::vec3), &indexed_normals_lightgun[0], GL_DYNAMIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer_lightgun;
	glGenBuffers(1, &elementbuffer_lightgun);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_lightgun);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_lightgun.size() * sizeof(unsigned short), &indices_lightgun[0], GL_DYNAMIC_DRAW);
	//shadowV/////////////////////////////////////////////////////////////////////////////////////////////////////
	GLuint shadowVB_car;
	glGenBuffers(1, &shadowVB_car);
	glBindBuffer(GL_ARRAY_BUFFER, shadowVB_car);
	ghost carBody(vec3(17, 10, 2), vertices.size() / 3);
	ghost type(vec3(17, 10, 2), vertices.size() / 3);
	/*=======================================================阴影帧缓冲创建======================================================*/
	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	GLuint FB4Depth = 0;
	glGenFramebuffers(1, &FB4Depth);
	glBindFramebuffer(GL_FRAMEBUFFER, FB4Depth);

	// Depth texture. Slower than a depth buffer, but you can sample it later in your shader
	GLuint depthTexture;
	int depthTextureSize = 1024;
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, depthTextureSize, depthTextureSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);

	// No color output in the bound framebuffer, only depth.
	glDrawBuffer(GL_NONE);

	/******************************************创建结果帧缓存*********************************************************/
	GLuint FramebufferName;
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	int framewidth = 1366;
	int frameheight = 766;

	GLuint rboID = 0;
	glGenRenderbuffers(1, &rboID);
	glBindRenderbuffer(GL_RENDERBUFFER, rboID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framewidth, frameheight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboID);

	GLuint ResTexture;
	glGenTextures(1, &ResTexture);

	glBindTexture(GL_TEXTURE_2D, ResTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framewidth, frameheight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ResTexture, 0);

	GLuint InforTexture;
	glGenTextures(1, &InforTexture);
	glBindTexture(GL_TEXTURE_2D, InforTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framewidth, frameheight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, InforTexture, 0);

	GLuint BrightTexture;
	glGenTextures(1, &BrightTexture);
	glBindTexture(GL_TEXTURE_2D, BrightTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framewidth, frameheight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, BrightTexture, 0);
	/*********************************************结果帧缓存创建完毕**************************************************/



	/******************************************创建水平高斯模糊用的帧缓存*********************************************************/
	GLuint FB4HGB;
	glGenFramebuffers(1, &FB4HGB);
	glBindFramebuffer(GL_FRAMEBUFFER, FB4HGB);

	GLuint HGBTexture;
	glGenTextures(1, &HGBTexture);
	glBindTexture(GL_TEXTURE_2D, HGBTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framewidth, frameheight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, HGBTexture, 0);

	GLuint HVGBTexture;
	glGenTextures(1, &HVGBTexture);
	glBindTexture(GL_TEXTURE_2D, HVGBTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framewidth, frameheight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, HVGBTexture, 0);


	/*********************************************水平高斯模糊用的帧缓存创建完毕**************************************************/

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	static const GLfloat g_quad_vertex_buffer_data[] = {
		-1, -1, 0.0f,
		1, -1, 0.0f,
		-1, 1, 0.0f,
		-1, 1, 0.0f,
		1, -1, 0.0f,
		1, 1, 0.0f,
	};
	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	for (int i = 0; i < vertices.size() / 3; i++)
	{
		vec3 AB = vertices[3 * i + 1] - vertices[3 * i + 0];
		vec3 BC = vertices[3 * i + 2] - vertices[3 * i + 1];
		normal_surface.push_back(normalize(cross(AB, BC)));
		Point_Surface_rposition.push_back(vec3(100.0, 1.5, 1.5));
	}

	do {
		computeMatricesFromInputs_my(normal_surface, vertices, &carBody);
		float scanDistance = getScanDistRT();
		glBindFramebuffer(GL_FRAMEBUFFER, FB4Depth);
		glViewport(0, 0, depthTextureSize, depthTextureSize); // Render on the whole framebuffer, complete from the lower left corner to the upper right

		// We don't use bias in the shader, but instead we draw back faces, 
		// which are already separated from the front faces by a small distance 
		// (if your geometry is made this way)
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT); // Cull back-facing triangles -> draw only front-facing triangles

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(depthProgramID);
		ModelMatrix_car = getModelMatrix_car();
		ModelMatrixLightGun = getModMtrxLightGun();
		ModelMatrixTyreArray[0] = getModMtrxTyre1();
		ModelMatrixTyreArray[1] = getModMtrxTyre2();
		ModelMatrixTyreArray[2] = getModMtrxTyre3();
		ModelMatrixTyreArray[3] = getModMtrxTyre4();
		//glm::vec3 lightInvDir = normalize(sunPos);
		glm::vec3 lightInvDir = normalize(sunPos - carBody.position);
		glm::mat4 depthProjectionMatrix;
		glm::mat4 depthViewMatrix;
		zFarCmra = carBody.size * 100.0f;
		zNearCmra = -carBody.size * 0.1f;
		depthProjectionMatrix = glm::ortho<float>(-0.1, 0.1, -0.1, 0.1, zNearCmra, zFarCmra);
		//
		depthViewMatrix = glm::lookAt(carBody.position + lightInvDir, carBody.position, glm::vec3(0, 1, 0));
		// or, for spot light :
		//glm::vec3 lightPos(5, 20, 20);
		//glm::mat4 depthProjectionMatrix = glm::perspective<float>(45.0f, 1.0f, 2.0f, 50.0f);
		//glm::mat4 depthViewMatrix = glm::lookAt(lightPos, lightPos-lightInvDir, glm::vec3(0,1,0));

		// Compute the MVP matrix from the light's point of view

		glm::mat4 depthMVP_car = depthProjectionMatrix * depthViewMatrix * ModelMatrix_car;
		mat4 * depthMVPtyreArray = new mat4[4];
		depthMVPtyreArray[0] = depthProjectionMatrix * depthViewMatrix * ModelMatrixTyreArray[0];
		depthMVPtyreArray[1] = depthProjectionMatrix * depthViewMatrix * ModelMatrixTyreArray[1];
		depthMVPtyreArray[2] = depthProjectionMatrix * depthViewMatrix * ModelMatrixTyreArray[2];
		depthMVPtyreArray[3] = depthProjectionMatrix * depthViewMatrix * ModelMatrixTyreArray[3];

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(depthMatrixID, 1, GL_FALSE, &depthMVP_car[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_car);
		glVertexAttribPointer(
			0,  // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_car);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices_car.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
			);

		glDisableVertexAttribArray(0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_tyre);
		glVertexAttribPointer(
			0,  // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_tyre);
		for (int i = 0; i < 4; i++)
		{
			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(depthMatrixID, 1, GL_FALSE, &depthMVPtyreArray[i][0][0]);



			// Draw the triangles !
			glDrawElements(
				GL_TRIANGLES,      // mode
				indices_tyre.size(),    // count
				GL_UNSIGNED_SHORT, // type
				(void*)0           // element array buffer offset
				);
		}
		glDisableVertexAttribArray(0);

		glm::mat4 depthMVP_env = depthProjectionMatrix * depthViewMatrix * mat4();
		glCullFace(GL_BACK);
		/*========================================================生成阴影贴图完成============================================*/
		/*========================================================开始在帧缓冲中离屏渲染====================================*/
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		GLenum TBOattaches[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, TBOattaches);
		glViewport(0, 0, framewidth, frameheight);
		startTime = glfwGetTime();
		std::vector<glm::vec3> indexed_vertices_SVcar;
		std::vector<unsigned short> indices_SVcar;
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		//depth
		glClearDepth(1.0f);
		glClearStencil(0);
		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		if (useSV)
			glEnable(GL_STENCIL_TEST);
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

		// Cull triangles which normal is not towards the camera
		glEnable(GL_CULL_FACE);//去掉背面的显示
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Cull back-facing triangles -> draw only front-facing triangles
		// Clear the screen

		// Use our shader
		glUseProgram(programID);
		lightSwitch(&lightOn);
		glUniform1f(distScanID, scanDistance);
		glUniform1f(timeID, currentTime);
		glUniform1f(ZFarID, zFarCmra);

		glUniform1f(ZNearID, zNearCmra);
		glUniform1i(classNrID, 1);
		glUniform1i(haveTorchID, lightOn ? 1 : 0);
		glUniform1f(specDegreeID, 7.0);
		glUniform1f(diffDegreeID, 0.7);
		glUniform3f(LightID, sunPos.x, sunPos.y, sunPos.z);
		glUniform3f(EyePosID, camera_org.x, camera_org.y, camera_org.z);
		glUniform3f(TorchID, carBody.position.x, carBody.position.y, carBody.position.z);
		camera_org = getCameraOrg();
		glUniform3f(CameraID, camera_org.x, camera_org.y, camera_org.z);
		glUniform1f(SpeedID, carBody.speed);

		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = mat4();
		glm::mat4 MVP;
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		//glm::mat4 RotationMatrix_car = glm::mat4();
		glm::vec3 CameraDirection = getViewDirection();
		CameraDirection = normalize(CameraDirection);
		//mat4 ScaleMatrix_car = scale(ModelMatrix, vec3(0.1,0.1,0.1));
		vec3 old_car_pos;
		old_car_pos = carBody.position;
		carBody.position = CameraDirection + camera_org;
		v_car = carBody.position - old_car_pos;
		//float realTimeSpeed = length(v_car);

		MVP_car = ProjectionMatrix * ViewMatrix * ModelMatrix_car;
		MVP_lightgun = ProjectionMatrix * ViewMatrix * ModelMatrixLightGun;
		MVPtyreArray[0] = ProjectionMatrix * ViewMatrix * ModelMatrixTyreArray[0];
		MVPtyreArray[1] = ProjectionMatrix * ViewMatrix * ModelMatrixTyreArray[1];
		MVPtyreArray[2] = ProjectionMatrix * ViewMatrix * ModelMatrixTyreArray[2];
		MVPtyreArray[3] = ProjectionMatrix * ViewMatrix * ModelMatrixTyreArray[3];
		if (useSV)
		{
			/***************************************************************开始绘制暗影下的赛道***************************************************/
			glUniform1i(isShaderID, 1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, TextureEnvi);
			glUniform1i(TextureID, 0);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);

			// 2nd attribute buffer : UVs
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
			glVertexAttribPointer(
				1,                                // attribute
				2,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
				);

			// 3rd attribute buffer : normals
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
			glVertexAttribPointer(
				2,                                // attribute
				3,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
				);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
			glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

			// Draw the triangles !
			glDrawElements(
				GL_TRIANGLES,      // mode
				indices.size(),    // count
				GL_UNSIGNED_SHORT, // type
				(void*)0           // element array buffer offset
				);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			/***************************************************************结束绘制暗影下的赛道***************************************************/

			/***************************************************************开始绘制暗影下的小车***************************************************/


			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texturecar);
			glUniform1i(TextureID, 0);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_car);
			glVertexAttribPointer(
				0,                  // attribute
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_car);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
			// 3rd attribute buffer : normals
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_car);
			glVertexAttribPointer(
				2,                                // attribute
				3,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
				);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_car);
			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVP_car[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix_car[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

			// Draw the triangles !
			glDrawElements(
				GL_TRIANGLES,      // mode
				indices_car.size(),    // count
				GL_UNSIGNED_SHORT, // type
				(void*)0           // element array buffer offset
				);
			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			/***************************************************************结束绘制暗影下的小车***************************************************/

			/*============================================================计算模板缓存开始=======================================================*/
			//lightPos = carBody.position + (cross(carBody.right, carBody.top) * 5.0f + carBody.top) * 1.1f;
			lightPos = sunPos;
			lightPos_modelSpace = lightPos - carBody.position;
			lightPos_modelSpace = normalize(vec3(dot(lightPos_modelSpace, carBody.right),
				dot(lightPos_modelSpace, carBody.top), dot(lightPos_modelSpace, cross(carBody.right, carBody.top))))*length(lightPos_modelSpace);
			models[0].CalculateSilhouetteEdges(lightPos_modelSpace);//里面是物体坐标系下的光源坐标
			models[0].DrawInfiniteShadowVolume(lightPos_modelSpace, true, &indexed_vertices_SVcar, &indices_SVcar);/////////////////      
			glColorMask(0, 0, 0, 0);
			glDepthMask(0);
			glDepthFunc(GL_LESS);
			glStencilFunc(GL_ALWAYS, 1, ~0);
			glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
			glCullFace(GL_FRONT);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, shadowVB_car);
			glBufferData(GL_ARRAY_BUFFER, indexed_vertices_SVcar.size() * sizeof(glm::vec3), &indexed_vertices_SVcar[0], GL_STATIC_DRAW);
			glVertexAttribPointer(
				0,                  // attribute
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);
			glDrawArrays(GL_TRIANGLES, 0, indexed_vertices_SVcar.size());
			glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
			glCullFace(GL_BACK);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, shadowVB_car);
			glBufferData(GL_ARRAY_BUFFER, indexed_vertices_SVcar.size() * sizeof(glm::vec3), &indexed_vertices_SVcar[0], GL_STATIC_DRAW);
			glVertexAttribPointer(
				0,                  // attribute
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);
			glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVP_car[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix_car[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
			glDrawArrays(GL_TRIANGLES, 0, indexed_vertices_SVcar.size());
			glDisableVertexAttribArray(0);

			glDepthMask(1);
			glDepthFunc(GL_LEQUAL);
			glColorMask(1, 1, 1, 1);

			glUniform1i(isShaderID, 0);
			glStencilFunc(GL_EQUAL, 0, ~0);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			/*===============================================计算模板深度值结束   5.0ms==============================================*/

		}

		glm::mat4 biasMatrix(
			0.5, 0.0, 0.0, 0.0,
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0
			);
		glm::mat4 depthBiasMVP = biasMatrix*depthMVP_env;
		glm::mat4 depthMV_env = depthViewMatrix * mat4();
		///////////////////////////////////////////////////////绘制第二次背景
		glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniformMatrix4fv(DepthBiasMVPID, 1, GL_FALSE, &depthBiasMVP[0][0]);
		glUniformMatrix4fv(DepthMVPID, 1, GL_FALSE, &depthMVP_env[0][0]);
		glUniformMatrix4fv(DepthMVID, 1, GL_FALSE, &depthMV_env[0][0]);
		glUniform3f(TorchDirecID, CameraDirection.x, CameraDirection.y, CameraDirection.z);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureEnvi);
		glUniform1i(TextureID, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glUniform1i(ShadowMapID, 1);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
			);

		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, TextureEnvi);
		//glUniform1i(TextureID, 0);

		//glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_2D, depthTexture);
		//glUniform1i(ShadowMapID, 1);

		//glEnableVertexAttribArray(0);
		//glBindBuffer(GL_ARRAY_BUFFER, track_vertexbuffer);
		//glVertexAttribPointer(
		//	0,                  // attribute
		//	3,                  // size
		//	GL_FLOAT,           // type
		//	GL_FALSE,           // normalized?
		//	0,                  // stride
		//	(void*)0            // array buffer offset
		//	);

		//// 2nd attribute buffer : UVs
		//glEnableVertexAttribArray(1);
		//glBindBuffer(GL_ARRAY_BUFFER, track_uvbuffer);
		//glVertexAttribPointer(
		//	1,                                // attribute
		//	2,                                // size
		//	GL_FLOAT,                         // type
		//	GL_FALSE,                         // normalized?
		//	0,                                // stride
		//	(void*)0                          // array buffer offset
		//	);

		//// 3rd attribute buffer : normals
		//glEnableVertexAttribArray(2);
		//glBindBuffer(GL_ARRAY_BUFFER, track_normalbuffer);
		//glVertexAttribPointer(
		//	2,                                // attribute
		//	3,                                // size
		//	GL_FLOAT,                         // type
		//	GL_FALSE,                         // normalized?
		//	0,                                // stride
		//	(void*)0                          // array buffer offset
		//	);

		//// Index buffer
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, track_elementbuffer);

		//// Draw the triangles !
		//glDrawElements(
		//	GL_TRIANGLES,      // mode
		//	track_indices.size(),    // count
		//	GL_UNSIGNED_SHORT, // type
		//	(void*)0           // element array buffer offset
		//	);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		/*==========================================begin to draw man============================================*/
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texturecar);
		glUniform1i(TextureID, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_car);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_car);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_car);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_car);
		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVP_car[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix_car[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices_car.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
			);

		glBindTexture(GL_TEXTURE_2D, Texturetyre);
		glUniform1i(TextureID, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_tyre);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_tyre);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);


		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_tyre);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_tyre);
		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform

		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniform1f(specDegreeID, 3.0);
		glUniform1f(diffDegreeID, 0.0);
		// Draw the triangles !
		for (int i = 0; i < 4; i++)
		{
			glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVPtyreArray[i][0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrixTyreArray[i][0][0]);
			glDrawElements(
				GL_TRIANGLES,      // mode
				indices_tyre.size(),    // count
				GL_UNSIGNED_SHORT, // type
				(void*)0           // element array buffer offset
				);
		}


		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		if (lightOn)
		{
			glDepthMask(0);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glUniform1i(classNrID, 2);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texturelightgun);
			glUniform1i(TextureID, 0);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_lightgun);
			glVertexAttribPointer(
				0,                  // attribute
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_lightgun);
			glVertexAttribPointer(
				1,                                // attribute
				2,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
				);

			// 3rd attribute buffer : normals
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_lightgun);
			glVertexAttribPointer(
				2,                                // attribute
				3,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
				);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_lightgun);
			// Draw the triangles !
			glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVP_lightgun[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrixLightGun[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

			glDrawElements(
				GL_TRIANGLES,      // mode
				indices_lightgun.size(),    // count
				GL_UNSIGNED_SHORT, // type
				(void*)0           // element array buffer offset
				);
			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glDisable(GL_BLEND);
			glDepthMask(1);
		}
		/*====================================开始画透明的物体==================================*/
		/*{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUniform1i(classNrID, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texturecar);
		glUniform1i(TextureID, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_car);
		glVertexAttribPointer(
		0,                  // attribute
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_car);
		glVertexAttribPointer(
		2,                                // attribute
		3,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_car);
		// Send our transformation to the currently bound shader,
		// in the "MVP" uniform
		mat4 testMVP = ProjectionMatrix * ViewMatrix * translate(mat4(), vec3(0, 2, 0));
		glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &testMVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &translate(mat4(), vec3(0, 2, 0))[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		// Draw the triangles !
		glDrawElements(
		GL_TRIANGLES,      // mode
		indices_car.size(),    // count
		GL_UNSIGNED_SHORT, // type
		(void*)0           // element array buffer offset
		);
		glDisable(GL_BLEND);
		}*/
		/*==============================================================开始在FBO4HGB上绘制============================*/
		glBindFramebuffer(GL_FRAMEBUFFER, FB4HGB);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(HGBProgramID);
		glUniform1f(blurBiasID, 0.002);
		glUniform1f(blurDirecID, true);
		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, BrightTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(srcGBtexID, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
		//glViewport(0, 0, 946, 526);
		// Draw the triangle !
		//	 You have to disable GL_COMPARE_R_TO_TEXTURE above in order to see anything 
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles	

		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glUniform1f(blurDirecID, false);
		glBindTexture(GL_TEXTURE_2D, HGBTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(srcGBtexID, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles	
		glDisableVertexAttribArray(1);

		/*==============================================================开始在真正的屏幕上绘制============================*/

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, 1366, 766);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(quad_programID);
		glUniform1f(SpeedinQuadID, carBody.speed);
		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ResTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(coltexID, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, InforTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(inftexID, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, HVGBTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(britexID, 2);


		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
		//glViewport(0, 0, 946, 526);
		// Draw the triangle !
		//	 You have to disable GL_COMPARE_R_TO_TEXTURE above in order to see anything 
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles		
		glDisableVertexAttribArray(1);
		/*===========================================观察高斯模糊结果=========================*/
		if (bool wannaLookBrightNess = false || (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS))
		{
			glUseProgram(HGBProgramID);
			glViewport(0, 0, 960, 540);
			glDepthFunc(GL_LEQUAL);
			glUniform1f(blurBiasID, 0.0);
			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, HVGBTexture);
			// Set our "renderedTexture" sampler to user Texture Unit 0
			glUniform1i(srcGBtexID, 0);

			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);
			//glViewport(0, 0, 946, 526);
			// Draw the triangle !
			//	 You have to disable GL_COMPARE_R_TO_TEXTURE above in order to see anything 
			glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles		
			glDisableVertexAttribArray(1);
		}

		/*==========================================================输出参数======================================*/
		glViewport(0, 0, 1366, 766);
		glDepthFunc(GL_LEQUAL);
		currentTime = glfwGetTime();
		nbFrames++;
		char text[256];
		//sprintf(text, "%.2f %.2f %.2f", spf, loopTime, subTime);
		sprintf(text, "speed:%.2f     time:%.2f", carBody.speed, currentTime);
		printText2D(text, 10, 550, 30);
		/*==========================================================输出参数完毕======================================*/

		if (bool needDepthInformation = false)
		{
			// Optionally render the shadowmap (for debug only)
			glDisable(GL_COMPARE_R_TO_TEXTURE);//?????????????????
			// Render only on a corner of the window (or we we won't see the real rendering...)
			glViewport(0, 0, 512, 512);

			// Use our shader
			glUseProgram(quad_programID);

			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			// Set our "renderedTexture" sampler to user Texture Unit 0
			glUniform1i(coltexID, 0);

			// 1rst attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);

			// Draw the triangle !
			// You have to disable GL_COMPARE_R_TO_TEXTURE above in order to see anything !
			glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
			glDisableVertexAttribArray(0);
		}



		glfwSwapBuffers(window);
		float midTime = glfwGetTime();
		glfwPollEvents();

		endTime = glfwGetTime();
		if (currentTime - lastTime >= 1.0) { // If last prinf() was more than 1sec ago
			// printf and reset
			spf = 1000.0 / double(nbFrames);
			nbFrames = 0;
			lastTime += 1.0;
			loopTime = 1000.0*(endTime - startTime);
			subTime = 1000.0*(midTime - startTime);
		}
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &track_vertexbuffer);
	glDeleteBuffers(1, &track_uvbuffer);
	glDeleteBuffers(1, &track_normalbuffer);
	glDeleteBuffers(1, &track_elementbuffer);
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);
	glDeleteBuffers(1, &vertexbuffer_car);
	glDeleteBuffers(1, &uvbuffer_car);
	glDeleteBuffers(1, &normalbuffer_car);
	glDeleteBuffers(1, &elementbuffer_car);
	glDeleteProgram(programID);
	glDeleteTextures(1, &TextureEnvi);
	glDeleteTextures(1, &ResTexture);
	glDeleteTextures(1, &InforTexture);
	glDeleteFramebuffers(1, &FramebufferName);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Delete the text's VBO, the shader and the texture
	cleanupText2D();

	// Close OpenGL window and terminate GLFW
	//TwTerminate();
	glfwTerminate();

	return 0;
}

