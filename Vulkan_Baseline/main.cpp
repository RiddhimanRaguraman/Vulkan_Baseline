//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "MathEngine.h"

#include "Game.h"

//---------------------------------------------------------------------------
// Standalone entry point.
//
// Creates the Game, brings it up with its own top-level window (nullptr parent
// == standalone), and hands the frame loop to the Engine until the window is
// closed. Shutdown() runs inside Run().
//
// The C# / WPF editor will NOT go through here: it links the engine as a DLL
// and calls Initialize(hwnd) / Tic() / Shutdown() itself. This main() is the
// standalone-runnable path used for development and testing.
//---------------------------------------------------------------------------

int main()
{
	// Names this (the main / engine) thread and starts the thread framework's
	// Debug system, so every Debug::out below shows which thread it came from.
	// The engine loop runs right here on this thread -- hence "EngineThread".
	START_BANNER_MAIN("EngineThread");

	// Quick sanity ping through the Azul math library (a lib, still Azul::).
	Azul::Vec3 v(1.4f, 1.5f, 1.6f);
	Debug::out("%f ,%f, %f\n", v.x(), v.y(), v.z());

	Neelam::Game game;

	game.Initialize();		// nullptr parent -> standalone top-level window
	game.Run();				// pumps + Update/Render until WM_QUIT, then Shutdown

	return 0;
}

// ---  End of File ---
