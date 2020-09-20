#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <string>
#include <map>
#include <vector>

#include "instance.hpp"
#include "object.hpp"
#include "shader.hpp"

class Scene
{
public:
	const uint32_t width = 600;
	const uint32_t height = 600;
	const float fps = 60.f;
	bool enableSkybox = false;

	struct Environment
	{
		glm::vec4 color;
		glm::float32_t intensity;
	} environment;

	struct Directional
	{
		glm::vec4 color;
		glm::vec3 direct;
		glm::float32_t intensity;
	} directional;

	struct Camera
	{
		glm::vec3 position;
		glm::vec3 forward;
		glm::float32_t fovy;
		glm::float32_t aspect;
		glm::float32_t near;
		glm::float32_t far;
	} camera;

	vk::Pipeline defaultPipeline;
	vk::Pipeline currentPipeline;
	Texture defaultImage;
	vk::PipelineLayout pipelineLayout;
	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
	std::vector<vk::WriteDescriptorSet> descriptorWrites;

	std::map<std::string, std::shared_ptr<Drawable>> objects;
	std::map<std::string, vk::Pipeline> pipelines;

	Instance instance;
	SDL_Window* window;

public:
	Scene()
	{
		if (0 != SDL_Init(SDL_INIT_VIDEO))
		{
			Log::Error(SDL_GetError());
			return;
		}

		int flag = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;

		window = SDL_CreateWindow(
			"Vulkan Engine",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			width,
			height,
			flag);

		if (nullptr == window)
		{
			Log::Error(SDL_GetError());
			return;
		}

		camera.position = glm::vec3(1.f, 0.f, 1.f);
		camera.forward = glm::vec3(-1.f, 0.f, -1.f);
		camera.fovy = 45.f;
		camera.near = 0.1f;
		camera.far = 100.f;
		camera.aspect = 1.f;
		directional.color = glm::vec4(1.f, 1.f, 1.f, 1.f);
		directional.direct = glm::vec3(-1.f, -1.f, -1.f);
		directional.intensity = 0.8;

		instance.initInstance();
		instance.getPhysicalDevice();
		instance.createSurface(window);
		instance.getQueueFamilyIndex();
		instance.initDevice();
		instance.createCommmandPool();
		instance.initSemaphore();
		instance.getQueue();
		instance.initSwapchain();
		instance.initSwapchainImages();
		instance.allocateCommandBuffers();
		instance.initDepthBuffers();
		instance.initRenderPass();
		instance.initFrameBuffer();

		Shader defaultShader = Shader().Load("default");
		defaultImage = Texture("default.bmp");
		auto vertex = instance.createShaderModule(instance.device, defaultShader.vertex);
		auto fragment = instance.createShaderModule(instance.device, defaultShader.fragment);
		
		auto descriptorSets = instance.createDescriptorSets(instance.device, pipelineLayout, descriptorSetLayouts, 4, 
			vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex,
			vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 
			vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 
			vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
		defaultPipeline = instance.createPipeline(instance.device, vertex, fragment, pipelineLayout);
		pipelines.insert(std::pair<std::string, vk::Pipeline>("default", defaultPipeline));
		descriptorWrites = std::vector<vk::WriteDescriptorSet>(4);
		currentPipeline = defaultPipeline;
	}

	~Scene()
	{
		SDL_DestroyWindow(window);
		SDL_Quit();
		objects.clear();
		pipelines.clear();
	}

	void EnableSkybox()
	{
		enableSkybox = true;
	}

	void DisableSkybox()
	{
		enableSkybox = false;
	}

	void AddShader(std::string shaderName)
	{
		Shader shader = Shader().Load(shaderName);
		auto vertex = instance.createShaderModule(instance.device, shader.vertex);
		auto fragment = instance.createShaderModule(instance.device, shader.fragment);

		auto pipeline = instance.createPipeline(instance.device, vertex, fragment, pipelineLayout);
		pipelines.insert(std::pair<std::string, vk::Pipeline>(shaderName, pipeline));
	}

	void UseShader(std::string shaderName)
	{
		auto res = pipelines.find(shaderName);
		if (res == pipelines.end())
		{
			currentPipeline = defaultPipeline;
		}
		else
		{
			currentPipeline = res->second;
		}
	}

	void AddObject(Drawable* obj)
	{
		obj->pipeline = currentPipeline;
		objects.insert(std::pair<std::string, Drawable*>(obj->name, obj));
	}

	void InitObjects()
	{
		for (auto& item : objects)
		{
			auto& obj = *item.second;

			obj.descriptorSets = instance.createDescriptorSets(instance.device, pipelineLayout, descriptorSetLayouts, 4,
				vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex,
				vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment,
				vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex,
				vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

			obj.sampledImage.sampler = instance.createSampler(instance.device);
			if (obj.texture.pixels.size() == 0)
			{
				instance.createSampledImage(instance.device, obj.sampledImage, vk::Format::eR32G32B32A32Sfloat,
					defaultImage.width, defaultImage.height, defaultImage.pixels.data(), defaultImage.pixels.size() * sizeof(float));
			}
			else
			{
				instance.createSampledImage(instance.device, obj.sampledImage, vk::Format::eR32G32B32A32Sfloat,
					obj.texture.width, obj.texture.height, obj.texture.pixels.data(), obj.texture.pixels.size() * sizeof(float));
			}
			instance.setImageLayout(obj.sampledImage.image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::ePreinitialized,
				vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits(), vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eFragmentShader);
			instance.createUniformBuffer(instance.device, obj.mvpMemoryBuffer, nullptr, sizeof(glm::mat4) * 3);
			instance.createUniformBuffer(instance.device, obj.lightMemoryBuffer, nullptr, sizeof(directional));
			instance.createUniformBuffer(instance.device, obj.cameraMemoryBuffer, nullptr, sizeof(camera));

			instance.pushDescriptor(instance.device, descriptorWrites, 0, obj.descriptorSets[0], obj.mvpMemoryBuffer.buffer, sizeof(glm::mat4) * 3);
			instance.pushDescriptor(instance.device, descriptorWrites, 1, obj.descriptorSets[1], obj.sampledImage.sampler, obj.sampledImage.view);
			instance.pushDescriptor(instance.device, descriptorWrites, 2, obj.descriptorSets[2], obj.lightMemoryBuffer.buffer, sizeof(directional));
			instance.pushDescriptor(instance.device, descriptorWrites, 3, obj.descriptorSets[3], obj.cameraMemoryBuffer.buffer, sizeof(camera));
			instance.writeDescriptor(instance.device, descriptorWrites);
		}

	}

	void Draw()
	{
		if (!instance.prepared)
		{
			InitObjects();
			instance.Prepared();
		}

		for (uint32_t i = 0; i < instance.swapchainImageCount; i++)
		{
			instance.currentBuffer = i;
			auto cmd = instance.getCurrentCommandBuffer();
			std::vector<vk::CommandBuffer> secondarys = std::vector<vk::CommandBuffer>();
			instance.BeginCommandBuffer(cmd);
			for (auto& item : objects)
			{
				auto obj = *item.second;
				auto vertexBuffer = instance.createVertexBuffer(instance.device, obj.mesh.wrapData());
				auto sec = instance.DrawCommandBuffer(instance.device, cmd, obj.pipeline, pipelineLayout, obj.descriptorSets,
					vertexBuffer.second, vertexBuffer.first);
				secondarys.push_back(sec);
			}
			instance.Draw(cmd, secondarys);
		}
	}

	void Present()
	{
		for (const auto& item : objects)
		{
			auto obj = item.second;
			glm::mat4 mvp[3];
			mvp[0] = obj->transform.getModelMatrix();
			mvp[1] = getViewMatrix();
			mvp[2] = getPerpectiveMatrix();

			instance.CopyData(instance.device, obj->mvpMemoryBuffer.memory, mvp, sizeof(glm::mat4) * 3);

			instance.CopyData(instance.device, obj->lightMemoryBuffer.memory, &directional, sizeof(directional));
			instance.CopyData(instance.device, obj->cameraMemoryBuffer.memory, &camera, sizeof(camera));
			
		}
		instance.Present(instance.device);
	}

	glm::mat4 getViewMatrix()
	{
		auto target = glm::normalize(camera.position) + glm::normalize(camera.forward);
		glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);

		return glm::lookAt(camera.position, target, up);
	}

	glm::mat4 getPerpectiveMatrix()
	{
		auto fovy = glm::radians(camera.fovy);
		if (width > height) {
			fovy *= static_cast<float>(height) / static_cast<float>(width);
		}
		return glm::perspective(fovy, camera.aspect, camera.near, camera.far);
	}

	void Loop()
	{
		Drawable skybox = Drawable("skybox");
		if (enableSkybox)
		{
			this->AddShader("skybox");
			skybox.transform.position = camera.position;
			skybox.transform.scale = glm::vec3(49.f, 49.f, 49.f);
			this->UseShader("skybox");
			this->AddObject(&skybox);
		}
		std::shared_ptr<Drawable> player;
		auto playerIter = objects.find("player");
		if (playerIter != objects.end())
		{
			player = playerIter->second;
		}
		float cameraAngle = 225.f;

		Draw();

		uint32_t timePerFrame = (uint32_t)(1000 / fps);
		bool exitFlag = false;
		SDL_Event eventHandle;
		int xmouse = 0;
		int ymouse = 0;
		bool clicked = false;
		bool changed = false;
		while (!exitFlag)
		{
			while (SDL_PollEvent(&eventHandle))
			{
				auto beginTick = SDL_GetTicks();

				switch (eventHandle.type)
				{
				case SDL_QUIT:
					exitFlag = true;
					break;
				case SDL_KEYDOWN:
					switch (eventHandle.key.keysym.scancode)
					{
					case SDL_Scancode::SDL_SCANCODE_UP:
						if (player)
						{
							player->transform.position.x -= 0.1f;
						}
						break;
					case SDL_Scancode::SDL_SCANCODE_DOWN:
						if (player)
						{
							player->transform.position.x += 0.1f;
						}
						break;
					case SDL_Scancode::SDL_SCANCODE_LEFT:
						if (player)
						{
							player->transform.position.z -= 0.1f;
						}
						break;
					case SDL_Scancode::SDL_SCANCODE_RIGHT:
						if (player)
						{
							player->transform.position.x += 0.1f;
						}
						break;
					case SDL_Scancode::SDL_SCANCODE_SPACE:
						if (player)
						{
							player->transform.position = glm::vec3();
						}
						break;
					default:
						break;
					}
					break;
				case SDL_MOUSEMOTION:
					if (clicked & eventHandle.button.button == SDL_BUTTON_LEFT)
					{
						if (eventHandle.button.x - xmouse > 0)	//right move
						{
							cameraAngle -= 1;
						}
						else if(eventHandle.button.x - xmouse < 0)	//left move
						{
							cameraAngle += 1;
						}
						else
						{

						}
						camera.forward = glm::vec3(glm::cos(glm::radians(cameraAngle)), 0.f, glm::sin(glm::radians(cameraAngle)));
						if (eventHandle.button.y - ymouse > 0)	//down move
						{

						}
						else if (eventHandle.button.y - ymouse > 0)	//up move
						{

						}
						else
						{

						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (eventHandle.button.button == SDL_BUTTON_LEFT)
					{
						xmouse = eventHandle.button.x;
						ymouse = eventHandle.button.y;
						clicked = true;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (eventHandle.button.button == SDL_BUTTON_LEFT)
					{
						xmouse = eventHandle.button.x;
						ymouse = eventHandle.button.y;
						clicked = false;
					}
					break;
				case SDL_MOUSEWHEEL:
					break;
				default:
					break;
				}
				//skybox.transform.position = camera.position;
				///Render Begin
				if (changed)
				{
					Draw();
				}

				Present();

				///Render End

				auto endTick = SDL_GetTicks();
				int deltaTick = timePerFrame - (endTick - beginTick);
				if (deltaTick > 0)
				{
					SDL_Delay(deltaTick);
				}
			}
		}
	}
};