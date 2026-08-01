//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "ShaderWatcher.h"
#include "File_CompileShader_Cmd.h"
#include "QueueMan.h"

namespace Neelam
{
	ShaderWatcher::ShaderWatcher()
		: privThread(),
		  privRunning(false),
		  privDir{},
		  privLastSeen(0),
		  poShader(nullptr)
	{
	}

	ShaderWatcher::~ShaderWatcher()
	{
		this->Stop();
	}

	void ShaderWatcher::Start(const char *pDir, vk::ShaderObject *pShader)
	{
		assert(pDir);
		assert(pShader);

		strcpy_s(this->privDir, sizeof(this->privDir), pDir);
		this->poShader = pShader;

		// Baseline: whatever is newest right now is "already seen", so we only
		// fire on edits made AFTER Start().
		this->privLastSeen = ShaderWatcher::privNewestWriteTime(this->privDir);

		this->privRunning = true;
		this->privThread  = std::thread(&ShaderWatcher::privThreadMain, this);
	}

	void ShaderWatcher::Stop()
	{
		this->privRunning = false;
		if (this->privThread.joinable())
		{
			this->privThread.join();
		}
	}

	//-----------------------------------------------------------------
	// Background thread: poll the folder's newest write-time; on an increase,
	// post a reload command. Nothing Vulkan happens here.
	//-----------------------------------------------------------------
	void ShaderWatcher::privThreadMain()
	{
		// Register this thread's name + print a begin()/end() banner, so its
		// Debug::out lines are clearly tagged as the watcher thread (not the
		// engine thread).
		Debug::SetCurrentName("ShaderWatcher");
		SimpleBanner banner;

		Debug::out("watching '%s'\n", this->privDir);

		while (this->privRunning)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(250));

			const unsigned long long newest = ShaderWatcher::privNewestWriteTime(this->privDir);
			if (newest > this->privLastSeen)
			{
				this->privLastSeen = newest;

				Debug::out("change detected -> posting compile to FileThread\n");

				// Goes to the FILE thread, not the engine: the compile is the
				// expensive half (disk + DXC). It posts the resulting SPIR-V
				// back to the engine, which does the Vulkan half.
				//
				// The command carries the shader's NAME and its two source
				// paths BY VALUE -- never a ShaderObject*, because the engine
				// thread may destroy the technique while this is in flight
				// (§18). poShader is read here only, on this thread, and only
				// for those immutable strings.
				//
				// privNewestWriteTime returns the MAX across the folder, so a
				// multi-file save inside one 250ms tick is already one message.
				Command *pCmd = new File_CompileShader_Cmd(
					this->poShader->GetName(),
					this->poShader->GetVertexPath(),
					this->poShader->GetPixelPath());

				if (!QueueMan::SendFile(pCmd))
				{
					// Inbox full: never handed over, so it is still ours to
					// free. Dropping is fine -- the next save posts again.
					Debug::out("file inbox FULL -- reload dropped\n");
					delete pCmd;
				}
			}
		}
	}

	//-----------------------------------------------------------------
	// Newest last-write time across *.hlsl in pDir, as a comparable 64-bit
	// value (FILETIME). 0 if the folder cannot be read.
	//-----------------------------------------------------------------
	unsigned long long ShaderWatcher::privNewestWriteTime(const char *pDir)
	{
		char pattern[600] = {};
		sprintf_s(pattern, sizeof(pattern), "%s\\*.hlsl", pDir);

		WIN32_FIND_DATAA findData = {};
		HANDLE hFind = FindFirstFileA(pattern, &findData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			return 0;
		}

		unsigned long long newest = 0;
		do
		{
			ULARGE_INTEGER t;
			t.LowPart  = findData.ftLastWriteTime.dwLowDateTime;
			t.HighPart = findData.ftLastWriteTime.dwHighDateTime;
			if (t.QuadPart > newest)
			{
				newest = t.QuadPart;
			}
		} while (FindNextFileA(hFind, &findData));

		FindClose(hFind);
		return newest;
	}
}

// ---  End of File ---
