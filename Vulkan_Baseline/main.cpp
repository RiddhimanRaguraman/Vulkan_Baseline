//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "MathEngine.h"

#include "Window.h"
#include "VulkanInstance.h"
#include "VulkanSurface.h"

int main()
{
	Azul::Vec3 v(1.4f, 1.5f, 1.6f);
	Trace::out("%f ,%f, %f", v.x(), v.y(), v.z());
	Trace::out("\nHello World\n");

	Azul::Window window;
	if (!window.Create("Vulkan Baseline", 1280, 720))
	{
		return 1;
	}

	// No status check on these two -- vkAssert() inside asserts and exits the
	// process on a failed VkResult, so reaching the next line means success.
	Azul::VulkanInstance instance;
	instance.Create("Vulkan Baseline");

	// Hands the window's HWND to Vulkan -- the Win32-specific call.
	Azul::VulkanSurface surface;
	surface.Create(instance.GetInstance(), window.GetModule(), window.GetHandle());

	Trace::out("Vulkan surface created -- entering message loop\n");

	while (window.ProcessMessages())
	{
		// per-frame work goes here
	}

	// Tear down in reverse order: the surface is created FROM the instance,
	// so it has to go first. (Each Destroy is safe to call twice -- the
	// destructors run them again as a backstop.)
	surface.Destroy();
	instance.Destroy();
	window.Destroy();

	Trace::out("Shutdown clean\n");

	return 0;
}
