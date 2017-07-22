//////////////////////////////////////////////////////////////////////////////////////////
//	PLANE.cpp
//	function definitions for RGBA color class
//	Downloaded from: www.paulsprojects.net
//	Created:	20th July 2002
//	Modified:	6th August 2002	-	Added "Normalize"
//				7th November 2002	-	Altered constructor layout
//									-	Corrected "lerp"
//
//	Copyright (c) 2006, Paul Baker
//	Distributed under the New BSD Licence. (See accompanying file License.txt or copy at
//	http://www.paulsprojects.net/NewBSDLicense.txt)
//////////////////////////////////////////////////////////////////////////////////////////	
#include "glm/glm.hpp"
using namespace glm;
#include "PLANE.h"

void PLANE::SetFromPoints(const vec3 & p0, const vec3 & p1, const vec3 & p2)
{
	normal = cross((p1-p0),(p2-p0));

	normal = normalize(normal);

	CalculateIntercept(p0);
}

void PLANE::Normalize()
{
	float normalLength=length(normal);
	normal/=normalLength;
	intercept/=normalLength;
}

bool PLANE::Intersect3(const PLANE & p2, const PLANE & p3, vec3 & result)//find point of intersection of 3 planes
{
	float denominator=dot(normal,cross((p2.normal),(p3.normal)));
											//scalar triple product of normals
	if(denominator==0.0f)									//if zero
		return false;										//no intersection

	vec3 temp1, temp2, temp3;
	temp1= cross(p2.normal,(p3.normal))*intercept;
	temp2= cross(p3.normal,(normal))*p2.intercept;
	temp3= cross(normal,(p2.normal))*p3.intercept;

	result=(temp1+temp2+temp3)/(-denominator);

	return true;
}

float PLANE::GetDistance(const vec3 & point) const
{
	return point.x*normal.x + point.y*normal.y + point.z*normal.z + intercept;
}

int PLANE::ClassifyPoint(const vec3 & point) const
{
	float distance=point.x*normal.x + point.y*normal.y + point.z*normal.z + intercept;

	if(distance>0.01)	//==0.0f is too exact, give a bit of room
		return POINT_IN_FRONT_OF_PLANE;
	
	if(distance<-0.01)
		return POINT_BEHIND_PLANE;

	return POINT_ON_PLANE;	//otherwise
}

PLANE PLANE::lerp(const PLANE & p2, float factor)
{
	PLANE result;
	result.normal=normal*(1.0f-factor) + p2.normal*factor;
	result.normal = normalize(result.normal);

	result.intercept=intercept*(1.0f-factor) + p2.intercept*factor;

	return result;
}

bool PLANE::operator ==(const PLANE & rhs) const
{
	if(normal==rhs.normal && intercept==rhs.intercept)
		return true;

	return false;
}
