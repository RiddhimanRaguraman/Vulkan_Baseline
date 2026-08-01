//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "ShaderObject.h"

namespace Neelam::vk
{
	ShaderObject::ShaderObject()
		: privDevice(VK_NULL_HANDLE),
		  privColorFormat(VK_FORMAT_UNDEFINED),
		  privDepthFormat(VK_FORMAT_UNDEFINED),
		  vertexShader(),
		  pixelShader(),
		  privLayout(VK_NULL_HANDLE),
		  privPipeline(VK_NULL_HANDLE)
	{
	}

	ShaderObject::~ShaderObject()
	{
		this->Destroy();
	}

	void ShaderObject::Create(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
	{
		this->privDevice      = device;
		this->privColorFormat = colorFormat;
		this->privDepthFormat = depthFormat;

		// Compile the two stages (paths come from the derived technique).
		this->vertexShader.Create(device, this->GetVertexPath(), ShaderStage::Vertex);
		this->pixelShader.Create(device, this->GetPixelPath(),  ShaderStage::Pixel);

		this->privBuildLayout();
		this->privBuildPipeline();

		Debug::out("ShaderObject: pipeline built\n");
	}

	void ShaderObject::Reload()
	{
		// No in-flight work may be using the pipeline we are about to replace.
		vkDeviceWaitIdle(this->privDevice);

		// Recompile both. Each keeps its last-good module on failure.
		this->vertexShader.Reload();
		this->pixelShader.Reload();

		// Rebuild from the (possibly new) modules.
		this->privBuildPipeline();

		Debug::out("ShaderObject: reloaded + pipeline rebuilt\n");
	}

	void ShaderObject::SetActive(VkCommandBuffer cmd) const
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->privPipeline);
	}

	void ShaderObject::SetMatrices(VkCommandBuffer cmd, const ShaderMatrices &matrices) const
	{
		vkCmdPushConstants(cmd, this->privLayout, VK_SHADER_STAGE_VERTEX_BIT,
			0, (uint32_t)sizeof(ShaderMatrices), &matrices);
	}

	void ShaderObject::Destroy()
	{
		if (this->privPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(this->privDevice, this->privPipeline, nullptr);
			this->privPipeline = VK_NULL_HANDLE;
		}
		if (this->privLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(this->privDevice, this->privLayout, nullptr);
			this->privLayout = VK_NULL_HANDLE;
		}

		this->vertexShader.Destroy();
		this->pixelShader.Destroy();
	}

	//-----------------------------------------------------------------
	// No descriptor sets yet -- but one PUSH CONSTANT range for the camera's
	// view + proj matrices (ShaderMatrices, 128 bytes, vertex stage only).
	//
	// The range must be declared here because the layout is what the shader's
	// push-constant block is validated against; a mismatch between this and the
	// HLSL block is a validation error, not a silent wrong result.
	//
	// Built once in Create() and NOT rebuilt by Reload(), so the layout survives
	// shader hot-reload -- which is what lets an edited .hlsl keep the same
	// push-constant contract.
	//-----------------------------------------------------------------
	void ShaderObject::privBuildLayout()
	{
		VkPushConstantRange matrixRange = {};
		matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		matrixRange.offset     = 0;
		matrixRange.size       = (uint32_t)sizeof(ShaderMatrices);

		VkPipelineLayoutCreateInfo info = {};
		info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.pushConstantRangeCount = 1;
		info.pPushConstantRanges    = &matrixRange;

		VK_Try(vkCreatePipelineLayout(this->privDevice, &info, nullptr, &this->privLayout));
	}

	//-----------------------------------------------------------------
	// The graphics pipeline. Minimal triangle setup, dynamic rendering.
	//-----------------------------------------------------------------
	void ShaderObject::privBuildPipeline()
	{
		// Rebuild path: drop the old pipeline first.
		if (this->privPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(this->privDevice, this->privPipeline, nullptr);
			this->privPipeline = VK_NULL_HANDLE;
		}

		VkPipelineShaderStageCreateInfo stages[2] =
		{
			this->vertexShader.GetStageCreateInfo(),
			this->pixelShader.GetStageCreateInfo()
		};

		// No vertex buffers -- the vertex shader builds positions from
		// SV_VertexID, so the vertex input state is empty.
		VkPipelineVertexInputStateCreateInfo vertexInput = {};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// Viewport + scissor are DYNAMIC (set at draw time), so the pipeline is
		// independent of the current window size -- friendly to resizing.
		VkPipelineViewportStateCreateInfo viewport = {};
		viewport.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport.viewportCount = 1;
		viewport.scissorCount  = 1;

		VkPipelineRasterizationStateCreateInfo raster = {};
		raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		raster.polygonMode = VK_POLYGON_MODE_FILL;
		raster.cullMode    = VK_CULL_MODE_NONE;			// draw the triangle either winding
		raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		raster.lineWidth   = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisample = {};
		multisample.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// Depth test + write on, LESS. The frame loop attaches the swapchain's
		// depth image; the pipeline must declare it too (depthAttachmentFormat
		// below) or validation flags the mismatch.
		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable  = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.blendEnable    = VK_FALSE;
		blendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlend = {};
		colorBlend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlend.attachmentCount = 1;
		colorBlend.pAttachments    = &blendAttachment;

		VkDynamicState dynamics[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates    = dynamics;

		// Dynamic rendering: no VkRenderPass object -- the pipeline just names
		// the attachment formats it will be used with (one color, no depth).
		VkPipelineRenderingCreateInfo rendering = {};
		rendering.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		rendering.colorAttachmentCount    = 1;
		rendering.pColorAttachmentFormats = &this->privColorFormat;
		rendering.depthAttachmentFormat   = this->privDepthFormat;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.pNext               = &rendering;			// dynamic rendering
		info.stageCount          = 2;
		info.pStages             = stages;
		info.pVertexInputState   = &vertexInput;
		info.pInputAssemblyState = &inputAssembly;
		info.pViewportState      = &viewport;
		info.pRasterizationState = &raster;
		info.pMultisampleState   = &multisample;
		info.pDepthStencilState  = &depthStencil;
		info.pColorBlendState    = &colorBlend;
		info.pDynamicState       = &dynamicState;
		info.layout              = this->privLayout;
		info.renderPass          = VK_NULL_HANDLE;		// none -> dynamic rendering

		VK_Try(vkCreateGraphicsPipelines(this->privDevice, VK_NULL_HANDLE, 1, &info, nullptr, &this->privPipeline));
	}

	VkPipeline ShaderObject::GetPipeline() const
	{
		return this->privPipeline;
	}

	VkPipelineLayout ShaderObject::GetLayout() const
	{
		return this->privLayout;
	}
}

// ---  End of File ---
