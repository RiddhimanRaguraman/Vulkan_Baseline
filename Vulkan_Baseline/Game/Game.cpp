//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "Game.h"

namespace Neelam
{
	Game::Game()
		: Engine()
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
		Trace::out("Game: LoadContent\n");
	}

	//-----------------------------------------------------------------
	// UnloadContent -- called once, before Vulkan is torn down.
	//   Later: destroy whatever LoadContent created (reverse order).
	//-----------------------------------------------------------------
	void Game::UnloadContent()
	{
		Trace::out("Game: UnloadContent\n");
	}

	//-----------------------------------------------------------------
	// Update -- once per frame, before Render. deltaTime is in seconds.
	//   Later: walk the scene graph updating each node (input, animation,
	//   transforms, camera).
	//-----------------------------------------------------------------
	void Game::Update(float deltaTime)
	{
		AZUL_UNUSED_VAR(deltaTime);
		// no-op until there is content to update
	}

	//-----------------------------------------------------------------
	// Render -- once per frame, after Update.
	//   Later: begin the command buffer / render pass (this also clears the
	//   swapchain), walk the scene graph submitting draws, then present.
	//-----------------------------------------------------------------
	void Game::Render()
	{
		// no-op until there is a pipeline to draw with
	}
}

// ---  End of File ---
