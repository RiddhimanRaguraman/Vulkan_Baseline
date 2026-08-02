//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef GPU_BUFFER_H
#define GPU_BUFFER_H

// Vulkan + VMA types come from the force-included Framework.h.

namespace Neelam::vk
{
	//-----------------------------------------------------------------------
	// struct VertexPosColor
	//
	// One vertex as the GPU sees it. Deliberately PLAIN FLOATS, not Azul::Vec3:
	// Vec3 is Align16, so sizeof(Vec3) is 16 and a {Vec3,Vec3} vertex would be
	// a 32-byte stride with 8 bytes of padding per field. This is 24, tight,
	// and its offsets match the attribute descriptions in
	// ShaderObject_ColorByVertex. Azul math types are for CPU-side SIMD work;
	// GPU buffers want packed data.
	//-----------------------------------------------------------------------
	struct VertexPosColor
	{
		float x, y, z;			// POSITION
		float r, g, b;			// COLOR0
	};

	static_assert(sizeof(VertexPosColor) == 24,
		"VertexPosColor must stay packed -- the attribute offsets depend on it");

	//-----------------------------------------------------------------------
	// class GpuBuffer
	//
	// One VMA-allocated VkBuffer. Used for both vertex and index data -- the
	// only difference is the usage flag, so this is one class rather than two
	// near-identical ones.
	//
	// UPLOAD STRATEGY: host-visible and persistently MAPPED, written with a
	// plain memcpy. No staging buffer, no transfer command buffer, no queue
	// submit -- which is why this is ~80 lines instead of ~300.
	//
	// The tradeoff, stated plainly: on a discrete GPU this lives in host or BAR
	// memory, so the GPU reads it across PCIe every draw. That is fine for a
	// handful of verts and for anything the CPU rewrites often. Large STATIC
	// geometry wants DEVICE_LOCAL plus a staging copy; that needs transfer
	// infrastructure this engine does not have yet, and it is the natural
	// upgrade once meshes get real.
	//
	// Allocation goes through vk::VulkanAllocator, so every byte here is
	// tracked and shows up in the leak report (§11).
	//-----------------------------------------------------------------------
	class GpuBuffer
	{
	public:
		GpuBuffer();
		GpuBuffer(const GpuBuffer &) = delete;
		GpuBuffer &operator = (const GpuBuffer &) = delete;
		~GpuBuffer();

		// usage is VK_BUFFER_USAGE_VERTEX_BUFFER_BIT / INDEX_BUFFER_BIT.
		// Copies pData in immediately; pData is not retained.
		void Create(const void *pData, VkDeviceSize sizeBytes, VkBufferUsageFlags usage);

		// Overwrite the contents. Same size only -- the allocation is fixed.
		void Update(const void *pData, VkDeviceSize sizeBytes);

		void Destroy();

		VkBuffer     GetBuffer() const;
		VkDeviceSize GetSize() const;

	private:
		VkBuffer      privBuffer;
		VmaAllocation privAllocation;
		void         *privMapped;		// persistent mapping, owned by VMA
		VkDeviceSize  privSize;
	};
}

#endif   // GPU_BUFFER_H

// ---  End of File ---
