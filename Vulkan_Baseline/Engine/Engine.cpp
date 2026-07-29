//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "Engine.h"

namespace Neelam
{
	// 1/30s. If a frame takes longer than this (e.g. stopped on a breakpoint),
	// the delta is capped here so nothing teleports on the next Update.
	const float Engine::privMaxTimeStep = 1.0f / 30.0f;

	Engine::Engine()
		: window(),
		  instance(),
		  surface(),
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

		// Hand off to the game to load its content.
		this->LoadContent();

		// Start the frame clock so the first Tic() gets a sane delta.
		this->privFrameTimer.Tic();
		this->privInitialized = true;

		Trace::out("Engine: initialized (%s)\n", hParentWnd ? "editor / child" : "standalone");
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

		this->Update(deltaTime);	// -> Game
		this->Render();				// -> Game
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

		this->UnloadContent();	// -> Game

		// Reverse of Initialize: the surface is created FROM the instance, so
		// it must go first. Each Destroy is idempotent.
		this->surface.Destroy();
		this->instance.Destroy();
		this->window.Destroy();

		this->privInitialized = false;

		Trace::out("Engine: shutdown clean\n");
	}

	HWND Engine::GetWindowHandle() const
	{
		return this->window.GetHandle();
	}
}

// ---  End of File ---
