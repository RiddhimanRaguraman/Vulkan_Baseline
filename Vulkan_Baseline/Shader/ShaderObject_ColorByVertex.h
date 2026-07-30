//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef SHADER_OBJECT_COLOR_BY_VERTEX_H
#define SHADER_OBJECT_COLOR_BY_VERTEX_H

#include "ShaderObject.h"

//---------------------------------------------------------------------------
// class ShaderObject_ColorByVertex
//
// The concrete triangle technique (same name Azul used for the vertex-colored
// shader). It only supplies its two .hlsl source files; the base does the
// compile, pipeline build, and hot-reload. The triangle's vertices + colors
// live inside the vertex shader (indexed by SV_VertexID), so there is no
// vertex buffer or input layout to configure here yet.
//---------------------------------------------------------------------------

namespace Neelam::vk
{
	class ShaderObject_ColorByVertex : public ShaderObject
	{
	public:
		ShaderObject_ColorByVertex() = default;
		ShaderObject_ColorByVertex(const ShaderObject_ColorByVertex &) = delete;
		ShaderObject_ColorByVertex &operator = (const ShaderObject_ColorByVertex &) = delete;
		virtual ~ShaderObject_ColorByVertex() = default;

		virtual const char *GetVertexPath() const override;
		virtual const char *GetPixelPath() const override;
	};
}

#endif   // SHADER_OBJECT_COLOR_BY_VERTEX_H

// ---  End of File ---
