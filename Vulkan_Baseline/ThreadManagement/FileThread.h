//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef FILE_THREAD_H
#define FILE_THREAD_H

//---------------------------------------------------------------------------
// class FileThread   (actor)
//
// The first real worker: pops commands from its own inbox and Execute()s them
// on this thread. Today that is HLSL -> SPIR-V compiles, which are tens of
// milliseconds of disk + DXC that used to stall a frame during hot-reload.
//
// It does no Vulkan. Work that needs the GPU is posted BACK to the engine
// thread as a ShaderModulesReady_Cmd (§9, §18).
//
// Unlike the reference audio engine, this does NOT busy-spin on its queue --
// that burns a core doing nothing. It blocks on a condition_variable and is
// woken by QueueMan::SendFile, so an idle file thread costs zero CPU. This is
// where a CV genuinely pays: the file thread IS idle most of the time, whereas
// the engine thread never is (which is why the engine drains by polling).
//---------------------------------------------------------------------------

namespace Neelam
{
	class FileThread
	{
	public:
		FileThread();
		FileThread(const FileThread &) = delete;
		FileThread &operator = (const FileThread &) = delete;
		~FileThread();

		void Start();

		// Signals quit, wakes the thread, joins it, then drains + deletes
		// anything still queued. Must run BEFORE QueueMan::Destroy, or this
		// thread could post into an inbox that is being torn down.
		void Stop();

	private:
		void privThreadMain();

		std::thread       privThread;
		std::atomic<bool> privRunning;
	};
}

#endif   // FILE_THREAD_H

// ---  End of File ---
