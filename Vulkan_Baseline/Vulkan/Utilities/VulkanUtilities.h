//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef VULKAN_UTILITIES_H
#define VULKAN_UTILITIES_H

// volk is the single Vulkan include for the whole engine. It defines
// VK_NO_PROTOTYPES and pulls in <vulkan/vulkan.h>, exposing every vk* name as
// a function POINTER that volkInitialize / volkLoadInstance / volkLoadDevice
// fill in at runtime (see Instance.cpp / LogicalDevice.cpp). The definitions
// live in the ThirdParty static lib; here we only get the declarations.
#include <Volk/volk.h>

// -------------------------------------------------------------------------
//                      vkAssert
// - To print the VkResult to the debug and behave like an assert(); function
// - Instead of the code snippents where you use  if(res != VK_SUCCESS) return false;
//         use vkAssert(function(args));
// - it would say the error and where it is called from
//
// - VkResult is NOT an HRESULT, so there is no system message table for Vulkan.
//   The names come from the switch below.
// - Only NEGATIVE codes are errors. VK_SUCCESS is 0 and the positive codes are
//   non-error statuses (VK_TIMEOUT, VK_SUBOPTIMAL_KHR, ...), so the test is
//   "< 0" and not "!= VK_SUCCESS" -- otherwise a merely suboptimal swapchain
//   would kill the process once presenting starts.
// --------------------------------------------------------------------------

static inline const char *vkResultToString(VkResult result) noexcept
{
	switch (result)
	{
		// Success / non-error statuses
	case VK_SUCCESS:						return "VK_SUCCESS";
	case VK_NOT_READY:						return "VK_NOT_READY";
	case VK_TIMEOUT:						return "VK_TIMEOUT";
	case VK_EVENT_SET:						return "VK_EVENT_SET";
	case VK_EVENT_RESET:					return "VK_EVENT_RESET";
	case VK_INCOMPLETE:						return "VK_INCOMPLETE";
	case VK_SUBOPTIMAL_KHR:					return "VK_SUBOPTIMAL_KHR";

		// Errors -- what vkCreateInstance hands back
	case VK_ERROR_OUT_OF_HOST_MEMORY:		return "VK_ERROR_OUT_OF_HOST_MEMORY";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	case VK_ERROR_INITIALIZATION_FAILED:	return "VK_ERROR_INITIALIZATION_FAILED";
	case VK_ERROR_LAYER_NOT_PRESENT:		return "VK_ERROR_LAYER_NOT_PRESENT";
	case VK_ERROR_EXTENSION_NOT_PRESENT:	return "VK_ERROR_EXTENSION_NOT_PRESENT";
	case VK_ERROR_INCOMPATIBLE_DRIVER:		return "VK_ERROR_INCOMPATIBLE_DRIVER";

		// Errors -- general device / object failures
	case VK_ERROR_DEVICE_LOST:				return "VK_ERROR_DEVICE_LOST";
	case VK_ERROR_MEMORY_MAP_FAILED:		return "VK_ERROR_MEMORY_MAP_FAILED";
	case VK_ERROR_FEATURE_NOT_PRESENT:		return "VK_ERROR_FEATURE_NOT_PRESENT";
	case VK_ERROR_TOO_MANY_OBJECTS:			return "VK_ERROR_TOO_MANY_OBJECTS";
	case VK_ERROR_FORMAT_NOT_SUPPORTED:		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
	case VK_ERROR_FRAGMENTED_POOL:			return "VK_ERROR_FRAGMENTED_POOL";
	case VK_ERROR_OUT_OF_POOL_MEMORY:		return "VK_ERROR_OUT_OF_POOL_MEMORY";
	case VK_ERROR_INVALID_EXTERNAL_HANDLE:	return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
	case VK_ERROR_UNKNOWN:					return "VK_ERROR_UNKNOWN";

		// Errors -- surface / swapchain (WSI)
	case VK_ERROR_SURFACE_LOST_KHR:			return "VK_ERROR_SURFACE_LOST_KHR";
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:	return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
	case VK_ERROR_OUT_OF_DATE_KHR:			return "VK_ERROR_OUT_OF_DATE_KHR";
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:	return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
	case VK_ERROR_VALIDATION_FAILED_EXT:	return "VK_ERROR_VALIDATION_FAILED_EXT";

	default:								return "Unknown VkResult";
	}
}

static inline void vkAssertImpl(VkResult result, const char *file, int line) noexcept
{
	if (result < 0)
	{
		// Print the raw code alongside the name so an enum the switch does not
		// know yet is still identifiable from the log.
		Debug::out("%s(%d): <double-click> \nvkAssert failed: %s (%d)\n",
			file,
			line,
			vkResultToString(result),
			(int)result);

		assert(false);
		ExitProcess((UINT)result);
	}
}

// Macro to call the assertion for Vulkan Code
#define VK_Try(expr) vkAssertImpl((expr), __FILE__, __LINE__)

// -------------------------------------------------------------------------
//                      Validation layer helpers
//
// All the debug-messenger boilerplate lives here so the Instance code that
// USES it stays short and readable. Instance just holds the messenger handle
// and calls Validation::IsSupported / FillMessengerInfo / CreateMessenger /
// DestroyMessenger. See Instance.cpp.
//
// Validation is Debug-only (Validation::Enabled): the layer adds real per-call
// overhead you do not want in Release.
// -------------------------------------------------------------------------
namespace Neelam::vk::Validation
{
#ifdef _DEBUG
	static const bool Enabled = true;
#else
	static const bool Enabled = false;
#endif

	// The single layer we ask for. It ships with the Vulkan SDK.
	static const char *const LayerName = "VK_LAYER_KHRONOS_validation";

	// Is that layer actually installed? Requesting a missing layer makes
	// vkCreateInstance fail with VK_ERROR_LAYER_NOT_PRESENT, so check first.
	static inline bool IsSupported()
	{
		uint32_t count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);

		VkLayerProperties *pAvailable = new VkLayerProperties[count];
		vkEnumerateInstanceLayerProperties(&count, pAvailable);

		bool found = false;
		for (uint32_t i = 0; i < count; i++)
		{
			if (strcmp(pAvailable[i].layerName, LayerName) == 0)
			{
				found = true;
				break;
			}
		}

		delete[] pAvailable;
		return found;
	}

	// Where every validation message lands -- all through Keenan's Debug::out so
	// each line is thread-tagged.
	//
	// Debug::out formats into a fixed buffer capped at 256 chars internally and
	// ASSERTS ("Buffer too small") on overflow. Validation strings are often
	// longer, so we memcpy the message into a bounded buffer and print it in
	// CHUNKS -- every chunk stays under the cap, and nothing is lost.
	static inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
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

		Debug::out("[Vulkan %s]\n", pSeverity);

		const char *pMessage = pCallbackData->pMessage ? pCallbackData->pMessage : "(null)";

		// 180 leaves headroom under the 256 cap for the tabs + "(ThreadName): "
		// prefix Debug::out prepends.
		const size_t chunkMax = 180;
		char         chunk[chunkMax + 1];

		size_t total  = strlen(pMessage);
		size_t offset = 0;
		do
		{
			size_t n = total - offset;
			if (n > chunkMax)
			{
				n = chunkMax;
			}

			memcpy(chunk, pMessage + offset, n);
			chunk[n] = '\0';

			Debug::out("%s\n", chunk);		// chunk is the %s arg -> any '%' in it is safe
			offset += n;
		} while (offset < total);

		// VK_FALSE: do not abort the Vulkan call that triggered the message.
		return VK_FALSE;
	}

	// Messenger settings (warnings + errors -- info/verbose are noisy). Shared
	// by the instance-create pNext chain and the standalone messenger.
	static inline void FillMessengerInfo(VkDebugUtilsMessengerCreateInfoEXT &info)
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
		info.pfnUserCallback = DebugCallback;
	}

	// vkCreate/DestroyDebugUtilsMessengerEXT are EXTENSION entry points -- the
	// static loader library does not export them, so their addresses must be
	// fetched at runtime with vkGetInstanceProcAddr. (This hand-loading is
	// exactly the boilerplate a meta-loader like volk would do automatically.)
	static inline VkDebugUtilsMessengerEXT CreateMessenger(VkInstance instance)
	{
		PFN_vkCreateDebugUtilsMessengerEXT pCreate =
			(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
				instance, "vkCreateDebugUtilsMessengerEXT");

		if (pCreate == nullptr)
		{
			Debug::out("Validation: vkCreateDebugUtilsMessengerEXT unavailable\n");
			return VK_NULL_HANDLE;
		}

		VkDebugUtilsMessengerCreateInfoEXT info;
		FillMessengerInfo(info);

		VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
		VK_Try(pCreate(instance, &info, nullptr, &messenger));
		return messenger;
	}

	static inline void DestroyMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger)
	{
		if (messenger == VK_NULL_HANDLE)
		{
			return;
		}

		PFN_vkDestroyDebugUtilsMessengerEXT pDestroy =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
				instance, "vkDestroyDebugUtilsMessengerEXT");

		if (pDestroy != nullptr)
		{
			pDestroy(instance, messenger, nullptr);
		}
	}
}

#endif   // VULKAN_UTILITIES_H

// ---  End of File ---
