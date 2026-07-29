//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "Instance.h"

namespace Neelam::vk
{
	Instance::Instance()
		: privInstance(VK_NULL_HANDLE),
		  privDebugMessenger(VK_NULL_HANDLE)
	{
	}

	Instance::~Instance()
	{
		this->Destroy();
	}

	void Instance::Create(const char *pAppName)
	{
		// -------------------------------------------------------------
		// Validation layer (Debug only, and only if it is installed).
		// -------------------------------------------------------------
		const char *layers[1] = { "VK_LAYER_KHRONOS_validation" };
		uint32_t    layerCount = 0;

		if (privEnableValidation)
		{
			if (this->privCheckValidationLayerSupport())
			{
				layerCount = 1;
			}
			else
			{
				// Not fatal: the layer ships with the Vulkan SDK, so a machine
				// with only the runtime driver simply runs without it.
				Trace::out("Instance: VK_LAYER_KHRONOS_validation not found "
					"(install the Vulkan SDK?) -- continuing without validation\n");
			}
		}

		// -------------------------------------------------------------
		// Extensions: the two surface ones always; debug-utils only when
		// validation is actually on (it provides the messenger).
		// -------------------------------------------------------------
		const char *extensions[3];
		uint32_t    extCount = 0;
		extensions[extCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
		extensions[extCount++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
		if (layerCount > 0)
		{
			extensions[extCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName   = pAppName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName        = "Neelam";
		appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion         = VK_API_VERSION_1_4;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo        = &appInfo;
		createInfo.enabledExtensionCount   = extCount;
		createInfo.ppEnabledExtensionNames = extensions;
		createInfo.enabledLayerCount       = layerCount;
		createInfo.ppEnabledLayerNames     = (layerCount > 0) ? layers : nullptr;

		// Chain a messenger create-info into pNext so the layer also validates
		// the vkCreateInstance / vkDestroyInstance calls themselves -- the
		// standalone messenger below does not exist yet during those calls.
		VkDebugUtilsMessengerCreateInfoEXT dbgInfo = {};
		if (layerCount > 0)
		{
			Instance::privFillMessengerInfo(dbgInfo);
			createInfo.pNext = &dbgInfo;
		}

		VK_Try(vkCreateInstance(&createInfo, nullptr, &this->privInstance));

		// Standalone messenger: catches everything after instance creation.
		if (layerCount > 0)
		{
			this->privCreateDebugMessenger();
		}
	}

	void Instance::Destroy()
	{
		if (this->privInstance != VK_NULL_HANDLE)
		{
			// Messenger is created FROM the instance, so it goes first.
			this->privDestroyDebugMessenger();

			vkDestroyInstance(this->privInstance, nullptr);
			this->privInstance = VK_NULL_HANDLE;
		}
	}

	VkInstance Instance::GetInstance() const
	{
		return this->privInstance;
	}

	//-----------------------------------------------------------------
	// Is the validation layer present? Requesting a missing layer makes
	// vkCreateInstance fail with VK_ERROR_LAYER_NOT_PRESENT, so check first.
	//-----------------------------------------------------------------
	bool Instance::privCheckValidationLayerSupport() const
	{
		uint32_t count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);

		VkLayerProperties *pAvailable = new VkLayerProperties[count];
		vkEnumerateInstanceLayerProperties(&count, pAvailable);

		bool found = false;
		for (uint32_t i = 0; i < count; i++)
		{
			if (strcmp(pAvailable[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
			{
				found = true;
				break;
			}
		}

		delete[] pAvailable;
		return found;
	}

	//-----------------------------------------------------------------
	// Messenger settings shared by the pNext chain and the standalone
	// messenger. Warnings + errors only -- info/verbose are noisy.
	//-----------------------------------------------------------------
	void Instance::privFillMessengerInfo(VkDebugUtilsMessengerCreateInfoEXT &info)
	{
		info = {};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		info.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		info.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		info.pfnUserCallback = Instance::privDebugCallback;
	}

	//-----------------------------------------------------------------
	// vkCreateDebugUtilsMessengerEXT is an EXTENSION entry point -- the static
	// loader library does not export it, so its address must be fetched at
	// runtime with vkGetInstanceProcAddr. (This hand-loading is exactly the
	// boilerplate a meta-loader like volk would do for us automatically.)
	//-----------------------------------------------------------------
	void Instance::privCreateDebugMessenger()
	{
		PFN_vkCreateDebugUtilsMessengerEXT pCreate =
			(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
				this->privInstance, "vkCreateDebugUtilsMessengerEXT");

		if (pCreate == nullptr)
		{
			Trace::out("Instance: vkCreateDebugUtilsMessengerEXT unavailable\n");
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT info;
		Instance::privFillMessengerInfo(info);

		VK_Try(pCreate(this->privInstance, &info, nullptr, &this->privDebugMessenger));
	}

	void Instance::privDestroyDebugMessenger()
	{
		if (this->privDebugMessenger == VK_NULL_HANDLE)
		{
			return;
		}

		PFN_vkDestroyDebugUtilsMessengerEXT pDestroy =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
				this->privInstance, "vkDestroyDebugUtilsMessengerEXT");

		if (pDestroy != nullptr)
		{
			pDestroy(this->privInstance, this->privDebugMessenger, nullptr);
		}

		this->privDebugMessenger = VK_NULL_HANDLE;
	}

	//-----------------------------------------------------------------
	// Every validation message lands here. Routed to Trace::out so it shows up
	// in the VS Output window next to the rest of the engine's logging.
	//-----------------------------------------------------------------
	VKAPI_ATTR VkBool32 VKAPI_CALL Instance::privDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
		void *pUserData)
	{
		AZUL_UNUSED_VAR(type);
		AZUL_UNUSED_VAR(pUserData);

		const char *pSeverity =
			(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   ? "ERROR" :
			(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "WARN"  :
																		   "INFO";

		Trace::out("[Vulkan %s] %s\n", pSeverity, pCallbackData->pMessage);

		// VK_FALSE: do not abort the Vulkan call that triggered the message.
		return VK_FALSE;
	}
}

// ---  End of File ---
