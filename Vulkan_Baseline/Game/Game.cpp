//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "Game.h"

namespace Neelam
{
	Game::Game()
		: Engine(),
		  triangleShader(),
		  shaderWatcher()
	{
	}

	Game::~Game()
	{
	}

	//-----------------------------------------------------------------
	// LoadContent -- called once, after the window + Vulkan are up.
	//   Later: create GameObjects, cameras, and insert them into a scene
	//   graph (the PCSTree). Load buffers / shaders / pipeline.
	//-----------------------------------------------------------------
	void Game::LoadContent()
	{
		Debug::out("Game: LoadContent\n");

		// Build the triangle technique (compiles HLSL -> SPIR-V -> pipeline).
		// The swapchain's color format is what the pipeline renders into.
		this->triangleShader.Create(this->logicalDevice.GetDevice(),
									this->swapchain.GetColorFormat());

		// Start the background watcher on the shader folder. From here on,
		// saving a .hlsl posts a message that Update() picks up (see below).
		this->shaderWatcher.Start(SOLUTION_DIR "Vulkan_Baseline\\Shader\\hlsl");
	}

	//-----------------------------------------------------------------
	// UnloadContent -- called once, before Vulkan is torn down.
	//   Later: destroy whatever LoadContent created (reverse order).
	//-----------------------------------------------------------------
	void Game::UnloadContent()
	{
		Debug::out("Game: UnloadContent\n");

		// Stop the watcher thread before tearing the shader down.
		this->shaderWatcher.Stop();
		this->triangleShader.Destroy();
	}

	//-----------------------------------------------------------------
	// Update -- once per frame, before Render. deltaTime is in seconds.
	//   Later: walk the scene graph updating each node (input, animation,
	//   transforms, camera).
	//-----------------------------------------------------------------
	void Game::Update(float deltaTime)
	{
		AZUL_UNUSED_VAR(deltaTime);

		// Drain the watcher's mailbox (engine thread side of the actor split).
		// If a .hlsl was edited, rebuild the pipeline from the new source.
		if (this->shaderWatcher.Drain())
		{
			this->triangleShader.Reload();
		}
	}

	//-----------------------------------------------------------------
	// Render -- once per frame, after Update.
	//   Later: begin the command buffer / render pass (this also clears the
	//   swapchain), walk the scene graph submitting draws, then present.
	//-----------------------------------------------------------------
	void Game::Render()
	{
		// The pipeline exists (triangleShader), but there is no command buffer
		// / frame loop yet -- so nothing is drawn to the window this step. Next
		// step: acquire a swapchain image, begin dynamic rendering,
		// triangleShader.SetActive(cmd), vkCmdDraw(cmd, 3, 1, 0, 0), present.
	}
}

// ---  End of File ---
