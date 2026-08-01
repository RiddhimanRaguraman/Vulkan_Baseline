//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef SHADER_WATCHER_H
#define SHADER_WATCHER_H

#include "ShaderObject.h"		// Name + the two source paths, read on this thread

// std::thread / mutex / atomic come from Framework.h. Nothing else is needed:
// the mailbox is a CircularData owned by QueueMan now, so <queue> is gone --
// this was the app's last STL container (§2, §18).

//---------------------------------------------------------------------------
// class ShaderWatcher   (an actor)
//
// A background thread that watches a folder of .hlsl files and, when one is
// edited, POSTS a File_CompileShader_Cmd to the FILE thread's inbox.
//
// Three actors, one direction:
//   Watcher  detects the change, posts the shader NAME + source paths
//   File     reads + DXC-compiles to SPIR-V (the slow part), posts the blobs
//   Engine   vkCreateShaderModule + pipeline rebuild -- the only Vulkan step
//
// The split is the point: the compile costs tens of milliseconds and used to
// stall a frame, while ALL Vulkan work belongs to the engine thread (§9). This
// thread does neither -- it only detects and posts.
//
// It no longer owns a mailbox. Inboxes belong to the CONSUMER (QueueMan), which
// is what lets other actors post to the engine later without touching this
// class.
//
// Change detection is timestamp polling, robust against editors that save
// twice. It is internal to the thread; swapping in ReadDirectoryChangesW
// (§18 -- blocks until the OS reports a change, instead of waking 4x/second)
// would not touch anything outside this file.
//
// Namespace is Neelam, not Neelam::vk: this is threading plumbing, not a
// Vulkan object (§1).
//---------------------------------------------------------------------------

namespace Neelam
{
	class ShaderWatcher
	{
	public:
		ShaderWatcher();
		ShaderWatcher(const ShaderWatcher &) = delete;
		ShaderWatcher &operator = (const ShaderWatcher &) = delete;
		~ShaderWatcher();

		// Launch the watcher thread on pDir (polls *.hlsl there). pShader is
		// the technique a detected change should rebuild -- it is only ever
		// stored into the posted command, never dereferenced on this thread.
		void Start(const char *pDir, vk::ShaderObject *pShader);

		// Stop and join. Safe to call more than once.
		void Stop();

	private:
		void privThreadMain();
		static unsigned long long privNewestWriteTime(const char *pDir);

		std::thread        privThread;
		std::atomic<bool>  privRunning;
		char               privDir[512];
		unsigned long long privLastSeen;

		vk::ShaderObject  *poShader;		// borrowed; see Start()
	};
}

#endif   // SHADER_WATCHER_H

// ---  End of File ---
