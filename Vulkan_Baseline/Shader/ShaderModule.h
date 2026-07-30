//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef SHADER_MODULE_H
#define SHADER_MODULE_H

#include "VulkanUtilities.h"
#include "ShaderCompiler.h"		// ShaderStage

//---------------------------------------------------------------------------
// class ShaderModule
//
// The low-level wrapper (Azul's BufferVertexShader / BufferPixelShader) around
// ONE VkShaderModule. It knows its source .hlsl path + stage, so it can:
//   Create()  -- compile the file (DXC) and make the module. A broken shader
//                at startup is fatal (assert + exit).
//   Reload()  -- recompile from disk; on success swap in the new module, on
//                failure KEEP the current one (hot-reload keep-last-good).
//
// A VkShaderModule is stage-agnostic in Vulkan (the stage is named at pipeline
// time), so this is ONE parameterized class rather than separate VS/PS types.
//---------------------------------------------------------------------------

namespace Neelam::vk
{
	class ShaderModule
	{
	public:
		ShaderModule();
		ShaderModule(const ShaderModule &) = delete;
		ShaderModule &operator = (const ShaderModule &) = delete;
		~ShaderModule();

		void Create(VkDevice device, const char *pFilePath, ShaderStage stage);

		// Returns true if a NEW module was built and swapped in.
		bool Reload();

		void Destroy();

		VkShaderModule                  GetModule() const;
		VkShaderStageFlagBits           GetStageBit() const;
		const char                     *GetPath() const;
		VkPipelineShaderStageCreateInfo GetStageCreateInfo() const;

	private:
		// Compile privPath for privStage into a fresh VkShaderModule.
		// Returns VK_NULL_HANDLE on failure (already logged).
		VkShaderModule privCompile() const;

		VkDevice       privDevice;
		ShaderStage    privStage;
		char           privPath[512];
		VkShaderModule privModule;
	};
}

#endif   // SHADER_MODULE_H

// ---  End of File ---
