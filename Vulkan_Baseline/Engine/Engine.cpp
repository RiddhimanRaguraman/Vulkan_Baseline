//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "Engine.h"

// CircularData only FORWARD-declares Command (it stores pointers), so the full
// definition is needed here to call Execute() through one.
#include "Command.h"

namespace Neelam
{
	// 1/30s. If a frame takes longer than this (e.g. stopped on a breakpoint),
	// the delta is capped here so nothing teleports on the next Update.
	const float Engine::privMaxTimeStep = 1.0f / 30.0f;

	Engine::Engine()
		: window(),
		  instance(),
		  surface(),
		  physicalDevice(),
		  queueFamily(),
		  logicalDevice(),
		  swapchain(),
		  graphicsPipeline(),
		  privFrameTimer(),
		  privInitialized(false)
	{
	}

	//-----------------------------------------------------------------
	// Initialize
	//-----------------------------------------------------------------
	void Engine::Initialize(HWND hParentWnd)
	{
		// Window: a child inside the editor if a parent HWND was handed in,
		// otherwise a standalone top-level window.
		if (hParentWnd != nullptr)
		{
			this->window.CreateChild(hParentWnd, 1280, 720);
		}
		else
		{
			this->window.Create("Neelam Engine", 1280, 720);
		}

		// Vulkan bring-up. vkAssert inside these asserts + exits the process on
		// a failed VkResult, so there is no status to branch on here.
		this->instance.Create("Neelam Engine");
		this->surface.Create(this->instance.GetInstance(),
							 this->window.GetModule(),
							 this->window.GetHandle());

		// Pick the GPU. Needs the surface too, so it can verify the surface
		// supports the format the swapchain will later request.
		this->physicalDevice.Create(this->instance.GetInstance(),
									this->surface.GetSurface());

		// Find the graphics+present queue family index on that GPU. This is a
		// selection only -- the actual VkQueue is obtained after the logical
		// device exists (vkGetDeviceQueue), just below.
		this->queueFamily.Create(this->physicalDevice.GetPhysicalDevice(),
								 this->surface.GetSurface());

		// Create the logical device: register the graphics+present queue, then
		// build. After this, GetQueue(...) hands back the real VkQueue.
		this->logicalDevice.Add(this->queueFamily.GetGraphicsFamilyIndex());
		this->logicalDevice.Create(this->physicalDevice.GetPhysicalDevice());

		// VMA memory allocator -- built from instance + GPU + device. Must come
		// after the device (volk's device pointers are loaded by now). It is a
		// framework singleton, so there is no member to own: the Engine only
		// decides WHEN it is built and torn down.
		vk::VulkanAllocator::Create(this->instance.GetInstance(),
									this->physicalDevice.GetPhysicalDevice(),
									this->logicalDevice.GetDevice());

		// Swapchain: the images we render into and present. Depth buffer is
		// allocated through VMA, so this comes after the allocator. 1280x720 is
		// a hint -- the surface's currentExtent decides the real size.
		this->swapchain.Create(this->physicalDevice.GetPhysicalDevice(),
							   this->logicalDevice.GetDevice(),
							   this->surface.GetSurface(),
							   vk::VulkanAllocator::Get(),
							   1280, 720);

		// The frame loop: command buffers + sync, drawing into the swapchain.
		this->graphicsPipeline.Create(this->logicalDevice.GetDevice(),
									  this->logicalDevice.GetQueue(this->queueFamily.GetGraphicsFamilyIndex()),
									  this->queueFamily.GetGraphicsFamilyIndex(),
									  &this->swapchain);

		// Command inboxes. Must exist BEFORE LoadContent, because that is where
		// the game starts its worker threads (the ShaderWatcher) and they post
		// as soon as they are running.
		QueueMan::Create();

		// The async worker. Started after the inboxes exist so it always has a
		// queue to block on.
		//
		// The Engine owns the THREAD; it does not own the technique registry --
		// ShaderObjects are content, so Game::LoadContent creates and destroys
		// ShaderObjectNodeMan, same as it does CameraNodeMan.
		this->privFileThread.Start();

		// Hand off to the game to load its content.
		this->LoadContent();

		// Start the frame clock so the first Tic() gets a sane delta.
		this->privFrameTimer.Tic();
		this->privInitialized = true;

		Debug::out("Engine: initialized (%s)\n", hParentWnd ? "editor / child" : "standalone");
	}

	//-----------------------------------------------------------------
	// Tic -- one frame. Editor calls this from its render callback; Run()
	// calls it from the standalone loop.
	//-----------------------------------------------------------------
	void Engine::Tic()
	{
		// Seconds elapsed since the previous Tic().
		const Azul::AnimTime elapsed = this->privFrameTimer.Toc();
		this->privFrameTimer.Tic();

		float deltaTime = elapsed / Azul::AnimTime(Azul::AnimTime::Duration::ONE_SECOND);

		if (deltaTime > Engine::privMaxTimeStep)
		{
			deltaTime = Engine::privMaxTimeStep;
		}

		// If the last frame found the swapchain out of date (resize/minimize),
		// rebuild it before drawing again.
		if (this->graphicsPipeline.IsSwapchainStale())
		{
			this->privRecreateSwapchain();
			this->graphicsPipeline.ClearSwapchainStale();
		}

		// Actor-model inbox. Drained BEFORE Update so a shader reload posted
		// last frame is live for this one's Render.
		this->privDrainCommands();

		this->Update(deltaTime);	// -> Game
		this->Render();				// -> Game (drives graphicsPipeline.Render)
	}

	//-----------------------------------------------------------------
	// Execute commands posted by worker threads. This is the ONE point where
	// another thread's work touches engine state, which is exactly what makes
	// "all Vulkan on the engine thread" (§9) true and checkable.
	//
	// It is a poll, deliberately. The check is an uncontended mutex lock
	// (tens of ns against a 16ms frame); interrupting the frame instead would
	// let a reload free a VkPipeline the in-flight command buffer already
	// recorded. See §18.
	//-----------------------------------------------------------------
	void Engine::privDrainCommands()
	{
		CircularData *pInbox = QueueMan::GetEngineInQueue();
		Command      *pCmd   = nullptr;

		for (uint32_t i = 0; i < Engine::privMaxCommandsPerFrame; i++)
		{
			if (!pInbox->PopFront(pCmd))
			{
				break;
			}

			assert(pCmd);
			pCmd->Execute();		// the command deletes itself
		}
	}

	//-----------------------------------------------------------------
	// Rebuild the swapchain in place at the window's current client size. The
	// GraphicsPipeline borrows the swapchain by pointer and re-queries it each
	// frame, so it transparently picks up the new images/views/semaphores.
	//-----------------------------------------------------------------
	void Engine::privRecreateSwapchain()
	{
		RECT rect = {};
		GetClientRect(this->window.GetHandle(), &rect);
		const uint32_t width  = (uint32_t)(rect.right - rect.left);
		const uint32_t height = (uint32_t)(rect.bottom - rect.top);

		// Minimized (0-size) -- nothing to build; try again next frame.
		if (width == 0 || height == 0)
		{
			return;
		}

		vkDeviceWaitIdle(this->logicalDevice.GetDevice());

		this->swapchain.Destroy();
		this->swapchain.Create(this->physicalDevice.GetPhysicalDevice(),
							   this->logicalDevice.GetDevice(),
							   this->surface.GetSurface(),
							   vk::VulkanAllocator::Get(),
							   width, height);
	}

	//-----------------------------------------------------------------
	// Run -- standalone loop only.
	//-----------------------------------------------------------------
	void Engine::Run()
	{
		// ProcessMessages drains the queue and returns false on WM_QUIT.
		while (this->window.ProcessMessages())
		{
			this->Tic();
		}

		this->Shutdown();
	}

	//-----------------------------------------------------------------
	// Shutdown 
	//-----------------------------------------------------------------
	void Engine::Shutdown()
	{
		// Guard so double-shutdown (e.g. Run() then the destructor path, or a
		// second call from C#) is a no-op.
		if (!this->privInitialized)
		{
			return;
		}

		// Wait for the GPU to finish EVERYTHING before we start destroying the
		// objects it used. Without this, UnloadContent() destroys the pipeline
		// while the last frame's command buffer still references it -- a
		// validation error (VUID-vkDestroyPipeline-pipeline-00765).
		vkDeviceWaitIdle(this->logicalDevice.GetDevice());

		this->UnloadContent();	// -> Game (stops the watcher, destroys content)

		// SHUTDOWN ORDER MATTERS -- this is what makes the actor chain safe
		// without any handle/weak-pointer machinery:
		//
		//   1. UnloadContent stopped the ShaderWatcher, so nothing new can be
		//      posted to the file thread. It also dropped the technique
		//      registry -- safe, because from here on NO command is ever
		//      EXECUTED, only deleted (steps 2 and 3).
		//   2. Stop the file thread: joins it, then drains + deletes whatever
		//      it never got to. After this, nothing can post to the engine.
		//   3. QueueMan::Destroy drains + deletes the engine inbox. Anything a
		//      compile posted after step 1 dies here WITHOUT executing -- which
		//      is why it can never touch an already-destroyed technique.
		this->privFileThread.Stop();

		QueueMan::Destroy();

		// Reverse of Initialize. The physical device is only a borrowed handle
		// (nothing to release), but tear down in reverse order for discipline:
		// surface is created FROM the instance, so surface before instance.

		// Frame loop first -- it waits for the GPU to go idle, then frees its
		// command buffers + sync objects.
		this->graphicsPipeline.Destroy();

		// Swapchain uses the device + allocator (depth image).
		this->swapchain.Destroy();

		// The allocator lives on the device, so it must go before the device.
		vk::VulkanAllocator::Destroy();

		// The logical device owns real GPU state, so it must go before the
		// things it was built from.
		this->logicalDevice.Destroy();
		this->queueFamily.Destroy();
		this->physicalDevice.Destroy();
		this->surface.Destroy();
		this->instance.Destroy();
		this->window.Destroy();

		this->privInitialized = false;

		Debug::out("Engine: shutdown clean\n");
	}

	HWND Engine::GetWindowHandle() const
	{
		return this->window.GetHandle();
	}
}

// ---  End of File ---
