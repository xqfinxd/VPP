#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vector>
#include <memory>
#include <cstdarg>
#include "utility.hpp"

class Instance
{
#define FRAME_LAG 2

public:
	Instance()
	{
	}

	void clear()
	{

		for (uint32_t i = 0; i < swapchainImageCount; i++)
		{
			device.destroyFramebuffer(frameBuffers[i]);
			device.destroyImageView(views.swapchain[i]);
			device.destroyImage(images.swapchain[i]);
		}
		device.destroyRenderPass(renderPass);
		device.destroyImageView(views.depth);
		device.destroyImage(images.depth);
		device.freeCommandBuffers(commandPool, 1u, &commandBuffers.base);
		device.freeCommandBuffers(commandPool, swapchainImageCount, commandBuffers.swapchain.data());
		device.destroyCommandPool(commandPool);
		device.destroy();
		instance.destroySurfaceKHR(surface);
		instance.destroy();
	}

	using LayerType = const char*;
	using ExtensionType = const char*;

	void initInstance()
	{
		vk::Result result;

		char const *const desiredLayersRelease[] = { "VK_LAYER_LUNARG_standard_validation" };
		char const *const desiredLayersDebug[] = {
			"VK_LAYER_GOOGLE_threading",
			"VK_LAYER_LUNARG_parameter_validation",
			"VK_LAYER_LUNARG_object_tracker",
			"VK_LAYER_LUNARG_core_validation",
			"VK_LAYER_GOOGLE_unique_objects"
		};

		std::vector<LayerType> enabledLayers = std::vector<LayerType>();
		enabledLayers.push_back("VK_LAYER_LUNARG_standard_validation");

		char const *const desiredExtensions[] = {
			"VK_KHR_surface",
			"VK_KHR_win32_surface"
		};
		std::vector<ExtensionType> enabledExtensions = std::vector<ExtensionType>();
		enabledExtensions.push_back("VK_KHR_surface");
		enabledExtensions.push_back("VK_KHR_win32_surface");

		vk::ApplicationInfo appCI = vk::ApplicationInfo()
			.setPNext(nullptr)
			.setPApplicationName("Vulkan Engine")
			.setApplicationVersion(0)
			.setApiVersion(VK_API_VERSION_1_1)
			.setPEngineName("None")
			.setEngineVersion(0);

		vk::InstanceCreateInfo instCI = vk::InstanceCreateInfo()
			.setPNext(nullptr)
			.setEnabledLayerCount(enabledLayers.size())
			.setPpEnabledLayerNames(enabledLayers.data())
			.setEnabledExtensionCount(enabledExtensions.size())
			.setPpEnabledExtensionNames(enabledExtensions.data())
			.setPApplicationInfo(&appCI);

		result = vk::createInstance(&instCI, nullptr, &instance);
		assert(result == vk::Result::eSuccess);
	}

	void getPhysicalDevice()
	{
		assert(instance);

		vk::Result result;
		uint32_t gpuCount = 0;
		result = instance.enumeratePhysicalDevices(&gpuCount, static_cast<vk::PhysicalDevice*>(nullptr));
		assert(result == vk::Result::eSuccess);
		assert(gpuCount > 0);
		std::vector<vk::PhysicalDevice> availableGPUs(gpuCount);
		result = instance.enumeratePhysicalDevices(&gpuCount, availableGPUs.data());
		assert(result == vk::Result::eSuccess);
		int index = -1;
		for (size_t i = 0; i < availableGPUs.size(); i++)
		{
			const auto& dev = availableGPUs[i];
			auto devProperties = dev.getProperties();
			auto devFeatures = dev.getFeatures();
			if (devProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				gpu = dev;
				index = i;
				break;
			}
		}
		assert(index != -1);
	}

	void createSurface(SDL_Window* window)
	{
		assert(window);
		assert(instance);

		VkInstance cInstance = VkInstance(instance);
		VkSurfaceKHR cSurface;
		auto result = SDL_Vulkan_CreateSurface(window, cInstance, &cSurface);
		assert(result == SDL_TRUE);
		surface = vk::SurfaceKHR(cSurface);
	}

	void getQueueFamilyIndex()
	{
		assert(gpu);
		assert(surface);

		uint32_t queueFamilyCount;
		gpu.getQueueFamilyProperties(&queueFamilyCount, static_cast<vk::QueueFamilyProperties*>(nullptr));
		assert(queueFamilyCount > 0);
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		gpu.getQueueFamilyProperties(&queueFamilyCount, queueFamilyProperties.data());
		std::vector<vk::Bool32> supportsPresent(queueFamilyCount);
		for (size_t i = 0; i < queueFamilyCount; i++)
		{
			gpu.getSurfaceSupportKHR(i, surface, &supportsPresent[i]);
		}
		queueFamilyIndex = UINT32_MAX;
		for (size_t i = 0; i < queueFamilyCount; i++)
		{
			if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && supportsPresent[i])
			{
				if (queueFamilyIndex == UINT32_MAX)
				{
					queueFamilyIndex = i;
				}
			}
		}
		assert(queueFamilyIndex != UINT32_MAX);
	}

	void initDevice()
	{
		assert(gpu);

		vk::Result result;

		std::vector<LayerType> enabledExtensions = std::vector<LayerType>();
		std::vector<ExtensionType> enabledLayers = std::vector<ExtensionType>();

		char const *const desiredExtensions[] = { "VK_KHR_swapchain" };

		enabledExtensions.push_back("VK_KHR_swapchain");

		float const priorities[1] = { 0.0 };
		vk::DeviceQueueCreateInfo queueCI[1];

		queueCI[0].setPNext(nullptr);
		queueCI[0].setPQueuePriorities(priorities);
		queueCI[0].setQueueFamilyIndex(queueFamilyIndex);
		queueCI[0].setQueueCount(1);

		vk::DeviceCreateInfo deviceCI = vk::DeviceCreateInfo()
			.setQueueCreateInfoCount(1)
			.setPQueueCreateInfos(queueCI)
			.setEnabledExtensionCount(enabledExtensions.size())
			.setPpEnabledExtensionNames(enabledExtensions.data())
			.setEnabledLayerCount(0)
			.setPpEnabledLayerNames(nullptr)
			.setPEnabledFeatures(nullptr);
		
		result = gpu.createDevice(&deviceCI, nullptr, &device);
		assert(result == vk::Result::eSuccess);
	}

	void createCommmandPool()
	{
		assert(device);

		vk::Result result;
		auto cmdPoolCI = vk::CommandPoolCreateInfo()
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(queueFamilyIndex);
		result = device.createCommandPool(&cmdPoolCI, nullptr, &commandPool);
		assert(result == vk::Result::eSuccess);
	}

	void allocateCommandBuffers()
	{
		assert(commandPool);
		assert(device);

		vk::Result result;
		auto cmdAI = vk::CommandBufferAllocateInfo()
			.setCommandPool(commandPool)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(1);

		result = device.allocateCommandBuffers(&cmdAI, &commandBuffers.base);
		commandBuffers.swapchain = std::vector<vk::CommandBuffer>(swapchainImageCount);
		for (size_t i = 0; i < swapchainImageCount; i++)
		{
			result = device.allocateCommandBuffers(&cmdAI, &commandBuffers.swapchain[i]);
		}
		assert(result == vk::Result::eSuccess);

		auto cmdBI = vk::CommandBufferBeginInfo()
			.setPInheritanceInfo(nullptr);
		//��ʼ��¼����
		result = commandBuffers.base.begin(&cmdBI);
		assert(result == vk::Result::eSuccess);
	}

	void initSemaphore()
	{
		vk::Result result;
		auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
		auto const fenceCI = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
		for (uint32_t i = 0; i < FRAME_LAG; i++) {
			result = device.createFence(&fenceCI, nullptr, &fences[i]);
			assert(result == vk::Result::eSuccess);

			result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &imageAcquiredSemaphores[i]);
			assert(result == vk::Result::eSuccess);

			result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &drawCompleteSemaphores[i]);
			assert(result == vk::Result::eSuccess);
		}
		frameIndex = 0;
	}

	void getQueue()
	{
		assert(device);
		device.getQueue(queueFamilyIndex, 0, &queue);
		assert(queue);
	}

	void initSwapchain()
	{
		assert(gpu);
		assert(surface);
		assert(device);

		vk::Result result;

		vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

		vk::SurfaceCapabilitiesKHR surfaceCapabilities;
		result = gpu.getSurfaceCapabilitiesKHR(surface, &surfaceCapabilities);
		assert(result == vk::Result::eSuccess);

		uint32_t formatCount;
		result = gpu.getSurfaceFormatsKHR(surface, &formatCount, static_cast<vk::SurfaceFormatKHR*>(nullptr));
		assert(result == vk::Result::eSuccess);
		assert(formatCount > 0);
		std::vector<vk::SurfaceFormatKHR> surfaceFormats(formatCount);
		result = gpu.getSurfaceFormatsKHR(surface, &formatCount, surfaceFormats.data());
		assert(result == vk::Result::eSuccess);
		
		vk::Format format;
		vk::ColorSpaceKHR colorspace;
		format = surfaceFormats[0].format;
		colorspace = surfaceFormats[0].colorSpace;

		vk::Extent2D swapchainExtent;
		if (surfaceCapabilities.currentExtent.width == (uint32_t)-1)
		{
			swapchainExtent = surfaceCapabilities.maxImageExtent;
		}
		else
		{
			swapchainExtent = surfaceCapabilities.currentExtent;
		}
		windowSize = swapchainExtent;
		
		uint32_t desiredSwapchainImageCount = 3;
		if (desiredSwapchainImageCount < surfaceCapabilities.minImageCount)
		{
			desiredSwapchainImageCount = surfaceCapabilities.minImageCount;
		}
		if ((desiredSwapchainImageCount > 0)
			&& (desiredSwapchainImageCount > surfaceCapabilities.maxImageCount))
		{
			desiredSwapchainImageCount = surfaceCapabilities.maxImageCount;
		}

		vk::SurfaceTransformFlagBitsKHR swapchainpreTransform;
		if (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
		{
			swapchainpreTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		}
		else
		{
			swapchainpreTransform = surfaceCapabilities.currentTransform;
		}
		
		vk::CompositeAlphaFlagBitsKHR swapchainCompositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		vk::CompositeAlphaFlagBitsKHR allCompositeAlphaFlags[4] = {
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
			vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
			vk::CompositeAlphaFlagBitsKHR::eInherit,
		};
		for (size_t i = 0; i < 4; i++)
		{
			if (surfaceCapabilities.supportedCompositeAlpha & allCompositeAlphaFlags[i])
			{
				swapchainCompositeAlpha = allCompositeAlphaFlags[i];
				break;
			}
		}

		vk::SwapchainCreateInfoKHR swapchainCI = vk::SwapchainCreateInfoKHR()
			.setPNext(nullptr)
			.setMinImageCount(desiredSwapchainImageCount)
			.setImageFormat(format)
			.setImageColorSpace(colorspace)
			.setImageExtent({ swapchainExtent.width, swapchainExtent.height })
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setPreTransform(swapchainpreTransform)
			.setCompositeAlpha(swapchainCompositeAlpha)
			.setPresentMode(presentMode)
			.setClipped(true)
			.setSurface(surface);
		uint32_t queueFamilyIndices[1] = { queueFamilyIndex };
		swapchainCI.setQueueFamilyIndexCount(1);
		swapchainCI.setPQueueFamilyIndices(queueFamilyIndices);

		result = device.createSwapchainKHR(&swapchainCI, nullptr, &swapchain);
		assert(result == vk::Result::eSuccess);
	}

	void initSwapchainImages()
	{
		assert(device);
		assert(swapchain);
		assert(surface);

		vk::Result result;

		result = device.getSwapchainImagesKHR(swapchain, &swapchainImageCount, static_cast<vk::Image*>(nullptr));
		assert(result == vk::Result::eSuccess);
		assert(swapchainImageCount > 0);

		images.swapchain = std::vector<vk::Image>(swapchainImageCount);
		views.swapchain = std::vector<vk::ImageView>(swapchainImageCount);

		result = device.getSwapchainImagesKHR(swapchain, &swapchainImageCount, images.swapchain.data());
		assert(result == vk::Result::eSuccess);

		uint32_t formatCount;
		result = gpu.getSurfaceFormatsKHR(surface, &formatCount, static_cast<vk::SurfaceFormatKHR*>(nullptr));
		assert(result == vk::Result::eSuccess);
		assert(formatCount > 0);
		std::vector<vk::SurfaceFormatKHR> surfaceFormats(formatCount);
		result = gpu.getSurfaceFormatsKHR(surface, &formatCount, surfaceFormats.data());
		assert(result == vk::Result::eSuccess);

		for (size_t i = 0; i < swapchainImageCount; i++)
		{
			vk::ImageViewCreateInfo imageViewCI = vk::ImageViewCreateInfo()
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(surfaceFormats[0].format)
				.setPNext(nullptr)
				.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
			imageViewCI.setImage(images.swapchain[i]);
			result = device.createImageView(&imageViewCI, nullptr, &views.swapchain[i]);
			assert(result == vk::Result::eSuccess);
		}
	}

	void setImageLayout(vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
		vk::AccessFlags srcAccessMask, vk::PipelineStageFlags src_stages, vk::PipelineStageFlags dest_stages) {
		assert(commandBuffers.base);

		auto DstAccessMask = [](vk::ImageLayout const &layout) {
			vk::AccessFlags flags;

			switch (layout) {
			case vk::ImageLayout::eTransferDstOptimal:
				// Make sure anything that was copying from this image has
				// completed
				flags = vk::AccessFlagBits::eTransferWrite;
				break;
			case vk::ImageLayout::eColorAttachmentOptimal:
				flags = vk::AccessFlagBits::eColorAttachmentWrite;
				break;
			case vk::ImageLayout::eDepthStencilAttachmentOptimal:
				flags = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				break;
			case vk::ImageLayout::eShaderReadOnlyOptimal:
				// Make sure any Copy or CPU writes to image are flushed
				flags = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
				break;
			case vk::ImageLayout::eTransferSrcOptimal:
				flags = vk::AccessFlagBits::eTransferRead;
				break;
			case vk::ImageLayout::ePresentSrcKHR:
				flags = vk::AccessFlagBits::eMemoryRead;
				break;
			default:
				break;
			}

			return flags;
		};

		auto const barrier = vk::ImageMemoryBarrier()
			.setSrcAccessMask(srcAccessMask)
			.setDstAccessMask(DstAccessMask(newLayout))
			.setOldLayout(oldLayout)
			.setNewLayout(newLayout)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(image)
			.setSubresourceRange(vk::ImageSubresourceRange(aspectMask, 0, 1, 0, 1));

		commandBuffers.base.pipelineBarrier(src_stages, dest_stages, vk::DependencyFlagBits(), 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void createSampledImage(vk::Device& device, ImageMemory& imageMemory, 
		vk::Format format, uint32_t width, uint32_t height, void* pData, uint32_t size)
	{	
		auto imageCI = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D)
			.setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setExtent({ width, height, 1 })
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eLinear)
			.setUsage(vk::ImageUsageFlagBits::eSampled)
			.setSharingMode(vk::SharingMode::eExclusive)
			.setQueueFamilyIndexCount(0)
			.setPQueueFamilyIndices(nullptr)
			.setInitialLayout(vk::ImageLayout::ePreinitialized);

		auto result = device.createImage(&imageCI, nullptr, &imageMemory.image);
		assert(result == vk::Result::eSuccess);

		vk::MemoryRequirements req;
		device.getImageMemoryRequirements(imageMemory.image, &req);

		vk::MemoryAllocateInfo memoryAI = vk::MemoryAllocateInfo();
		memoryAI.setAllocationSize(req.size);
		memoryAI.setMemoryTypeIndex(0);
		auto memType = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		auto pass = GetPhysicalMemoryType(gpu, req, memType, memoryAI.memoryTypeIndex);
		assert(pass == true);
		result = device.allocateMemory(&memoryAI, nullptr, &imageMemory.memory);
		assert(result == vk::Result::eSuccess);

		device.bindImageMemory(imageMemory.image, imageMemory.memory, 0);

		if (memType & vk::MemoryPropertyFlagBits::eHostVisible) {
			auto subres = vk::ImageSubresource()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(0)
				.setArrayLayer(0);
			vk::SubresourceLayout layout;
			device.getImageSubresourceLayout(imageMemory.image, &subres, &layout);

			auto ptr = device.mapMemory(imageMemory.memory, 0, req.size);
			assert(ptr != nullptr);
			memcpy(ptr, pData, size);
			device.unmapMemory(imageMemory.memory);
		}
		auto const imageViewCI = vk::ImageViewCreateInfo()
			.setImage(imageMemory.image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

		result = device.createImageView(&imageViewCI, nullptr, &imageMemory.view);
		assert(result == vk::Result::eSuccess);
	}

	void createUniformBuffer(vk::Device& device, BufferMemory& bufferMemory, void* pData, uint32_t size)
	{
		auto bufferCI = vk::BufferCreateInfo()
			.setSize(size)
			.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

		auto result = device.createBuffer(&bufferCI, nullptr, &bufferMemory.buffer);
		assert(result == vk::Result::eSuccess);

		vk::MemoryRequirements memReq;
		device.getBufferMemoryRequirements(bufferMemory.buffer, &memReq);

		auto memoryAllocateInfo = vk::MemoryAllocateInfo()
			.setAllocationSize(memReq.size)
			.setMemoryTypeIndex(0);

		bool pass = GetPhysicalMemoryType(gpu, memReq,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			memoryAllocateInfo.memoryTypeIndex);
		assert(pass);

		result = device.allocateMemory(&memoryAllocateInfo, nullptr, &bufferMemory.memory);
		assert(result == vk::Result::eSuccess);
		if (pData != nullptr)
		{
			void* ptr = nullptr;
			result = device.mapMemory(bufferMemory.memory, 0, memReq.size, vk::MemoryMapFlags(), &ptr);
			assert(result == vk::Result::eSuccess);

			memcpy(ptr, &pData, size);
			device.unmapMemory(bufferMemory.memory);

		}
		device.bindBufferMemory(bufferMemory.buffer, bufferMemory.memory, 0);
	}

	void initDepthBuffers()
	{
		assert(gpu);
		assert(device);

		vk::Result result;
		vk::DeviceMemory memory;

		auto imageCI = vk::ImageCreateInfo()
			.setFormat(vk::Format::eD16Unorm)
			.setImageType(vk::ImageType::e2D)
			.setExtent(vk::Extent3D(windowSize, 1u))
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
			.setSharingMode(vk::SharingMode::eExclusive)
			.setQueueFamilyIndexCount(0)
			.setPQueueFamilyIndices(nullptr)
			.setInitialLayout(vk::ImageLayout::eUndefined);
		result = device.createImage(&imageCI, nullptr, &images.depth);
		assert(result == vk::Result::eSuccess);

		vk::MemoryAllocateInfo memoryAI;
		vk::MemoryRequirements memReq;
		device.getImageMemoryRequirements(images.depth, &memReq);
		memoryAI.setAllocationSize(memReq.size);
		auto pass = GetPhysicalMemoryType(gpu, memReq, vk::MemoryPropertyFlagBits::eDeviceLocal, memoryAI.memoryTypeIndex);
		assert(pass);
		result = device.allocateMemory(&memoryAI, nullptr, &memory);
		assert(result == vk::Result::eSuccess);

		device.bindImageMemory(images.depth, memory, 0);
		auto imageViewCI = vk::ImageViewCreateInfo()
			.setImage(images.depth)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(vk::Format::eD16Unorm)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
			.setPNext(nullptr);
		result = device.createImageView(&imageViewCI, nullptr, &views.depth);
		assert(result == vk::Result::eSuccess);
	}

	void initRenderPass()
	{
		assert(device);

		vk::Result result;

		uint32_t formatCount;
		result = gpu.getSurfaceFormatsKHR(surface, &formatCount, static_cast<vk::SurfaceFormatKHR*>(nullptr));
		assert(result == vk::Result::eSuccess);
		assert(formatCount > 0);
		std::vector<vk::SurfaceFormatKHR> surfaceFormats(formatCount);
		result = gpu.getSurfaceFormatsKHR(surface, &formatCount, surfaceFormats.data());
		assert(result == vk::Result::eSuccess);

		vk::AttachmentDescription attachments[2] = {
			vk::AttachmentDescription()
				.setFormat(surfaceFormats[0].format)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eStore)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
			vk::AttachmentDescription()
				.setFormat(vk::Format::eD16Unorm)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal) };

		auto colorReference = vk::AttachmentReference()
			.setAttachment(0)
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

		auto depthReference =
			vk::AttachmentReference()
			.setAttachment(1)
			.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

		auto subpass = vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setInputAttachmentCount(0)
			.setPInputAttachments(nullptr)
			.setColorAttachmentCount(1)
			.setPColorAttachments(&colorReference)
			.setPResolveAttachments(nullptr)
			.setPDepthStencilAttachment(&depthReference)
			.setPreserveAttachmentCount(0)
			.setPPreserveAttachments(nullptr);

		auto renderPassCI = vk::RenderPassCreateInfo()
			.setAttachmentCount(2)
			.setPAttachments(attachments)
			.setSubpassCount(1)
			.setPSubpasses(&subpass)
			.setDependencyCount(0)
			.setPDependencies(nullptr);

		result = device.createRenderPass(&renderPassCI, nullptr, &renderPass);
		assert(result == vk::Result::eSuccess);
	}

	void initFrameBuffer()
	{
		assert(device);
		assert(swapchain);

		frameBuffers = std::vector<vk::Framebuffer>(swapchainImageCount);

		vk::ImageView attachments[2];
		attachments[1] = views.depth;

		auto frameBufferCI = vk::FramebufferCreateInfo()
			.setRenderPass(renderPass)
			.setAttachmentCount(2)
			.setPAttachments(attachments)
			.setWidth((uint32_t)windowSize.width)
			.setHeight((uint32_t)windowSize.height)
			.setLayers(1);

		for (uint32_t i = 0; i < swapchainImageCount; i++) {
			attachments[0] = views.swapchain[i];
			auto result = device.createFramebuffer(&frameBufferCI, nullptr, &frameBuffers[i]);
			assert(result == vk::Result::eSuccess);
		}
	}

	void Prepared()
	{
		commandBuffers.base.end();
		auto fenceInfo = vk::FenceCreateInfo();
		vk::Fence fence;
		auto result = device.createFence(&fenceInfo, nullptr, &fence);
		assert(result == vk::Result::eSuccess);

		vk::CommandBuffer cmds[] = { commandBuffers.base };
		auto submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(cmds);

		result = queue.submit(1, &submitInfo, fence);
		assert(result == vk::Result::eSuccess);

		result = device.waitForFences(1, &fence, VK_TRUE, UINT64_MAX);
		assert(result == vk::Result::eSuccess);

		device.destroyFence(fence, nullptr);
		device.freeCommandBuffers(commandPool, commandBuffers.base);
		commandBuffers.base = vk::CommandBuffer();
		prepared = true;
	}

	std::vector<vk::DescriptorSet> createDescriptorSets(vk::Device& device, 
		vk::PipelineLayout& pipelineLayout, 
		std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
		uint32_t descCount, ...)
	{
		//0
		vk::DescriptorPool descriptorPool;
		std::vector<vk::DescriptorSet> descriptorSets;

		//1
		va_list vlist;
		va_start(vlist, descCount);
		std::vector< vk::DescriptorSetLayoutBinding> layoutBindings = std::vector< vk::DescriptorSetLayoutBinding>();

		for (uint32_t i = 0; i < descCount; i++)
		{
			auto descType = va_arg(vlist, vk::DescriptorType);
			auto shaderStage = va_arg(vlist, vk::ShaderStageFlagBits);
			auto descBinding = vk::DescriptorSetLayoutBinding()
				.setBinding(0)
				.setDescriptorType(descType)
				.setDescriptorCount(1)
				.setStageFlags(shaderStage)
				.setPImmutableSamplers(nullptr);
			layoutBindings.push_back(descBinding);
		}

		va_end(vlist);

		descriptorSetLayouts = std::vector<vk::DescriptorSetLayout>(descCount);
		for (size_t i = 0; i < descCount; i++)
		{
			auto descriptorLayoutCI = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(1)
				.setPBindings(&layoutBindings[i]);
			auto result = device.createDescriptorSetLayout(&descriptorLayoutCI, nullptr, &descriptorSetLayouts[i]);
			assert(result == vk::Result::eSuccess);
		}

		auto pipelineLayoutCI = vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(descriptorSetLayouts.size())
			.setPSetLayouts(descriptorSetLayouts.data());

		auto result = device.createPipelineLayout(&pipelineLayoutCI, nullptr, &pipelineLayout);
		assert(result == vk::Result::eSuccess);

		//2
		std::vector<vk::DescriptorPoolSize> poolSizes = std::vector<vk::DescriptorPoolSize>();
		for (uint32_t i = 0; i < descCount; i++)
		{
			auto poolSize = vk::DescriptorPoolSize()
				.setType(layoutBindings[i].descriptorType)
				.setDescriptorCount(1);
			poolSizes.push_back(poolSize);
		}

		auto descriptorPoolCI = vk::DescriptorPoolCreateInfo()
			.setMaxSets(descCount)
			.setPoolSizeCount(poolSizes.size())
			.setPPoolSizes(poolSizes.data());

		result = device.createDescriptorPool(&descriptorPoolCI, nullptr, &descriptorPool);
		assert(result == vk::Result::eSuccess);

		//3
		descriptorSets = std::vector<vk::DescriptorSet>(descCount);
		auto descriptorSetAI = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descriptorPool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&descriptorSetLayouts[0]);

		result = device.allocateDescriptorSets(&descriptorSetAI, &descriptorSets[0]);
		assert(result == vk::Result::eSuccess);

		descriptorSetAI = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descriptorPool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&descriptorSetLayouts[1]);

		result = device.allocateDescriptorSets(&descriptorSetAI, &descriptorSets[1]);
		assert(result == vk::Result::eSuccess);

		descriptorSetAI = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descriptorPool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&descriptorSetLayouts[2]);

		result = device.allocateDescriptorSets(&descriptorSetAI, &descriptorSets[2]);
		assert(result == vk::Result::eSuccess);

		descriptorSetAI = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descriptorPool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&descriptorSetLayouts[3]);

		result = device.allocateDescriptorSets(&descriptorSetAI, &descriptorSets[3]);
		assert(result == vk::Result::eSuccess);

		return  descriptorSets;
	}
	vk::Sampler createSampler(vk::Device& device)
	{
		vk::Sampler sampler;

		vk::SamplerCreateInfo samplerInfo = vk::SamplerCreateInfo()
			.setMagFilter(vk::Filter::eNearest)
			.setMinFilter(vk::Filter::eNearest)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
			.setMipLodBias(0.0f)
			.setAnisotropyEnable(VK_FALSE)
			.setMaxAnisotropy(1)
			.setCompareEnable(VK_FALSE)
			.setCompareOp(vk::CompareOp::eNever)
			.setMinLod(0.0f)
			.setMaxLod(0.0f)
			.setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
			.setUnnormalizedCoordinates(VK_FALSE);
		auto result = device.createSampler(&samplerInfo, nullptr, &sampler);
		assert(result == vk::Result::eSuccess);
		return sampler;
	}

	void pushDescriptor(vk::Device& device, std::vector<vk::WriteDescriptorSet>& writes, uint32_t index, vk::DescriptorSet& descSet,vk::Buffer& buffer, uint32_t size)
	{
		auto descriptorBI = new vk::DescriptorBufferInfo();
		descriptorBI->setOffset(0);
		descriptorBI->setBuffer(buffer);
		descriptorBI->setRange(size);
		vk::WriteDescriptorSet write;
		write.setDstBinding(0);
		write.setDescriptorCount(1);
		write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
		write.setPBufferInfo(descriptorBI);
		write.setDstSet(descSet);
		writes[index] = (write);
	}
	void pushDescriptor(vk::Device& device, std::vector<vk::WriteDescriptorSet>& writes, uint32_t index, vk::DescriptorSet& descSet, vk::Sampler& sampler, vk::ImageView& view)
	{
		auto descriptorII = new vk::DescriptorImageInfo();
		descriptorII->setSampler(sampler);
		descriptorII->setImageView(view);
		descriptorII->setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		vk::WriteDescriptorSet write;
		write.setDstBinding(0);
		write.setDescriptorCount(1);
		write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
		write.setPImageInfo(descriptorII);
		write.setDstSet(descSet);
		writes[index] = (write);
	}
	void writeDescriptor(vk::Device& device, std::vector<vk::WriteDescriptorSet>& writes)
	{
		device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
		for (auto& item : writes)
		{
			switch (item.descriptorType)
			{
			case vk::DescriptorType::eCombinedImageSampler:
				delete item.pImageInfo;
				break;
			case vk::DescriptorType::eUniformBuffer:
				delete item.pBufferInfo;
				break;
			default:
				break;
			}
		}
	}
	vk::ShaderModule createShaderModule(vk::Device& device, const std::vector<uint32_t>& code)
	{
		vk::ShaderModule module;
		auto shaderModuleCI = vk::ShaderModuleCreateInfo()
			.setCodeSize(code.size() * sizeof(uint32_t))
			.setPCode(code.data());
		auto result = device.createShaderModule(&shaderModuleCI, nullptr, &module);
		assert(result == vk::Result::eSuccess);
		return module;
	}

	void CopyData(vk::Device& device, vk::DeviceMemory& memory, void* pData, uint32_t size)
	{
		auto ptr = device.mapMemory(memory, 0, size);
		assert(ptr != nullptr);
		memcpy(ptr, pData, size);
		device.unmapMemory(memory);
	}

	vk::CommandBuffer getCurrentCommandBuffer()
	{
		return commandBuffers.swapchain[currentBuffer];
	}

	vk::Pipeline createPipeline(vk::Device& device, vk::ShaderModule& vertex, vk::ShaderModule& fragment, vk::PipelineLayout& pipelineLayout)
	{
		vk::Pipeline pipeline;
		vk::PipelineCache cache;

		vk::PipelineCacheCreateInfo pipelineCacheInfo;
		auto result = device.createPipelineCache(&pipelineCacheInfo, nullptr, &cache);
		assert(result == vk::Result::eSuccess);

		vk::PipelineShaderStageCreateInfo shaderStageInfo[2] = {
		vk::PipelineShaderStageCreateInfo().setStage(vk::ShaderStageFlagBits::eVertex).setModule(vertex).setPName("main"),
		vk::PipelineShaderStageCreateInfo().setStage(vk::ShaderStageFlagBits::eFragment).setModule(fragment).setPName("main") };

		vk::VertexInputBindingDescription bindingDesc[1] = {
		vk::VertexInputBindingDescription()
		.setBinding(0)
		.setInputRate(vk::VertexInputRate::eVertex)
		.setStride(sizeof(float) * (3 + 3 + 2)),
		};
		vk::VertexInputAttributeDescription attributeDesc[3] = {
			vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(0)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(0),
			vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(1)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(3 * sizeof(float)),
			vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(2)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setOffset((3 + 3) * sizeof(float))
		};
		auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
			.setVertexAttributeDescriptionCount(3)
			.setPVertexAttributeDescriptions(attributeDesc)
			.setVertexBindingDescriptionCount(1)
			.setPVertexBindingDescriptions(bindingDesc);

		auto inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology::eTriangleList);

		auto viewportInfo = vk::PipelineViewportStateCreateInfo()
			.setViewportCount(1).setScissorCount(1);

		auto rasterizationInfo = vk::PipelineRasterizationStateCreateInfo()
			.setDepthClampEnable(VK_FALSE)
			.setRasterizerDiscardEnable(VK_FALSE)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setDepthBiasEnable(VK_FALSE)
			.setLineWidth(1.f);
		auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo();

		auto stencilOp =
			vk::StencilOpState().setFailOp(vk::StencilOp::eKeep).setPassOp(vk::StencilOp::eKeep).setCompareOp(vk::CompareOp::eAlways);

		auto depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(VK_TRUE)
			.setDepthWriteEnable(VK_TRUE)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
			.setDepthBoundsTestEnable(VK_FALSE)
			.setStencilTestEnable(VK_FALSE)
			.setFront(stencilOp)
			.setBack(stencilOp);

		vk::PipelineColorBlendAttachmentState const colorBlendAttachments[1] = {
			vk::PipelineColorBlendAttachmentState().setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
																	  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA) };

		auto colorBlendInfo =
			vk::PipelineColorBlendStateCreateInfo().setAttachmentCount(1).setPAttachments(colorBlendAttachments);

		vk::DynamicState dynamicStates[2] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		auto dynamicStateInfo = vk::PipelineDynamicStateCreateInfo().setPDynamicStates(dynamicStates).setDynamicStateCount(2);

		auto pipelineCI = vk::GraphicsPipelineCreateInfo()
			.setStageCount(2)
			.setPStages(shaderStageInfo)
			.setPVertexInputState(&vertexInputInfo)
			.setPInputAssemblyState(&inputAssemblyInfo)
			.setPViewportState(&viewportInfo)
			.setPRasterizationState(&rasterizationInfo)
			.setPMultisampleState(&multisampleInfo)
			.setPDepthStencilState(&depthStencilInfo)
			.setPColorBlendState(&colorBlendInfo)
			.setPDynamicState(&dynamicStateInfo)
			.setLayout(pipelineLayout)
			.setRenderPass(renderPass);
		result = device.createGraphicsPipelines(cache, 1, &pipelineCI, nullptr, &pipeline);
		assert(result == vk::Result::eSuccess);

		return pipeline;
	}

	std::pair<uint32_t, vk::Buffer> createVertexBuffer(vk::Device& device, std::vector<float>& mesh)
	{
		vk::Buffer vertexBuffer;
		vk::DeviceMemory memory;
		uint32_t vertexCount;

		auto bufferCI = vk::BufferCreateInfo()
			.setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
			.setQueueFamilyIndexCount(0)
			.setPQueueFamilyIndices(nullptr)
			.setSharingMode(vk::SharingMode::eExclusive)
			.setSize(mesh.size() * sizeof(float));
		auto result = device.createBuffer(&bufferCI, nullptr, &vertexBuffer);
		assert(result == vk::Result::eSuccess);
		vk::MemoryRequirements req;
		req = device.getBufferMemoryRequirements(vertexBuffer);
		auto allocateInfo = vk::MemoryAllocateInfo()
			.setAllocationSize(req.size);

		vk::MemoryPropertyFlags memFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		auto ret = GetPhysicalMemoryType(gpu, req, memFlags, allocateInfo.memoryTypeIndex);
		assert(ret);
		result = device.allocateMemory(&allocateInfo, nullptr, &memory);
		assert(result == vk::Result::eSuccess);
		void* pdata = nullptr;
		result = device.mapMemory(memory, 0, req.size, vk::MemoryMapFlags(), &pdata);
		assert(result == vk::Result::eSuccess);
		memcpy(pdata, mesh.data(), mesh.size() * sizeof(float));
		device.unmapMemory(memory);
		device.bindBufferMemory(vertexBuffer, memory, 0);
		vertexCount = mesh.size() / (3 + 3 + 2);
		return std::pair<uint32_t, vk::Buffer>(vertexCount, vertexBuffer);
	}

	void BeginCommandBuffer(vk::CommandBuffer& commandBuffer)
	{
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo();
		commandBuffer.begin(beginInfo);
	}

	void EndCommandBuffer(vk::CommandBuffer& commandBuffer)
	{
		commandBuffer.end();
	}

	vk::CommandBuffer DrawCommandBuffer(vk::Device& device, vk::CommandBuffer cmd,
		vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, 
		std::vector<vk::DescriptorSet> descriptorSets, vk::Buffer vertexBuffer, uint32_t vertexCount)
	{
		vk::CommandBuffer secondary;
		vk::CommandBufferAllocateInfo commandBufferAI = vk::CommandBufferAllocateInfo()
			.setCommandBufferCount(1)
			.setLevel(vk::CommandBufferLevel::eSecondary)
			.setCommandPool(commandPool);
		device.allocateCommandBuffers(&commandBufferAI, &secondary);
		vk::CommandBufferInheritanceInfo inheritanceInfo = vk::CommandBufferInheritanceInfo()
			.setFramebuffer(frameBuffers[currentBuffer])
			.setRenderPass(renderPass)
			.setOcclusionQueryEnable(VK_FALSE)
			.setSubpass(0);
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue)
			.setPInheritanceInfo(&inheritanceInfo);
		secondary.begin(beginInfo);
		secondary.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		secondary.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets.size(),
			descriptorSets.data(), 0, nullptr);
		const vk::DeviceSize offset[1] = { 0 };
		secondary.bindVertexBuffers(0, 1, &vertexBuffer, offset);
		auto viewport = vk::Viewport()
			.setWidth((float)windowSize.width)
			.setHeight((float)windowSize.height)
			.setMinDepth((float)0.0f)
			.setMaxDepth((float)1.0f);
		secondary.setViewport(0, 1, &viewport);

		vk::Rect2D const scissor(vk::Offset2D(0, 0), vk::Extent2D(windowSize.width, windowSize.height));
		secondary.setScissor(0, 1, &scissor);
		secondary.draw(vertexCount, 1, 0, 0);
		secondary.end();
		return secondary;
	}

	void Draw(vk::CommandBuffer& commandBuffer, std::vector<vk::CommandBuffer>& cmds)
	{
		vk::ClearValue const clearValues[2] = {
			vk::ClearColorValue(std::array<float, 4>({{0.f, 0.f, 0.f, 1.f}})),
			vk::ClearDepthStencilValue(1.0f, 0u)
		};

		auto const renderPassInfo = vk::RenderPassBeginInfo()
			.setRenderPass(renderPass)
			.setFramebuffer(frameBuffers[currentBuffer])
			.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D((uint32_t)windowSize.width, (uint32_t)windowSize.width)))
			.setClearValueCount(2)
			.setPClearValues(clearValues);
		commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eSecondaryCommandBuffers);
		commandBuffer.executeCommands(cmds.size(), cmds.data());
		commandBuffer.endRenderPass();
		commandBuffer.end();
	}

	void Present(vk::Device& device)
	{
		device.waitForFences(1, &fences[frameIndex], VK_TRUE, UINT64_MAX);
		device.resetFences(1, &fences[frameIndex]);

		vk::Result result;
		do {
			result =
				device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAcquiredSemaphores[frameIndex], vk::Fence(), &currentBuffer);
			if (result == vk::Result::eErrorOutOfDateKHR) {
				Log::Error("swapchain need to resize");
			}
			else if (result == vk::Result::eSuboptimalKHR) {
				break;
			}
			else {
				assert(result == vk::Result::eSuccess);
			}
		} while (result != vk::Result::eSuccess);

		//UpdateDataBuffer(obj);

		vk::PipelineStageFlags pipeStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		auto const submitInfo = vk::SubmitInfo()
			.setPWaitDstStageMask(&pipeStageFlags)
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&imageAcquiredSemaphores[frameIndex])
			.setCommandBufferCount(1)
			.setPCommandBuffers(&commandBuffers.swapchain[currentBuffer])
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&drawCompleteSemaphores[frameIndex]);

		result = queue.submit(1, &submitInfo, fences[frameIndex]);
		assert(result == vk::Result::eSuccess);

		// If we are using separate queues we have to wait for image ownership,
		// otherwise wait for draw complete
		auto const presentInfo = vk::PresentInfoKHR()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&drawCompleteSemaphores[frameIndex])
			.setSwapchainCount(1)
			.setPSwapchains(&swapchain)
			.setPImageIndices(&currentBuffer);

		result = queue.presentKHR(&presentInfo);
		frameIndex += 1;
		frameIndex %= FRAME_LAG;
		if (result == vk::Result::eErrorOutOfDateKHR) {
			// swapchain is out of date (e.g. the window was resized) and
			// must be recreated:
			//Resize();
			Log::Error("swapchain need to resize");
		}
		else if (result == vk::Result::eSuboptimalKHR) {
			// swapchain is not as optimal as it could be, but the platform's
			// presentation engine will still present the image correctly.
		}
		else {
			assert(result == vk::Result::eSuccess);
		}
	}
public:
	vk::Instance instance;
	SDL_Window* window;
	vk::PhysicalDevice gpu;
	vk::SurfaceKHR surface;
	uint32_t queueFamilyIndex;
	vk::Device device;
	vk::Queue queue;
	vk::CommandPool commandPool;
	struct CommandBuffers
	{
		vk::CommandBuffer base;
		std::vector<vk::CommandBuffer> swapchain;
	} commandBuffers;
	vk::SwapchainKHR swapchain;
	uint32_t swapchainImageCount;
	vk::Extent2D windowSize;
	vk::RenderPass renderPass;
	vk::Buffer depthBuffer;
	std::vector<vk::Framebuffer> frameBuffers;
	struct Images
	{
		std::vector<vk::Image> swapchain;
		vk::Image depth;
	} images;
	struct Views
	{
		std::vector<vk::ImageView> swapchain;
		vk::ImageView depth;
	} views;

	bool prepared = false;
	uint32_t frameIndex;
	uint32_t currentBuffer;

	vk::Fence fences[FRAME_LAG];
	vk::Semaphore imageAcquiredSemaphores[FRAME_LAG];
	vk::Semaphore drawCompleteSemaphores[FRAME_LAG];
};