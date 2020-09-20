#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>

class Log
{
public:
	template<typename T>
	static void Error(const T text)
	{
		std::cout << "ERROR: " << text << "!" << std::endl;
	}

	template<>
	static void Error(const int code)
	{
		std::cout << "ERROR CODE: " << code << std::endl;
	}

	template<typename T>
	static void Info(const char* description, const T text)
	{
		std::cout << "INFO(" << description << "): " << text << "." << std::endl;
	}
};

class ShaderUtil
{
public:
	static std::vector<uint32_t> Create(const char* filename, vk::ShaderStageFlagBits type)
	{
		FILE* input = fopen(filename, "rb");
		assert(input != nullptr);
		fseek(input, 0, SEEK_END);
		long fileSize = ftell(input);
		assert(fileSize > 0);
		fseek(input, 0, SEEK_SET);
		std::vector<char> content(fileSize + 1);
		fread(content.data(), fileSize, sizeof(char), input);
		content[fileSize] = '\0';
		fclose(input);
		//Log::Info("filename:", content.data());
		InitGlslang();
		std::vector<uint32_t> result;
		if (GLSLtoSPV(type, content.data(), result))
		{
			FinalizeGlslang();
			content.clear();
			return result;
		}
		else
		{
			FinalizeGlslang();
			content.clear();
			return std::vector<uint32_t>();
		}
	}

private:
	static bool GLSLtoSPV(const vk::ShaderStageFlagBits shaderType, const char *pShader, std::vector<uint32_t> &spirv)
	{
		using namespace glslang;
		EShLanguage stage = MapLanguage(shaderType);
		glslang::TShader shader(stage);
		glslang::TProgram program;
		const char *shaderStrings[1];
		TBuiltInResource Resources;
		InitResources(Resources);

		// Enable SPIR-V and Vulkan rules when parsing GLSL
		EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

		shaderStrings[0] = pShader;
		shader.setStrings(shaderStrings, 1);
		if (!shader.parse(&Resources, 140, false, messages)) {
			Log::Error(shader.getInfoLog());
			Log::Error(shader.getInfoDebugLog());
			return false;  // something didn't work
		}
		program.addShader(&shader);

		if (!program.link(messages)) {
			Log::Error(shader.getInfoLog());
			Log::Error(shader.getInfoDebugLog());
			return false;
		}

		glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
		return true;
	}

	static EShLanguage MapLanguage(const vk::ShaderStageFlagBits shaderType) {
		using ShaderType = vk::ShaderStageFlagBits;
		switch (shaderType) {
		case ShaderType::eVertex:
			return EShLangVertex;

		case ShaderType::eTessellationControl:
			return EShLangTessControl;

		case ShaderType::eTessellationEvaluation:
			return EShLangTessEvaluation;

		case ShaderType::eGeometry:
			return EShLangGeometry;

		case ShaderType::eFragment:
			return EShLangFragment;

		case ShaderType::eCompute:
			return EShLangCompute;

		default:
			return EShLangVertex;
		}
	}

	static void InitGlslang()
	{
		glslang::InitializeProcess();
	}

	static void InitResources(TBuiltInResource &Resources) {
		Resources.maxLights = 32;
		Resources.maxClipPlanes = 6;
		Resources.maxTextureUnits = 32;
		Resources.maxTextureCoords = 32;
		Resources.maxVertexAttribs = 64;
		Resources.maxVertexUniformComponents = 4096;
		Resources.maxVaryingFloats = 64;
		Resources.maxVertexTextureImageUnits = 32;
		Resources.maxCombinedTextureImageUnits = 80;
		Resources.maxTextureImageUnits = 32;
		Resources.maxFragmentUniformComponents = 4096;
		Resources.maxDrawBuffers = 32;
		Resources.maxVertexUniformVectors = 128;
		Resources.maxVaryingVectors = 8;
		Resources.maxFragmentUniformVectors = 16;
		Resources.maxVertexOutputVectors = 16;
		Resources.maxFragmentInputVectors = 15;
		Resources.minProgramTexelOffset = -8;
		Resources.maxProgramTexelOffset = 7;
		Resources.maxClipDistances = 8;
		Resources.maxComputeWorkGroupCountX = 65535;
		Resources.maxComputeWorkGroupCountY = 65535;
		Resources.maxComputeWorkGroupCountZ = 65535;
		Resources.maxComputeWorkGroupSizeX = 1024;
		Resources.maxComputeWorkGroupSizeY = 1024;
		Resources.maxComputeWorkGroupSizeZ = 64;
		Resources.maxComputeUniformComponents = 1024;
		Resources.maxComputeTextureImageUnits = 16;
		Resources.maxComputeImageUniforms = 8;
		Resources.maxComputeAtomicCounters = 8;
		Resources.maxComputeAtomicCounterBuffers = 1;
		Resources.maxVaryingComponents = 60;
		Resources.maxVertexOutputComponents = 64;
		Resources.maxGeometryInputComponents = 64;
		Resources.maxGeometryOutputComponents = 128;
		Resources.maxFragmentInputComponents = 128;
		Resources.maxImageUnits = 8;
		Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
		Resources.maxCombinedShaderOutputResources = 8;
		Resources.maxImageSamples = 0;
		Resources.maxVertexImageUniforms = 0;
		Resources.maxTessControlImageUniforms = 0;
		Resources.maxTessEvaluationImageUniforms = 0;
		Resources.maxGeometryImageUniforms = 0;
		Resources.maxFragmentImageUniforms = 8;
		Resources.maxCombinedImageUniforms = 8;
		Resources.maxGeometryTextureImageUnits = 16;
		Resources.maxGeometryOutputVertices = 256;
		Resources.maxGeometryTotalOutputComponents = 1024;
		Resources.maxGeometryUniformComponents = 1024;
		Resources.maxGeometryVaryingComponents = 64;
		Resources.maxTessControlInputComponents = 128;
		Resources.maxTessControlOutputComponents = 128;
		Resources.maxTessControlTextureImageUnits = 16;
		Resources.maxTessControlUniformComponents = 1024;
		Resources.maxTessControlTotalOutputComponents = 4096;
		Resources.maxTessEvaluationInputComponents = 128;
		Resources.maxTessEvaluationOutputComponents = 128;
		Resources.maxTessEvaluationTextureImageUnits = 16;
		Resources.maxTessEvaluationUniformComponents = 1024;
		Resources.maxTessPatchComponents = 120;
		Resources.maxPatchVertices = 32;
		Resources.maxTessGenLevel = 64;
		Resources.maxViewports = 16;
		Resources.maxVertexAtomicCounters = 0;
		Resources.maxTessControlAtomicCounters = 0;
		Resources.maxTessEvaluationAtomicCounters = 0;
		Resources.maxGeometryAtomicCounters = 0;
		Resources.maxFragmentAtomicCounters = 8;
		Resources.maxCombinedAtomicCounters = 8;
		Resources.maxAtomicCounterBindings = 1;
		Resources.maxVertexAtomicCounterBuffers = 0;
		Resources.maxTessControlAtomicCounterBuffers = 0;
		Resources.maxTessEvaluationAtomicCounterBuffers = 0;
		Resources.maxGeometryAtomicCounterBuffers = 0;
		Resources.maxFragmentAtomicCounterBuffers = 1;
		Resources.maxCombinedAtomicCounterBuffers = 1;
		Resources.maxAtomicCounterBufferSize = 16384;
		Resources.maxTransformFeedbackBuffers = 4;
		Resources.maxTransformFeedbackInterleavedComponents = 64;
		Resources.maxCullDistances = 8;
		Resources.maxCombinedClipAndCullDistances = 8;
		Resources.maxSamples = 4;
		Resources.limits.nonInductiveForLoops = 1;
		Resources.limits.whileLoops = 1;
		Resources.limits.doWhileLoops = 1;
		Resources.limits.generalUniformIndexing = 1;
		Resources.limits.generalAttributeMatrixVectorIndexing = 1;
		Resources.limits.generalVaryingIndexing = 1;
		Resources.limits.generalSamplerIndexing = 1;
		Resources.limits.generalVariableIndexing = 1;
		Resources.limits.generalConstantMatrixVectorIndexing = 1;
	}

	static void FinalizeGlslang() {
		glslang::FinalizeProcess();
	}
};

bool GetPhysicalMemoryType(const vk::PhysicalDevice& gpu, vk::MemoryRequirements reqs, vk::MemoryPropertyFlags desiredMask, uint32_t& typeIndex)
{
	vk::PhysicalDeviceMemoryProperties memoryProperties;
	gpu.getMemoryProperties(&memoryProperties);
	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
		if ((reqs.memoryTypeBits & 1) == 1) {
			if ((memoryProperties.memoryTypes[i].propertyFlags & desiredMask) == desiredMask) {
				typeIndex = i;
				return true;
			}
		}
		reqs.memoryTypeBits >>= 1;
	}

	return false;
}

struct BufferMemory
{
	vk::DeviceMemory memory;
	vk::Buffer buffer;
};

struct ImageMemory
{
	vk::Image image;
	vk::Sampler sampler;
	vk::ImageView view;
	vk::DeviceMemory memory;
};