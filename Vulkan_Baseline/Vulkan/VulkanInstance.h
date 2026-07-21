//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include <vulkan/vulkan.hpp>

//---------------------------------------------------------------------------
// class VulkanInstance
//
// Owns the VkInstance -- the root Vulkan object every other call hangs off.
// It is created with the two surface extensions turned on:
//
//     VK_KHR_surface         -- generic surface support
//     VK_KHR_win32_surface   -- the Win32 flavour, needed by VulkanSurface
//
// We include the C++ bindings <vulkan/vulkan.hpp>, which pulls in the C header
// <vulkan/vulkan.h> for us -- so the plain C API used below stays available
// while leaving the door open to layer vk:: RAII types on top later.
//---------------------------------------------------------------------------

namespace Azul
{
	class VulkanInstance
	{
	public:
		//-----------------------------------------------------------------
		// Constructors / Destructors
		//-----------------------------------------------------------------
		VulkanInstance();
		VulkanInstance(const VulkanInstance &) = delete;
		VulkanInstance &operator = (const VulkanInstance &) = delete;
		~VulkanInstance();

		//-----------------------------------------------------------------
		// Lifetime
		//-----------------------------------------------------------------
		bool Create(const char *pAppName);

		// Safe to call more than once; the destructor calls it too.
		void Destroy();

		//-----------------------------------------------------------------
		// Accessors
		//-----------------------------------------------------------------
		VkInstance GetInstance() const;

	private:
		// Data
		VkInstance privInstance;
	};
}

#endif   // VULKAN_INSTANCE_H

// ---  End of File ---
