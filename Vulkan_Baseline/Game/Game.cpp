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
		// The pipeline bakes in the swapchain's color + depth formats.
		this->triangleShader.Create(this->logicalDevice.GetDevice(),
									this->swapchain.GetColorFormat(),
									this->swapchain.GetDepthFormat());

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
	//   The whole frame loop lives in GraphicsPipeline (owned by Engine); Game
	//   just hands it what to draw. Later this becomes a draw list / scene-graph
	//   walk instead of a single hard-coded technique.
	//-----------------------------------------------------------------
	void Game::Render()
	{
		this->graphicsPipeline.Render(this->triangleShader);
	}
}

// ---  End of File ---
