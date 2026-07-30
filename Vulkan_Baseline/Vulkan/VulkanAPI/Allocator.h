//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "VulkanUtilities.h"		// volk first -> defines VOLK_HEADER_VERSION

// VMA declarations. VulkanUtilities.h (volk) is included ABOVE, so VMA knows
// about volk and declares vmaImportVulkanFunctionsFromVolk. Only declarations
// come in here -- the implementation is compiled once in the ThirdParty lib.
#include <vma/vk_mem_alloc.h>

//---------------------------------------------------------------------------
// class Allocator
//
// Wraps VMA's VmaAllocator -- the Vulkan Memory Allocator. Vulkan makes you
// manage GPU memory by hand (query memory types, sub-allocate, align, pool);
// VMA does all of that for you, so buffers and images become one call each.
//
// This is a supporting object: it owns the VmaAllocator and hands it out.
// It is built from the instance + physical device + logical device, and takes
// its Vulkan entry points straight from volk (vmaImportVulkanFunctionsFromVolk),
// so it must be created AFTER the device (volkLoadDevice has run by then).
//---------------------------------------------------------------------------

namespace Neelam::vk
{
	class Allocator
	{
	public:
		//-----------------------------------------------------------------
		// Constructors / Destructors
		//-----------------------------------------------------------------
		Allocator();
		Allocator(const Allocator &) = delete;
		Allocator &operator = (const Allocator &) = delete;
		~Allocator();

		//-----------------------------------------------------------------
		// Lifetime
		//-----------------------------------------------------------------

		// Build the allocator. Needs all three: instance, the chosen GPU, and
		// the logical device it allocates from.
		void Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

		// vmaDestroyAllocator. Owned, so this really releases.
		void Destroy();

		//-----------------------------------------------------------------
		// Accessors
		//-----------------------------------------------------------------
		VmaAllocator GetAllocator() const;

	private:
		// Data
		VmaAllocator privAllocator;
	};
}

#endif   // ALLOCATOR_H

// ---  End of File ---
