//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef VULKAN_SURFACE_H
#define VULKAN_SURFACE_H

#include "VulkanUtilities.h"

//---------------------------------------------------------------------------
// class VulkanSurface
//
// Bridges the Win32 window to Vulkan: turns an HWND into a VkSurfaceKHR,
// which is what a swapchain later gets presented to.
//
// This is the piece that needs the VK_USE_PLATFORM_WIN32_KHR macro. That
// macro is defined project-wide in premake5.lua, and without it neither
// VkWin32SurfaceCreateInfoKHR nor vkCreateWin32SurfaceKHR is declared --
// the Win32 half of the Vulkan headers compiles out entirely.
//
// The surface does NOT own the VkInstance it is created from; the instance
// must outlive the surface (destroy the surface first).
//---------------------------------------------------------------------------

namespace Azul
{
	class VulkanSurface
	{
	public:
		//-----------------------------------------------------------------
		// Constructors / Destructors
		//-----------------------------------------------------------------
		VulkanSurface();
		VulkanSurface(const VulkanSurface &) = delete;
		VulkanSurface &operator = (const VulkanSurface &) = delete;
		~VulkanSurface();

		//-----------------------------------------------------------------
		// Lifetime
		//-----------------------------------------------------------------
		void Create(VkInstance instance, HINSTANCE module, HWND hwnd);

		// Safe to call more than once; the destructor calls it too.
		void Destroy();

		//-----------------------------------------------------------------
		// Accessors
		//-----------------------------------------------------------------
		VkSurfaceKHR GetSurface() const;

	private:
		// Data
		VkInstance   privInstance;		// borrowed, not owned
		VkSurfaceKHR privSurface;
	};
}

#endif   // VULKAN_SURFACE_H

// ---  End of File ---
