#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "utility.hpp"

class Shader
{
public:
	std::string name;
	std::vector<uint32_t> vertex;
	std::vector<uint32_t> fragment;

	Shader() : name(), vertex(), fragment() {}

	Shader& Load(const std::string name)
	{
		this->name = name;
		vertex = ShaderUtil::Create((name + ".vert").c_str(), vk::ShaderStageFlagBits::eVertex);
		fragment = ShaderUtil::Create((name + ".frag").c_str(), vk::ShaderStageFlagBits::eFragment);
		return *this;
	}

	~Shader()
	{
		name.clear();
		vertex.clear();
		fragment.clear();
	}
};
