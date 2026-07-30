//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "Allocator.h"

namespace Neelam::vk
{
	Allocator::Allocator()
		: privAllocator(VK_NULL_HANDLE)
	{
	}

	Allocator::~Allocator()
	{
		this->Destroy();
	}

	void Allocator::Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
	{
		// VMA needs Vulkan function pointers. Because we use volk, we hand it
		// volk's already-loaded pointers via vmaImportVulkanFunctionsFromVolk
		// rather than letting VMA load its own -- one source of truth.
		VmaVulkanFunctions functions = {};

		VmaAllocatorCreateInfo createInfo = {};
		// BUFFER_DEVICE_ADDRESS lets VMA hand out GPU addresses for buffers.
		// It requires the device to have bufferDeviceAddress enabled -- which
		// LogicalDevice does (see the feature chain there).
		createInfo.flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		createInfo.physicalDevice   = physicalDevice;
		createInfo.device           = device;
		createInfo.pVulkanFunctions = &functions;
		createInfo.instance         = instance;
		createInfo.vulkanApiVersion = VK_API_VERSION_1_4;

		VK_Try(vmaImportVulkanFunctionsFromVolk(&createInfo, &functions));

		VK_Try(vmaCreateAllocator(&createInfo, &this->privAllocator));

		Trace::out("Allocator: VMA allocator created\n");
	}

	void Allocator::Destroy()
	{
		if (this->privAllocator != VK_NULL_HANDLE)
		{
			vmaDestroyAllocator(this->privAllocator);
			this->privAllocator = VK_NULL_HANDLE;
		}
	}

	VmaAllocator Allocator::GetAllocator() const
	{
		return this->privAllocator;
	}
}

// ---  End of File ---
