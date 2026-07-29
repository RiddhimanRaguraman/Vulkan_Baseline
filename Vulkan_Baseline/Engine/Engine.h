//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef ENGINE_H
#define ENGINE_H

#include "Window.h"
#include "Instance.h"
#include "Surface.h"

#include "AnimTimer.h"		// Azul::AnimTimer -- the per-frame delta clock

//---------------------------------------------------------------------------
// class Engine  (Template Method base)
//
// Owns the window + the Vulkan objects (by composition) and the frame loop.
// Game derives from it and fills in the four content hooks -- the base calls
// DOWN into them through the vtable, so Engine never knows what the game is
// and Game never knows what a message pump is.
//
// Two ways to run:
//
//   Standalone .exe:
//       Game game;
//       game.Initialize();     // nullptr parent -> own top-level window
//       game.Run();            // Engine owns the loop until WM_QUIT
//
//   Hosted in the C# / WPF editor:
//       game.Initialize(hwndFromWpf);   // child window inside the editor
//       ... C# drives frames by calling Tick() from its render callback ...
//       game.Shutdown();
//       (hand GetWindowHandle() to WPF's HwndHost.BuildWindowCore)
//
// The public Initialize / Tick / Shutdown / GetWindowHandle map 1:1 to the
// extern "C" functions a future DLL build will export for P/Invoke -- keeping
// them free of C++ types in their signatures is deliberate.
//---------------------------------------------------------------------------

namespace Neelam
{
	class Engine
	{
	public:
		//-----------------------------------------------------------------
		// Constructors / Destructors
		//-----------------------------------------------------------------
		Engine();
		Engine(const Engine &) = delete;
		Engine &operator = (const Engine &) = delete;
		virtual ~Engine() = default;

		//-----------------------------------------------------------------
		// Lifecycle  (the DLL-export surface)
		//-----------------------------------------------------------------

		// Bring up window + Vulkan, then LoadContent().
		//   hParentWnd == nullptr -> standalone top-level window.
		//   hParentWnd != nullptr -> child window hosted in the editor's HWND.
		void Initialize(HWND hParentWnd = nullptr);

		// Advance exactly one frame: compute dt, Update(dt), Render().
		// Does NOT pump the message queue -- in the editor WPF owns the pump,
		// and standalone Run() pumps separately before calling this.
		void Tic();

		// UnloadContent() + tear down Vulkan and the window (reverse order).
		void Shutdown();

		// Standalone convenience: pump this window's own queue and Tic() until
		// WM_QUIT, then Shutdown(). Do not call this when hosted in WPF.
		void Run();

		//-----------------------------------------------------------------
		// Accessors
		//-----------------------------------------------------------------

		// The native handle the editor hosts. Valid after Initialize().
		HWND GetWindowHandle() const;

	protected:
		//-----------------------------------------------------------------
		// Content hooks -- Game overrides these.
		//-----------------------------------------------------------------
		virtual void LoadContent()           = 0;
		virtual void UnloadContent()         = 0;
		virtual void Update(float deltaTime) = 0;
		virtual void Render()                = 0;

		//-----------------------------------------------------------------
		// Infrastructure owned by composition. Protected so a derived Game
		// (and later a device/swapchain built here) can reach the instance
		// and surface.
		//-----------------------------------------------------------------
		vk::Window   window;
		vk::Instance instance;
		vk::Surface  surface;

	private:
		Azul::AnimTimer privFrameTimer;
		bool            privInitialized;

		// dt clamp: stops the delta exploding when execution is paused on a
		// breakpoint (matches the DX11 start point's maxTimeStep).
		static const float privMaxTimeStep;
	};
}

#endif   // ENGINE_H

// ---  End of File ---
