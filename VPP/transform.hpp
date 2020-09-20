#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Transform
{
public:
	Transform() : position(), rotation(), scale(1.f, 1.f, 1.f) {}
	glm::mat4 getModelMatrix()
	{
		glm::mat4 model = glm::mat4(1.f);
		model = glm::translate(model, position);
		auto angle = glm::radians(rotation);
		model = glm::rotate(model, angle.x, glm::vec3(1.f, 0.f, 0.f));
		model = glm::rotate(model, angle.y, glm::vec3(0.f, 1.f, 0.f));
		model = glm::rotate(model, angle.z, glm::vec3(0.f, 0.f, 1.f));
		model = glm::scale(model, scale);
		return model;
	}
public:
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};