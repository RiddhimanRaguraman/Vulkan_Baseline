//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------
// ColorByVertex -- vertex stage.
//
// The classic hello-triangle: 3 positions + 3 colors baked in, indexed by
// SV_VertexID. No vertex buffer needed. Edit this file while the app runs and
// the ShaderWatcher will hot-reload it.

struct VSOutput
{
	float4 position : SV_Position;
	float3 color    : COLOR0;
};

static const float2 positions[3] =
{
	float2( 0.0, -0.5),		// top
	float2( 0.5,  0.5),		// bottom-right
	float2(-0.5,  0.5)		// bottom-left
};

static const float3 colors[3] =
{
	float3(1.0, 0.0, 0.0),	// red
	float3(0.0, 1.0, 0.0),	// green
	float3(0.0, 0.0, 1.0)	// blue
};

VSOutput main(uint vertexID : SV_VertexID)
{
	VSOutput output;
	output.position = float4(positions[vertexID], 0.0, 1.0);
	output.color    = colors[vertexID];
	return output;
}
