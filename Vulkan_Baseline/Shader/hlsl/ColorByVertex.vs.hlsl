//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------
// ColorByVertex -- vertex stage.
//
// 3 positions + 3 colors baked in, indexed by SV_VertexID -- still no vertex
// buffer. The positions are now WORLD space and go through the camera's view +
// projection, handed over as push constants. Edit this file while the app runs
// and the ShaderWatcher will hot-reload it.

//-----------------------------------------------------------------
// Push constants -- must match Neelam::vk::ShaderMatrices exactly (128 bytes),
// and match the VkPushConstantRange declared in ShaderObject::privBuildLayout.
//
// row_major is NOT optional. HLSL packs matrices COLUMN-major by default, but
// Azul::Mat4 is stored as 4 row Vec4s and Azul is a row-vector library (v * M).
// Without row_major every matrix arrives transposed and the triangle vanishes
// off-screen. With it, mul(vector, matrix) below is the same operation the C++
// side does.
//-----------------------------------------------------------------
struct PushConstants
{
	row_major float4x4 view;
	row_major float4x4 proj;
};

[[vk::push_constant]] PushConstants pc;

struct VSOutput
{
	float4 position : SV_Position;
	float3 color    : COLOR0;
};

// World space, +Y UP. These used to be authored directly in Vulkan NDC (where
// +Y is DOWN), so the signs are flipped from the original: the camera's
// projection now does the Y flip (see Camera::privUpdateProjectionMatrix).
// One unit tall, sitting at the origin -- the camera is 5 units back on +Z.
static const float2 positions[3] =
{
	float2( 0.0,  0.5),		// top
	float2( 0.5, -0.5),		// bottom-right
	float2(-0.5, -0.5)		// bottom-left
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

	// Row-vector chain, left to right: world -> view -> clip. Matches the C++
	// side's  pos = Vec4(pos, 1.0f) * M.
	float4 worldPos = float4(positions[vertexID], 0.0, 1.0);
	float4 viewPos  = mul(worldPos, pc.view);
	output.position = mul(viewPos, pc.proj);

	output.color = colors[vertexID];
	return output;
}
