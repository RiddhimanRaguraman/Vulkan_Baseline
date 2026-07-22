//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "VulkanInstance.h"

namespace Azul
{
	VulkanInstance::VulkanInstance()
		: privInstance(VK_NULL_HANDLE)
	{
	}

	VulkanInstance::~VulkanInstance()
	{
		this->Destroy();
	}

	void VulkanInstance::Create(const char *pAppName)
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName   = pAppName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName        = "Azul";
		appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion         = VK_API_VERSION_1_0;

		const char *extensions[2];
		extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
		extensions[1] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo        = &appInfo;
		createInfo.enabledExtensionCount   = 2;
		createInfo.ppEnabledExtensionNames = extensions;
		createInfo.enabledLayerCount       = 0;

		vkAssert(vkCreateInstance(&createInfo, nullptr, &this->privInstance));
	}

	void VulkanInstance::Destroy()
	{
		if (this->privInstance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(this->privInstance, nullptr);
			this->privInstance = VK_NULL_HANDLE;
		}
	}

	VkInstance VulkanInstance::GetInstance() const
	{
		return this->privInstance;
	}
}

// ---  End of File ---
