#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <cstdio>
#include <string>
#include <vector>

class Mesh
{
public:
	std::string name;

	std::vector<float> wrapData()
	{
		std::vector<float> data = std::vector<float>(triangles.size() * 3 * (3 + 3 + 2));
		size_t index = 0;
		for (size_t i = 0; i < triangles.size(); i++)
		{
			data[index++] = vertices[triangles[i][0][0]].x;
			data[index++] = vertices[triangles[i][0][0]].y;
			data[index++] = vertices[triangles[i][0][0]].z;

			data[index++] = normals[triangles[i][1][0]].x;
			data[index++] = normals[triangles[i][1][0]].y;
			data[index++] = normals[triangles[i][1][0]].z;

			data[index++] = uvs[triangles[i][2][0]].x;
			data[index++] = uvs[triangles[i][2][0]].y;

			data[index++] = vertices[triangles[i][0][1]].x;
			data[index++] = vertices[triangles[i][0][1]].y;
			data[index++] = vertices[triangles[i][0][1]].z;

			data[index++] = normals[triangles[i][1][1]].x;
			data[index++] = normals[triangles[i][1][1]].y;
			data[index++] = normals[triangles[i][1][1]].z;

			data[index++] = uvs[triangles[i][2][1]].x;
			data[index++] = uvs[triangles[i][2][1]].y;

			data[index++] = vertices[triangles[i][0][2]].x;
			data[index++] = vertices[triangles[i][0][2]].y;
			data[index++] = vertices[triangles[i][0][2]].z;

			data[index++] = normals[triangles[i][1][2]].x;
			data[index++] = normals[triangles[i][1][2]].y;
			data[index++] = normals[triangles[i][1][2]].z;

			data[index++] = uvs[triangles[i][2][2]].x;
			data[index++] = uvs[triangles[i][2][2]].y;
		}
		return data;
	}
	Mesh() : name(), vertices(), normals(), uvs(), triangles() {}
	~Mesh()
	{
		vertices.clear();
		normals.clear();
		uvs.clear();
		triangles.clear();
	}
	static Mesh* Create(const char* filename)
	{
		FILE* input = fopen(filename, "r");
		if (input == nullptr)
		{
			return nullptr;
		}
		Mesh* mesh = new Mesh();
		while (!feof(input))
		{
			char line[1024u];
			fgets(line, 1024u, input);

			auto size = strlen(line);
			if (line[size - 1] != '\n')
			{
				delete mesh;
				return nullptr;
			}
			switch (line[0])
			{
			case 'o':
				sscanf(line, "o %s", mesh->name);
				break;
			case 'v':

				switch (line[1])
				{
				case 't':
				{
					glm::vec2 uv;
					sscanf(line, "vt %f %f", &uv.x, &uv.y);
					mesh->uvs.push_back(uv);
					break;
				}
				case 'n':
				{
					glm::vec3 normal;
					sscanf(line, "vn %f %f %f", &normal.x, &normal.y, &normal.z);
					mesh->normals.push_back(normal);
					break;
				}
				default:
				{
					glm::vec3 vertex;
					sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
					mesh->vertices.push_back(vertex);
					break;
				}
				}

				break;
			case 'f':
			{
				glm::imat3 face;
				face[0][0];
				sscanf(line, "f %u/%u/%u %u/%u/%u %u/%u/%u",
					&face[0][0], &face[1][0], &face[2][0],
					&face[0][1], &face[1][1], &face[2][1],
					&face[0][2], &face[1][2], &face[2][2]);
				for (size_t i = 0; i < 9u; i++)
				{
					face[i/3][i%3]--;
				}
				mesh->triangles.push_back(face);
				break;
			}
			default:
				break;
			}
		}
		fclose(input);
		return mesh;
	}
protected:
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<glm::imat3> triangles;
};