//////////////////////////////////////////////////////////////////////////////////////////
//	SHADOW_MODEL.h
//	class declaration for shadow volume equipped model
//	Downloaded from: www.paulsprojects.net
//	Created:	3rd September 2002
//
//	Copyright (c) 2006, Paul Baker
//	Distributed under the New BSD Licence. (See accompanying file License.txt or copy at
//	http://www.paulsprojects.net/NewBSDLicense.txt)
//////////////////////////////////////////////////////////////////////////////////////////	

#ifndef SHADOW_MODEL_H
#define SHADOW_MODEL_H
class SHADOW_MODEL_VERTEX
{
public:
	vec3 position;
	vec3 normal;
};

class SHADOW_MODEL
{
public:
	GLuint numIndices;
	GLuint * indices;

	GLuint numTriangles;

	GLuint numVertices;
	SHADOW_MODEL_VERTEX * vertices;

	//Store the plane equation for each face
	PLANE * planeEquations;

	//For each face, does it face the light?
	bool * isFacingLight;

	//For each edge, which is the neighbouring face?
	GLint * neighbourIndices;

	//For each edge, is it a silhouette edge?
	bool * isSilhouetteEdge;

	bool GenerateAnything(const char * path);
	void SetConnectivity();
	void CalculateSilhouetteEdges(vec3 lightPosition);
	void DrawInfiniteShadowVolume(vec3 lightPosition, bool drawCaps, std::vector<glm::vec3> * svVertuces, std::vector<unsigned short> * svElement);
};

#endif	//SHADOW_MODEL_H