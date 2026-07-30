//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef GAME_H
#define GAME_H

#include "Engine.h"
#include "ShaderObject_ColorByVertex.h"
#include "ShaderWatcher.h"

//---------------------------------------------------------------------------
// class Game
//
// The concrete application. Derives from Engine and fills in the four content
// hooks. This is where objects, cameras and (later) a scene graph live -- the
// Engine's frame loop calls Update()/Render() here every Tic().
//
// Right now the hooks are stubs: there is no graphics pipeline yet, so the
// window simply comes up and the loop spins. Loading buffers, shaders and a
// swapchain, then drawing, all slot in behind these same four methods without
// the Engine loop or the window/instance/surface changing.
//---------------------------------------------------------------------------

namespace Neelam
{
	class Game : public Engine
	{
	public:
		Game();
		Game(const Game &) = delete;
		Game &operator = (const Game &) = delete;
		virtual ~Game();

		//-----------------------------------------------------------------
		// Engine content hooks
		//-----------------------------------------------------------------
		virtual void LoadContent()           override;
		virtual void UnloadContent()         override;
		virtual void Update(float deltaTime) override;
		virtual void Render()                override;

	private:
		// The triangle technique + the background watcher that hot-reloads it.
		vk::ShaderObject_ColorByVertex triangleShader;
		vk::ShaderWatcher              shaderWatcher;
	};
}

#endif   // GAME_H

// ---  End of File ---
