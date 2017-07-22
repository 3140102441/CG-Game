// Include GLFW
//problem: return to the state being in the air casused by getting out of the trangle.
#include "GLFW\glfw3.h"
extern GLFWwindow* window; // The "extern" keyword here is to access the variable "window" declared in tutorialXXX.cpp. This is a hack to keep the tutorials simple. Please avoid this.

// Include GLM
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/norm.hpp"
#include <vector>
#include <queue>

#include <iostream>
using namespace glm;
using namespace std;
#include "ghost.h"
#include "controls.hpp"
#include "quaternion_utils.hpp"
glm::mat4 ViewMatrix;
glm::mat4 FPViewMatrix;
glm::mat4 BackViewMatrix;
glm::mat4 FixedViewMatrix;
glm::mat4 TPViewMatrix;
glm::mat4 ProjectionMatrix;
glm::mat4 RotationMatrix_car;
glm::mat4 ModelMatrix_tyre1;
glm::mat4 ModelMatrix_tyre2;
glm::mat4 ModelMatrix_tyre3;
glm::mat4 ModelMatrix_tyre4;
glm::mat4 ModelMatrix_car;
glm::mat4 ModelMatrix_paiqiguan;
glm::mat4 ModMtrxLightGun;
glm::vec3 direction_result;
glm::vec3 camera_org;
glm::vec3 cmraDirecBefore;
vec3 direction(0, 0, -1);
vec3 velocity(0, 0, -1);
vec3 gravity(0, -9.8, 0);
glm::quat FrontRotation;

// Initial position : on +Z


// Initial horizontal angle : toward -Z
int ViewState;
bool flag4ViewState = false;
bool flag4ScanState = false;
bool flag4GravityCtrl = false;
bool flag4lightSwitch = false;
bool islight = false;

float horizontalAngle = 0.0f;
// Initial vertical angle : none
float verticalAngle = 0.0f;

float FrontAxisAngle = 0.0f;
// Initial Field of View
const float cos_maxslope = 0.5;
const float initialFoV = 45.0f;
const float reboundRate = 0.1;
const float reboundRate_1 = 0.8;
const float speed = 3.0; // 3 units / second
const float rotspeed = 9000.0; // 3 units / second
const float littleHeight = 0.002; // 3 units / second
float mouseSpeed;
float acceleration = 8.0;
float maxV = 100.0;
//////////////////////////////
bool ExistGravity = true;
/////////////////////////////
float rotAngleTyre = 0.0;
float spinAngleTyre = 0.0;
float scandist = -0.5;
bool isScanning = false;
bool isshift = false;
queue <vec3> CmraDirecQue;

vec3 getCmraDirecBefore(ghost * car) {
	int nrFrames = 8;
	if (CmraDirecQue.size() > nrFrames)
		CmraDirecQue.pop();
	CmraDirecQue.push(cross((*car).top, (*car).right));
	return CmraDirecQue.front();

}

vec3 getCameraOrg() {
	return camera_org;
}
glm::mat4 getViewMatrix() {
	return ViewMatrix;
}

glm::mat4 getProjectionMatrix() {
	return ProjectionMatrix;
}
glm::vec3 getViewDirection()
{
	return direction_result;
}

glm::quat getFrontRotation()
{
	return FrontRotation;
}

glm::mat4 getRotationMatrix_car()
{
	return RotationMatrix_car;
}

glm::mat4 getModMtrxLightGun()
{
	return ModMtrxLightGun;
}
glm::mat4 getModelMatrix_car()
{
	return ModelMatrix_car;
}
glm::mat4 getModelMatrix_paiqiguan()
{
	return ModelMatrix_paiqiguan;
}
glm::mat4 getModMtrxTyre1()
{
	return ModelMatrix_tyre1;
}
glm::mat4 getModMtrxTyre2()
{
	return ModelMatrix_tyre2;
}
glm::mat4 getModMtrxTyre3()
{
	return ModelMatrix_tyre3;
}
glm::mat4 getModMtrxTyre4()
{
	return ModelMatrix_tyre4;
}
void lightSwitch(bool * lightOn)
{
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && islight == false)
	{
		islight = true;
		if (*lightOn == false){
			*lightOn = true;
		}
		else {
			*lightOn = false;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE){
		islight = false;
	}
}
bool IsEqual_Float(float a, float b, float forgiveness)/*�ж�һ������һ���������ϵ�ͶӰ�Ƿ������ڲ�*/
{
	return a <= b + forgiveness && a > b - forgiveness;
}
bool IsInPlane(vec3 dotPos, vec3 A, vec3 B, vec3 C, vec3 normal, float * u_out, float * v_out)/*�ж�һ������һ���������ϵ�ͶӰ�Ƿ������ڲ�*/
{
	vec3 CrossPos, Node, A2Cross, A2Node, AB, AC;
	/*vec3 touchPos, oldTouchPos;
	touchPos = carPos - normal_surface[i] * r;
	oldTouchPos = old_car_pos - normal_surface[i] * r;*/
	vec3 PA = A - dotPos;
	float temp_distance = dot(PA, -normal);
	float u = 0;
	float v = 0;
	CrossPos = dotPos - normal * temp_distance;//����λ�õ������ͶӰ
	A2Cross = CrossPos - A;
	AB = B - A;
	AC = C - A;
	u = (dot(AB, AB) * dot(A2Cross, AC) - dot(AB, AC)*dot(A2Cross, AB)) / (dot(AC, AC)*dot(AB, AB) - dot(AC, AB)*dot(AB, AC));
	v = (dot(AC, AC) * dot(A2Cross, AB) - dot(AC, AB)*dot(A2Cross, AC)) / (dot(AC, AC)*dot(AB, AB) - dot(AC, AB)*dot(AB, AC));
	if (u_out)
		*u_out = u;
	if (v_out)
		*v_out = v;
	return u >= 0 && u <= 1 && v >= 0 && v <= 1 && u + v <= 1;
}
float getScanDistRT()
{
	return scandist;
}
void computeMatricesFromInputs_my(std::vector<glm::vec3>  normal_surfaces, std::vector<glm::vec3>  vertices, ghost * man) {

	// glfwGetTime is called only once, the first time this function is called
	static double lastTime = glfwGetTime();
	vec2 tempXZdirection;
	float disCarTyre = 0.038;

	float maxRotAglTyre = 0.45;
	const float rotSpeedTyre = 6.28;
	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	// Get mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Reset mouse position for next frame
	glfwSetCursorPos(window, 1366 / 2, 768 / 2);

	// Compute new orientation
	if ((*man).subRotTimes > 0)
		mouseSpeed = 0;
	else
		mouseSpeed = 0.0008f;
	horizontalAngle = mouseSpeed * float(1366 / 2 - xpos);//sin x = x
	//verticalAngle = mouseSpeed * float(768 / 2 - ypos);


	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && ((*man).onRoad) && !(*man).isCrash) {
		if ((*man).speed >= 3)
			horizontalAngle = (*man).isBack ? -rotspeed * mouseSpeed * deltaTime / (*man).speed : rotspeed * mouseSpeed * deltaTime / (*man).speed;//sin x = x
		if (rotAngleTyre - deltaTime * rotSpeedTyre > -maxRotAglTyre)
			rotAngleTyre -= deltaTime * rotSpeedTyre;
		else
			rotAngleTyre = -maxRotAglTyre;
	}
	// Move backward
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && ((*man).onRoad) && !(*man).isCrash) {
		if ((*man).speed >= 3)
			horizontalAngle = (*man).isBack ? rotspeed * mouseSpeed * deltaTime / (*man).speed : -rotspeed * mouseSpeed * deltaTime / (*man).speed;//sin x = x
		if (rotAngleTyre + deltaTime * rotSpeedTyre <  maxRotAglTyre)
			rotAngleTyre += deltaTime * rotSpeedTyre;
		else
			rotAngleTyre = maxRotAglTyre;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) != GLFW_PRESS && glfwGetKey(window, GLFW_KEY_RIGHT) != GLFW_PRESS && (*man).onRoad) {
		if (rotAngleTyre > 0 && (rotAngleTyre - deltaTime * rotSpeedTyre * 0.5) < 0)
			rotAngleTyre = 0.0;
		else if (rotAngleTyre < 0 && (rotAngleTyre + deltaTime * rotSpeedTyre * 0.5) > 0)
			rotAngleTyre = 0.0;
		else
			rotAngleTyre += (rotAngleTyre > 0) ? -deltaTime * rotSpeedTyre * 0.5 : deltaTime * rotSpeedTyre * 0.5;
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS){
		acceleration = 20;
		(*man).maxSpeed = 50;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) != GLFW_PRESS){
		acceleration = 8;
		(*man).maxSpeed = 15;
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
		isshift = true;
		if ((*man).speed - deltaTime * 10 >= 0)
			(*man).speed -= deltaTime * 10;
		else
			(*man).speed = 0;

		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
			horizontalAngle += rotspeed * mouseSpeed * deltaTime * (*man).speed / 100;//sin x = x

		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
			horizontalAngle = -rotspeed * mouseSpeed * deltaTime * (*man).speed / 100;//sin x = x
		}
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE){
		isshift = false;
	}



	if ((*man).onRoad)
	{
		//printf("�ڵ��Ͽ�����\n");
		(*man).top = normal_surfaces[(*man).touchPlane];
		quat UpRotation(cos(horizontalAngle / 2), sin(horizontalAngle / 2) * (*man).top);
		(*man).right = UpRotation * (*man).right;
		if ((*man).isBack) {
			(*man).rightV = -(*man).right;
			(*man).topV = (*man).top;
		}
		else {
			(*man).rightV = (*man).right;
			(*man).topV = (*man).top;
		}
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && (*man).onRoad) {
		if ((*man).isBack) {
			(*man).speed -= deltaTime * acceleration;
			if ((*man).speed < 0) {
				(*man).speed = 0;
				(*man).isBack = false;
				(*man).rightV = -(*man).rightV;
			}
		}
		else {
			if ((*man).speed + deltaTime * acceleration < (*man).maxSpeed)
				(*man).speed += deltaTime * acceleration;
			else if ((*man).speed >(*man).maxSpeed){
				(*man).speed -= deltaTime * 15;
			}
			else
				(*man).speed = (*man).maxSpeed;
		}

		(*man).isCrash = false;
	}
	if (glfwGetKey(window, GLFW_KEY_UP) != GLFW_PRESS && glfwGetKey(window, GLFW_KEY_DOWN) != GLFW_PRESS && (*man).onRoad){
		if ((*man).speed - deltaTime * 0.02 * acceleration >= 0)
			(*man).speed -= deltaTime * 8;
		else
			(*man).speed = 0;
	}
	// Move backward
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && ((*man).onRoad)) {
		if (!(*man).isBack){
			(*man).speed -= deltaTime * acceleration * 1.6;
			if ((*man).speed < 0) {
				(*man).speed = 0;
				(*man).isBack = true;
				(*man).rightV = -(*man).rightV;
			}
		}
		else{
			if ((*man).speed + deltaTime * 0.6 * acceleration < 0.5 * (*man).maxSpeed)
				(*man).speed += deltaTime * 0.6 * acceleration;
			else
				(*man).speed = 0.5 * (*man).maxSpeed;
		}

		(*man).isCrash = false;
	}

	else
		FrontRotation = quat(1, 0, 0, 0);

	/******************************************************�ж��Ƿ���ײ*****************************************************/

	if ((*man).subRotTimes == 0)
	{
		vec3 PA;
		for (int i = 0; i < vertices.size() / 3; i++)
		{
			PA = vertices[3 * i] - (*man).position;
			float temp_distance = dot(PA, -normal_surfaces[i]);
			if (temp_distance > -(*man).SpeedPerFrame && temp_distance < (*man).SpeedPerFrame + 1)
				(*man).mayHit[i] = true;
			else
				(*man).mayHit[i] = false;
			if ((*man).onRoad && (*man).touchPlane<vertices.size() / 3) {/*��·�ϵ�ʱ��ȥ����������·��ķ��߷�����ȵ������ײ���*/
				if (normal_surfaces[i] == normal_surfaces[(*man).touchPlane] && IsEqual_Float(temp_distance, (*man).size, 0.01)
					&& IsInPlane((*man).position, vertices[3 * i], vertices[3 * i + 1], vertices[3 * i + 2], normal_surfaces[i], NULL, NULL)){
					(*man).mayHit[i] = false;
					(*man).touchPlane = i;
				}
			}
		}
		//	(*man).onRoad = farAway ? false : true;
		//v_car = normalize(v_car);
		for (int i = 0; i < vertices.size() / 3; i++)
		{
			if (!(*man).mayHit[i])
				continue;

			vec3 CrossPos, Node, A2Cross, A2Node, AB, AC;
			/*vec3 touchPos, oldTouchPos;
			touchPos = carPos - normal_surface[i] * r;
			oldTouchPos = old_car_pos - normal_surface[i] * r;*/
			vec3 PA = vertices[3 * i] - (*man).position;
			float temp_distance = dot(PA, -normal_surfaces[i]);
			float u;
			float v;

			if (IsInPlane((*man).position, vertices[3 * i], vertices[3 * i + 1], vertices[3 * i + 2], normal_surfaces[i], &u, &v) && temp_distance <= (*man).size
				&& ((*man).Point_Surface_rposition[i][0]> (*man).size
				|| ((*man).Point_Surface_rposition[i][0]> 0 && (!((*man).Point_Surface_rposition[i][1] < 1 && (*man).Point_Surface_rposition[i][2]>0 && (*man).Point_Surface_rposition[i][2] < 1 && (*man).Point_Surface_rposition[i][1] + (*man).Point_Surface_rposition[i][2] < 1)))))
			{
				vec3 refleV, paraDirection;
				(*man).lastVelocity = velocity;
				camera_org += normal_surfaces[i] * ((*man).size - temp_distance);
				(*man).position += normal_surfaces[i] * ((*man).size - temp_distance);
				
				refleV = normalize(reflect((*man).lastVelocity, normal_surfaces[i]))*(*man).speed;
				if (normal_surfaces[i][1] > cos_maxslope)/*��ײ��������¶�С��60�ȣ�Ĭ����Ϊ·��*/
				{
					printf("ײ������\n");
					if ((*man).isBack)
						printf("������ʱ��ײ�����ˣ�");
					(*man).rotationTimesSum = 1;
					paraDirection = normalize(cross(normal_surfaces[i], (*man).right));
					//	(*man).newVelocity = reflect((*man).lastVelocity, normal_surfaces[i]);
					(*man).right = normalize(cross(paraDirection, normal_surfaces[i]));
					(*man).top = normal_surfaces[i];
					(*man).newVelocity = paraDirection * dot(refleV, paraDirection) + normal_surfaces[i] * dot(refleV, normal_surfaces[i]) * reboundRate;
				}
				else/*ײ�������ϰ���*/
				{
					printf("ײ��ǽ��\n");
					if (dot(-normalize(velocity), normal_surfaces[i]) > 0.707)
						(*man).isCrash = true;
					(*man).onRoad = false;
					(*man).rotationTimesSum = floor(4 + 16.0 / pow((*man).speed, 0.1));
					(*man).rotationTimesSum = 2;
					(*man).newVelocity = refleV * reboundRate_1;
					(*man).position += vec3(0, littleHeight, 0);
					
					(*man).mayHit[(*man).touchPlane] = true;
					camera_org += vec3(0, littleHeight, 0);
				}
				//else
				//{
				//	printf("ײ��������\n");
				//	(*man).isCrash = true;
				//	(*man).newVelocity = vec3(0, 0, 0);
				//	(*man).position += vec3(0, littleHeight, 0);
				//	(*man).mayHit[(*man).touchPlane] = true;
				//	camera_org += vec3(0, littleHeight, 0);
				//}
				(*man).onRoad = false;
				(*man).touchPlane = i;
				(*man).subRotTimes = (*man).rotationTimesSum;
				(*man).lastTempCam2car = (*man).lastVelocity;
				(*man).SpeedAfterHit = length((*man).newVelocity);
				(*man).speed = 0;
				break;
			}
			(*man).Point_Surface_rposition[i][0] = temp_distance;
			(*man).Point_Surface_rposition[i][1] = u;
			(*man).Point_Surface_rposition[i][2] = v;
		}
	}
	//else if ((*man).subRotTimes >= (*man).rotationTimesSum*0.25)
	else if ((*man).subRotTimes > 0)
	{
		(*man).subRotTimes--;
		if ((*man).subRotTimes == 0 && dot((*man).newVelocity, normal_surfaces[(*man).touchPlane]) < 0.4 && normal_surfaces[(*man).touchPlane][1] >= cos_maxslope)/*�Ƿ����ƽ�ȵĹ������*/
		{
			printf("�ص��ܵ���ƽ�����У������ٶ�\n");
			if ((*man).isBack) {
				(*man).rightV = -(*man).right;
				(*man).topV = (*man).top;
			}
			else {
				(*man).rightV = (*man).right;
				(*man).topV = (*man).top;
			}

			if (!(*man).isCrash)
				(*man).speed = (*man).SpeedAfterHit;
			(*man).onRoad = true;
		}
		else
		{
			vec3 tempCamera2car;
			tempCamera2car = normalize((*man).lastVelocity * float((float)(*man).subRotTimes / (*man).rotationTimesSum) + (*man).newVelocity * float(((*man).rotationTimesSum - (float)(*man).subRotTimes) / (*man).rotationTimesSum));
			quat rotationCamera = RotationBetweenVectors((*man).lastTempCam2car, tempCamera2car);
			(*man).lastTempCam2car = tempCamera2car;
			//if ((*man).isCrash) {
			//	(*man).right = rotationCamera * (*man).right;
			//	(*man).top = rotationCamera * (*man).top;
			//}
			(*man).rightV = rotationCamera * (*man).rightV;
			(*man).topV = rotationCamera * (*man).topV;
			quat rotation2horizion = RotationBetweenVectors((*man).right, cross(cross((*man).top, (*man).right), vec3(0, 1, 0)));
			(*man).right = rotation2horizion * (*man).right;
			(*man).top = rotation2horizion * (*man).top;
			if ((*man).subRotTimes == 0)
			{
				printf("ײ�����֮�����������ٶ�\n");
				(*man).speed = (*man).SpeedAfterHit;
			}
		}
	}
	/**********************************************************�����Ƿ��뿪�˵�ǰ���ڱ���***************************/
	if ((*man).onRoad&&!IsInPlane((*man).position, vertices[3 * (*man).touchPlane], vertices[3 * (*man).touchPlane + 1], vertices[3 * (*man).touchPlane + 2], normal_surfaces[(*man).touchPlane], NULL, NULL))
	{
		printf("�ڵ����ܵĳ��ɳ���������\n");
		(*man).onRoad = false;
	}

	/********************************************************��������**********************************************/
	if (ExistGravity && !(*man).onRoad && (*man).subRotTimes == 0)//ֻ�������������ڿ��У��Ҳ�����ת�е�����Ż��յ���������
	{
		printf("����������Ӱ��\n");
		vec3 tempDirection = normalize(cross((*man).topV, (*man).rightV));
		vec3 newV = tempDirection * (*man).speed + gravity * deltaTime;
		vec3 newDirection = normalize(newV);
		(*man).topV = RotationBetweenVectors(tempDirection, newDirection) * (*man).topV;
		(*man).rightV = RotationBetweenVectors(tempDirection, newDirection) * (*man).rightV;
		(*man).speed = length(newV);
	}
	(*man).SpeedPerFrame = (*man).speed>0 ? (*man).speed  * deltaTime : -(*man).speed * deltaTime;
	direction = normalize(glm::cross((*man).top, (*man).right));
	velocity = normalize(glm::cross((*man).topV, (*man).rightV));
	camera_org = (*man).position - direction;
	camera_org += velocity * (*man).speed  * deltaTime;
	(*man).position += velocity * (*man).speed  * deltaTime;
	
	// Move forward


	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
		flag4ViewState = true;
	}
	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE) {
		if (flag4ViewState == true)
		{
			ViewState = (++ViewState) % 2;
			flag4ViewState = false;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		flag4ScanState = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE && flag4ScanState == true) {
		isScanning = true;
		scandist = -0.5;
		flag4ScanState = false;
	}
	scandist = isScanning ? scandist + deltaTime * 2.0 : scandist;
	if (scandist > 20)
	{
		isScanning = false;
		scandist = -0.5;
	}
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
		flag4GravityCtrl = true;
	}
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE) {
		if (flag4GravityCtrl == true)
		{
			ExistGravity = ExistGravity ? false : true;
			flag4GravityCtrl = false;
		}
	}


	direction_result = direction;
	float FoV = initialFoV;// - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires setting up a callback for this. It's a bit too complicated for this beginner's tutorial, so it's disabled instead.

	// Projection matrix : 45?Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	ProjectionMatrix = glm::perspective(FoV, 1366.0f / 768.0f, 0.1f, 1366.0f);
	// Camera matrix

	FPViewMatrix = glm::lookAt(
		camera_org,           // Camera is here
		camera_org + direction, // and looks here : at the same position, plus "direction"
		(*man).top                  // Head is up (set to 0,-1,0 to look upside-down)
		);
	/*vec3 FixedCamera2car = *position + direction - vec3(0, 3, 1);
	vec3 up4FixedCamera = FixedCamera2car + vec3(0, -length(FixedCamera2car)*length(FixedCamera2car) / FixedCamera2car[1], 0);
	if (up4FixedCamera[1] < 0)
	up4FixedCamera[1] = -up4FixedCamera[1];*/
	glm::mat4 InverseViewMatrix = inverse(FPViewMatrix);
	(*man).right = normalize(vec3((InverseViewMatrix * vec4(1.f, 0.0f, 0.0f, 1.0f))) - camera_org);
	(*man).top = normalize(vec3((InverseViewMatrix * vec4(0.f, 1.0f, 0.0f, 1.0f))) - camera_org);

	cmraDirecBefore = getCmraDirecBefore(man);
	tempXZdirection = normalize(vec2(cmraDirecBefore[0], cmraDirecBefore[2]));
	switch (ViewState) {
	case(0) :
		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			ViewMatrix = glm::lookAt(
				camera_org + direction,           // Camera is here
				camera_org, // and looks here : at the same position, plus "direction"
				(*man).top             // Head is up (set to 0,-1,0 to look upside-down)
				);
		}
		else
			ViewMatrix = FPViewMatrix; break;
	case(1) :
		ViewMatrix = glm::lookAt(
		((*man).position - 0.7f *  vec3(tempXZdirection[0], -0.25, tempXZdirection[1])),       // Camera is here
		(*man).position + 0.7f *  vec3(0, 0.25, 0), // and looks here : at the same position, plus "direction"
		vec3(0, 1, 0)                 // Head is up (set to 0,-1,0 to look upside-down)
		); break;
		//case(2):
		//	ViewMatrix = glm::lookAt(
		//		vec3(0, 3, 1),       // Camera is here
		//		*position + direction, // and looks here : at the same position, plus "direction"
		//		up4FixedCamera  // Head is up (set to 0,-1,0 to look upside-down)
		//		); break;
	}
	// For the next frame, the "last time" will be "now"
	lastTime = currentTime;

	RotationMatrix_car = mat4();
	quat rotateOne = RotationBetweenVectors(vec3(0, 1, 0), (*man).top);
	vec3 tempRight = rotateOne*vec3(1, 0, 0);
	quat rotateTwo = RotationBetweenVectors(tempRight, (*man).right);
	quat rotationTyre = quat(cos(rotAngleTyre / 2), -sin(rotAngleTyre / 2)*(*man).top);
	spinAngleTyre += (*man).speed / (0.36 * 0.06);
	spinAngleTyre = spinAngleTyre >= 3.1415926*2000.0 ? 0.0f : spinAngleTyre;
	quat rotSpinType = quat(cos(spinAngleTyre / 2), -sin(spinAngleTyre / 2)*vec3(1, 0, 0));
	RotationMatrix_car = toMat4(rotateTwo) * toMat4(rotateOne) * RotationMatrix_car;
	ModelMatrix_car = translate(mat4(), (*man).position) * translate(mat4(), vec3(-0, disCarTyre * 0.05, 0 * 0.9)) *RotationMatrix_car * scale(mat4(), vec3(0.045, 0.045, 0.045));
	ModelMatrix_paiqiguan = translate(mat4(), (*man).position) * RotationMatrix_car * translate(mat4(), vec3(0.001*disCarTyre - 0.03, -disCarTyre * 0.75 + 0.01, disCarTyre * 0.9 + 0.01)) *rotate(mat4(), -1.57f, vec3(1, 0, 0))* scale(mat4(), vec3(0.008, 0.01, 0.008));
	ModMtrxLightGun = translate(mat4(), (*man).position) * RotationMatrix_car * scale(mat4(), vec3(2.0, 2.0, 2.0));
	ModelMatrix_tyre1 = translate(mat4(), (*man).position) * RotationMatrix_car * translate(mat4(), vec3(-disCarTyre, -disCarTyre * 0.75, disCarTyre * 0.9)) * toMat4(rotSpinType) * scale(mat4(), vec3(0.06, 0.06, 0.06));
	ModelMatrix_tyre2 = translate(mat4(), (*man).position) * RotationMatrix_car * translate(mat4(), vec3(disCarTyre, -disCarTyre * 0.75, disCarTyre * 0.9)) * toMat4(rotSpinType) * scale(mat4(), vec3(0.06, 0.06, 0.06));
	ModelMatrix_tyre3 = translate(mat4(), (*man).position) * RotationMatrix_car * translate(mat4(), vec3(disCarTyre, -disCarTyre * 0.75, -disCarTyre * 0.8)) * toMat4(rotationTyre) * toMat4(rotSpinType) * scale(mat4(), vec3(0.06, 0.06, 0.06));
	ModelMatrix_tyre4 = translate(mat4(), (*man).position) * RotationMatrix_car * translate(mat4(), vec3(-disCarTyre, -disCarTyre * 0.75, -disCarTyre * 0.8)) * toMat4(rotationTyre) * toMat4(rotSpinType) * scale(mat4(), vec3(0.06, 0.06, 0.06));
	direction = cross((*man).right, (*man).top);
}
