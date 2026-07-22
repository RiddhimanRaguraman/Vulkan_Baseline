//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "VulkanSurface.h"

namespace Azul
{
	VulkanSurface::VulkanSurface()
		: privInstance(VK_NULL_HANDLE),
		  privSurface(VK_NULL_HANDLE)
	{
	}

	VulkanSurface::~VulkanSurface()
	{
		this->Destroy();
	}

	void VulkanSurface::Create(VkInstance instance, HINSTANCE module, HWND hwnd)
	{
		this->privInstance = instance;

		// VkWin32SurfaceCreateInfoKHR only exists when VK_USE_PLATFORM_WIN32_KHR
		// is defined -- see the note in the header.
		VkWin32SurfaceCreateInfoKHR createInfo = {};
		createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hinstance = module;
		createInfo.hwnd      = hwnd;

		vkAssert(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &this->privSurface));
	}

	void VulkanSurface::Destroy()
	{
		if (this->privSurface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(this->privInstance, this->privSurface, nullptr);
			this->privSurface = VK_NULL_HANDLE;
		}

		this->privInstance = VK_NULL_HANDLE;
	}

	VkSurfaceKHR VulkanSurface::GetSurface() const
	{
		return this->privSurface;
	}
}

// ---  End of File ---
