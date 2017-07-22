#include <windows.h>
#include <vector>
#include "GL/glew.h"
#include "glm/glm.hpp"
using namespace glm;
#include "PLANE.h"

#include "SHADOW_MODEL.h"
#include <iostream>


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool SHADOW_MODEL::GenerateAnything(const char * path)
{

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;
	std::vector<glm::vec3>  out_vertices;
	std::vector<glm::vec2>  out_uvs;
	std::vector<glm::vec3>  out_normals;
	FILE * file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}
	while (1) {

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]-1);
			vertexIndices.push_back(vertexIndex[1]-1);
			vertexIndices.push_back(vertexIndex[2]-1);
			uvIndices.push_back(uvIndex[0] - 1);
			uvIndices.push_back(uvIndex[1] - 1);
			uvIndices.push_back(uvIndex[2] - 1);
			normalIndices.push_back(normalIndex[0] - 1);
			normalIndices.push_back(normalIndex[1] - 1);
			normalIndices.push_back(normalIndex[2] - 1);
		}
		else {
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}
	numVertices = temp_vertices.size();//各异的点的个数
	numIndices = vertexIndices.size();//重复的点的个数
	numTriangles = numIndices / 3;

	vertices = new SHADOW_MODEL_VERTEX[numVertices];
	indices = new unsigned int[numIndices];
	for (int i = 0; i < numIndices; i++)
		indices[i] = vertexIndices[i];
	for (int i = 0; i < numIndices; i++)
		vertices[indices[i]].normal = vertices[indices[i]].normal + vec3(temp_normals[normalIndices[i]]);

	for (int i = 0; i<numVertices; i++)
	{
		vertices[i].position = vec3(temp_vertices[i]);

		vertices[i].normal = normalize(vertices[i].normal);
	}

	planeEquations = new PLANE[numTriangles];

	for (unsigned int j = 0; j<numTriangles; ++j)
	{
		planeEquations[j].SetFromPoints(vertices[indices[j * 3 + 0]].position,
			vertices[indices[j * 3 + 1]].position,
			vertices[indices[j * 3 + 2]].position);
	}
	isFacingLight = new bool[numTriangles];
	neighbourIndices = new GLint[numTriangles * 3];
	isSilhouetteEdge = new bool[numTriangles * 3];
	SetConnectivity();

	return true;
}
//Calculate neighbour faces for each edge
void SHADOW_MODEL::SetConnectivity()
{
	//set the neighbour indices to be -1
	for(unsigned int i=0; i<numTriangles*3; ++i)
		neighbourIndices[i]=-1;

	//loop through triangles
	for(unsigned int i=0; i<numTriangles-1; ++i)
	{
		//loop through edges on the first triangle
		for(int edgeI=0; edgeI<3; ++edgeI)
		{
			//continue if this edge already has a neighbour set
			if(neighbourIndices[i*3+edgeI]!=-1)
				continue;

			//loop through triangles with greater indices than this one
			for(unsigned int j=i+1; j<numTriangles; ++j)
			{
				//loop through edges on triangle j
				for(int edgeJ=0; edgeJ<3; ++edgeJ)
				{
					//get the vertex indices on each edge
					int edgeI1=indices[i*3+edgeI];
					int edgeI2=indices[i*3+(edgeI+1)%3];
					int edgeJ1=indices[j*3+edgeJ];
					int edgeJ2=indices[j*3+(edgeJ+1)%3];

					//if these are the same (possibly reversed order), these faces are neighbours
					if(		(edgeI1==edgeJ1 && edgeI2==edgeJ2)
						||	(edgeI1==edgeJ2 && edgeI2==edgeJ1))
					{
						neighbourIndices[i*3+edgeI]=j;
						neighbourIndices[j*3+edgeJ]=i;
					}
				}
			}
		}
	}
}

//calculate silhouette edges
void SHADOW_MODEL::CalculateSilhouetteEdges(vec3 lightPosition)
{
	int count = 0;
	//Calculate which faces face the light
	for(unsigned int i=0; i<numTriangles; ++i)
	{
		if(planeEquations[i].ClassifyPoint(lightPosition)==POINT_IN_FRONT_OF_PLANE)
			isFacingLight[i]=true;
		else
			isFacingLight[i]=false;
	}

	//loop through edges
	for(unsigned int i=0; i<numTriangles*3; ++i)
	{
		//if this face is not facing the light, not a silhouette edge
		if(!isFacingLight[i/3])
		{
			isSilhouetteEdge[i]=0;
			continue;
		}

		//this face is facing the light
		//if the neighbouring face is not facing the light, or there is no neighbouring face,
		//then this is a silhouette edge
		if(neighbourIndices[i]==-1 || !isFacingLight[neighbourIndices[i]])
		{
			isSilhouetteEdge[i]=1;
			count++;
			continue;
		}

		isSilhouetteEdge[i]=0;
	}
}

void SHADOW_MODEL::DrawInfiniteShadowVolume(vec3 lightPosition, bool drawCaps, std::vector<glm::vec3> * svVertuces, std::vector<unsigned short> * svElement)
{

	//std::vector<unsigned short> indices_SVcar;
	int count = 0;
	unsigned short indice = 0;
		for(unsigned int i=0; i<numTriangles; ++i)
		{
			//if this face does not face the light, continue
			if(!isFacingLight[i])
				continue;
			glColor3f(1.0, 1.0, 1.0);
			//Loop through edges on this face
			for(int j=0; j<3; ++j)
			{
				//Draw the shadow volume "edge" if this is a silhouette edge
				if(isSilhouetteEdge[i*3+j])
				{
					vec3 vertex1=vertices[indices[i*3+j]].position;
					vec3 vertex2=vertices[indices[i*3+(j+1)%3]].position;

					(*svVertuces).push_back(vertex2); (*svElement).push_back(indice++);
					(*svVertuces).push_back(vertex1); (*svElement).push_back(indice++);
					(*svVertuces).push_back(vertex1 -(lightPosition- vertex1)*100.0f); (*svElement).push_back(indice++);
					(*svVertuces).push_back(vertex1 - (lightPosition - vertex1)*100.0f); (*svElement).push_back(indice++);
					(*svVertuces).push_back(vertex2 - (lightPosition - vertex2)*100.0f); (*svElement).push_back(indice++);
					(*svVertuces).push_back(vertex2); (*svElement).push_back(indice++);
					count++;
				}
			}
	     }

	//Draw caps if required
	if(drawCaps)
	{
			for(unsigned int i=0; i<numTriangles; ++i)
			{
				for(int j=0; j<3; ++j)
				{
					vec3 vertex=vertices[indices[i*3+j]].position;
					
					if (isFacingLight[i])
					{
						(*svVertuces).push_back(vertex); (*svElement).push_back(indice++);
					}	
					else
					{
						(*svVertuces).push_back(vertex -(lightPosition- vertex)*100.0f); (*svElement).push_back(indice++);
					}
				}
			}
	}
	//glEnableVertexAttribArray(0);
	//glBindBuffer(GL_ARRAY_BUFFER, svVertices);
	//glBufferData(GL_ARRAY_BUFFER, svVertuces.size() * sizeof(glm::vec3), &svVertuces[0], GL_STATIC_DRAW);
	//glVertexAttribPointer(
	//	0,                  // attribute
	//	3,                  // size
	//	GL_FLOAT,           // type
	//	GL_FALSE,           // normalized?
	//	0,                  // stride
	//	(void*)0            // array buffer offset
	//	);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, svElement);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_SVcar.size() * sizeof(unsigned short), &indices_SVcar[0], GL_DYNAMIC_DRAW);
	//glDrawElements(
	//	GL_TRIANGLES,      // mode
	//	indices_SVcar.size(),    // count
	//	GL_UNSIGNED_SHORT, // type
	//	(void*)0           // element array buffer offset
	//	);
}
