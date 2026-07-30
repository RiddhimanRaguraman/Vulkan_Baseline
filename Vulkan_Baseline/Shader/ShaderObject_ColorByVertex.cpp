//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "ShaderObject_ColorByVertex.h"

namespace Neelam::vk
{
	// Absolute paths via SOLUTION_DIR (a compile-time macro from premake), so
	// the shaders are found no matter what the working directory is. The .hlsl
	// files must stay on disk at runtime -- hot-reload reads the SOURCE, not
	// baked bytecode.
	const char *ShaderObject_ColorByVertex::GetVertexPath() const
	{
		return SOLUTION_DIR "Vulkan_Baseline\\Shader\\hlsl\\ColorByVertex.vs.hlsl";
	}

	const char *ShaderObject_ColorByVertex::GetPixelPath() const
	{
		return SOLUTION_DIR "Vulkan_Baseline\\Shader\\hlsl\\ColorByVertex.ps.hlsl";
	}
}

// ---  End of File ---
