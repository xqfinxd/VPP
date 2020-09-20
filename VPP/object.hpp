#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>

#include "mesh.hpp"
#include "transform.hpp"
#include "texture.hpp"

class Drawable
{
public:
	Drawable(std::string name) : name(name), transform(), mvpMemoryBuffer(), lightMemoryBuffer(), cameraMemoryBuffer()
	{
		mesh = *Mesh::Create((name + ".obj").c_str());
		texture = Texture((name + ".bmp").c_str());
	}

public:
	std::string name;
	Mesh mesh;
	Transform transform;
	Texture texture;
	vk::Pipeline pipeline;

	BufferMemory mvpMemoryBuffer;
	BufferMemory lightMemoryBuffer;
	BufferMemory cameraMemoryBuffer;
	ImageMemory sampledImage;
	std::vector<vk::DescriptorSet> descriptorSets;
};