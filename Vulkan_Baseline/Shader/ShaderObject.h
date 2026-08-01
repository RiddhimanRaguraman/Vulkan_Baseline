//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef SHADER_OBJECT_H
#define SHADER_OBJECT_H

// Vulkan types come from the force-included Framework.h (VULKAN FRAMEWORK section).
#include "ShaderModule.h"
#include "MathEngine.h"

namespace Neelam::vk
{
	//-----------------------------------------------------------------
	// The per-frame matrices, handed to the vertex stage as PUSH CONSTANTS.
	//
	// Push constants are a small block written straight into the command buffer
	// -- no descriptor set, no buffer, no allocation. Perfect for a couple of
	// matrices that change every frame; the wrong tool once the data gets big.
	//
	// SIZE LIMIT: the Vulkan spec only guarantees maxPushConstantsSize >= 128
	// bytes. Two Mat4s is exactly 128, so this is full. Adding a per-object
	// world matrix means either pre-multiplying view*proj on the CPU (freeing
	// 64 bytes) or moving to a uniform buffer + descriptor set.
	//
	// LAYOUT: Azul::Mat4 is 4 Vec4 rows, ROW-major, and Azul is a row-vector
	// library (v * M). The HLSL side therefore declares these `row_major` and
	// uses mul(vector, matrix) -- see ColorByVertex.vs.hlsl. Get that wrong and
	// every matrix silently arrives transposed.
	//-----------------------------------------------------------------
	struct ShaderMatrices
	{
		Azul::Mat4 view;
		Azul::Mat4 proj;
	};

	static_assert(sizeof(ShaderMatrices) == 128,
		"ShaderMatrices must stay within the 128-byte guaranteed push-constant limit");
}

//---------------------------------------------------------------------------
// class ShaderObject  (abstract base)
//
// A complete "technique" -- the Vulkan take on Azul's ShaderObject. It owns
// its vertex + pixel modules AND the VkPipeline they are baked into, plus the
// pipeline layout.
//
// The key Vulkan difference from Azul/DX11: shaders are NOT bound individually.
// They are compiled into an immutable VkPipeline, so:
//   * SetActive() is one vkCmdBindPipeline (not VSSetShader + PSSetShader).
//   * Reload() must REBUILD the pipeline, not just swap a module.
//
// Derived classes supply only which .hlsl files to use (GetVertexPath /
// GetPixelPath). The base handles compile + pipeline build + hot-reload. The
// pipeline here is the minimal triangle setup: no vertex input (positions come
// from SV_VertexID), one color attachment, dynamic viewport/scissor, depth off.
//---------------------------------------------------------------------------

namespace Neelam::vk
{
	class ShaderObject
	{
	public:
		ShaderObject();
		ShaderObject(const ShaderObject &) = delete;
		ShaderObject &operator = (const ShaderObject &) = delete;
		virtual ~ShaderObject();

		//-----------------------------------------------------------------
		// Lifetime
		//-----------------------------------------------------------------

		// Compile VS+PS and build the pipeline. colorFormat / depthFormat are
		// the swapchain's formats (dynamic rendering bakes them into the
		// pipeline, so they must match the frame loop's attachments).
		void Create(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);

		// Recompile both shaders from disk and rebuild the pipeline. Waits for
		// the device to be idle first. Called on the engine thread when the
		// ShaderWatcher reports a change (see the actor-model note in Game).
		void Reload();

		void Destroy();

		//-----------------------------------------------------------------
		// Draw-time
		//-----------------------------------------------------------------
		void SetActive(VkCommandBuffer cmd) const;		// vkCmdBindPipeline

		// vkCmdPushConstants. Must be called AFTER SetActive() has bound the
		// pipeline, and re-issued every frame -- push constants live in the
		// command buffer, so a pool reset wipes them.
		void SetMatrices(VkCommandBuffer cmd, const ShaderMatrices &matrices) const;

		//-----------------------------------------------------------------
		// Accessors
		//-----------------------------------------------------------------
		VkPipeline       GetPipeline() const;
		VkPipelineLayout GetLayout() const;

		// Source .hlsl files, for the watcher to poll. Derived supplies these.
		virtual const char *GetVertexPath() const = 0;
		virtual const char *GetPixelPath() const  = 0;

	protected:
		// Data
		VkDevice         privDevice;
		VkFormat         privColorFormat;
		VkFormat         privDepthFormat;

		ShaderModule     vertexShader;
		ShaderModule     pixelShader;

		VkPipelineLayout privLayout;
		VkPipeline       privPipeline;

	private:
		void privBuildLayout();
		void privBuildPipeline();		// destroys the old pipeline first if present
	};
}

#endif   // SHADER_OBJECT_H

// ---  End of File ---
