//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef SHADER_WATCHER_H
#define SHADER_WATCHER_H

#include <queue>		// std::thread / mutex / atomic come from Framework.h

//---------------------------------------------------------------------------
// class ShaderWatcher   (the first "actor")
//
// A background thread that watches a folder of .hlsl files and, when one is
// edited, posts a message into a thread-safe MAILBOX. The ENGINE thread drains
// the mailbox each frame (Drain()) and does the actual reload -- so ALL Vulkan
// work stays on the engine thread and this thread only ever detects changes.
//
// That one-way "detect -> message -> consume" split is the seed of the actor
// model: when the engine-thread / game-thread split lands later, the mailbox
// generalizes to typed messages with handles; today it carries one signal.
//
// Change detection is timestamp polling (simple + robust against editors that
// save twice). It is internal to the thread -- swapping in ReadDirectoryChanges
// later would not touch the mailbox interface below.
//
// Lives in ThreadManagement/ with the rest of the threading + actor plumbing
// (mailboxes, handles) as that grows.
//---------------------------------------------------------------------------

namespace Neelam::vk
{
	// The message. Empty for now -- later it carries which shader / a handle.
	struct ShaderChangeMsg
	{
	};

	class ShaderWatcher
	{
	public:
		ShaderWatcher();
		ShaderWatcher(const ShaderWatcher &) = delete;
		ShaderWatcher &operator = (const ShaderWatcher &) = delete;
		~ShaderWatcher();

		// Launch the watcher thread on the given folder (polls *.hlsl there).
		void Start(const char *pDir);

		// Stop and join the thread. Safe to call more than once.
		void Stop();

		// Engine-thread side: pull the mailbox. Returns true if at least one
		// change was pending (all pending messages are consumed / coalesced).
		bool Drain();

	private:
		void privThreadMain();
		static unsigned long long privNewestWriteTime(const char *pDir);

		std::thread              privThread;
		std::atomic<bool>        privRunning;
		char                     privDir[512];
		unsigned long long       privLastSeen;

		// The mailbox.
		std::mutex                   privMailboxMtx;
		std::queue<ShaderChangeMsg>  privMailbox;
	};
}

#endif   // SHADER_WATCHER_H

// ---  End of File ---
