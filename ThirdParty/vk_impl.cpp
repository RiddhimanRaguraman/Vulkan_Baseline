//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------
//
// The single translation unit that compiles the header-only implementations
// of volk (the Vulkan meta-loader) and VMA (Vulkan Memory Allocator).
//
// It lives in its OWN static-library project on purpose. The app project
// force-includes Framework.h, whose Debug memory-tracking macros (#define new,
// #define malloc, ...) rewrite allocation syntax and would corrupt VMA's C++
// implementation (placement new, custom allocators). Compiling volk/VMA in
// isolation -- no Framework.h, no pch -- keeps these third-party sources
// pristine. The app only ever sees their DECLARATIONS and links this lib.
//
// VK_NO_PROTOTYPES / VK_USE_PLATFORM_WIN32_KHR / NOMINMAX are set by premake.

// --- volk ---------------------------------------------------------------
// volk defines VK_NO_PROTOTYPES itself and pulls in <vulkan/vulkan.h>.
// VOLK_IMPLEMENTATION emits the function-pointer definitions and the loader
// (volkInitialize / volkLoadInstance / volkLoadDevice), so the SDK's volk.c is
// not needed -- this header-only mode replaces it.
#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>

// --- VMA ----------------------------------------------------------------
// volk.h is included ABOVE, so VOLK_HEADER_VERSION is defined and VMA compiles
// vmaImportVulkanFunctionsFromVolk. With VK_NO_PROTOTYPES set, VMA does not try
// to statically link Vulkan entry points -- it takes them from the pointers we
// hand it at runtime (filled from volk).
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

// ---  End of File ---
