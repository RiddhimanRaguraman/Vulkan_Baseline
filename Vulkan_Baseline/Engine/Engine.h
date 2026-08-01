//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef ENGINE_H
#define ENGINE_H

#include "Window.h"
#include "Instance.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "QueueFamily.h"
#include "LogicalDevice.h"
#include "Swapchain.h"
#include "GraphicsPipeline.h"

#include "QueueMan.h"
#include "FileThread.h"

#include "AnimTimer.h"

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
		vk::Window			 window;
		vk::Instance		 instance;
		vk::Surface			 surface;
		vk::PhysicalDevice   physicalDevice;
		vk::QueueFamily		 queueFamily;
		vk::LogicalDevice	 logicalDevice;
		vk::Swapchain        swapchain;
		vk::GraphicsPipeline graphicsPipeline;	// the frame loop; Game::Render drives it

	private:
		// Rebuild the swapchain in place at the window's current client size
		// (called when the graphics pipeline reports it went out of date).
		void privRecreateSwapchain();

		// Engine-thread side of the actor model: pop commands posted by worker
		// threads and Execute() them HERE, where Vulkan calls are legal (§9).
		// Lives in Engine, not Game, so every Game gets it without asking.
		void privDrainCommands();

		// The async-load actor. Engine owns it so its lifetime brackets every
		// LoadContent/UnloadContent, and so a hosted (WPF) Game gets it too.
		FileThread      privFileThread;

		Azul::AnimTimer privFrameTimer;
		bool            privInitialized;

		// dt clamp: stops the delta exploding when execution is paused on a
		// breakpoint (matches the DX11 start point's maxTimeStep).
		static const float privMaxTimeStep;

		// Cap on commands executed per frame. Bounded so a flood of posts can
		// never stall a frame -- the leftovers just run next frame. (The audio
		// engine this came from pops exactly ONE per update, which silently
		// caps throughput at one message per frame; §18.)
		static const uint32_t privMaxCommandsPerFrame = 8;
	};
}

#endif   // ENGINE_H

// ---  End of File ---
