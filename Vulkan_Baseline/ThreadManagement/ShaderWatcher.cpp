//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "ShaderWatcher.h"

namespace Neelam::vk
{
	ShaderWatcher::ShaderWatcher()
		: privThread(),
		  privRunning(false),
		  privDir{},
		  privLastSeen(0),
		  privMailboxMtx(),
		  privMailbox()
	{
	}

	ShaderWatcher::~ShaderWatcher()
	{
		this->Stop();
	}

	void ShaderWatcher::Start(const char *pDir)
	{
		strcpy_s(this->privDir, sizeof(this->privDir), pDir);

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

	bool ShaderWatcher::Drain()
	{
		std::lock_guard<std::mutex> lock(this->privMailboxMtx);
		if (this->privMailbox.empty())
		{
			return false;
		}

		// Coalesce: several rapid edits collapse into one reload.
		while (!this->privMailbox.empty())
		{
			this->privMailbox.pop();
		}
		return true;
	}

	//-----------------------------------------------------------------
	// Background thread: poll the folder's newest write-time; on an increase,
	// post a message. Nothing Vulkan happens here.
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

				Debug::out("change detected -> posting reload message\n");

				std::lock_guard<std::mutex> lock(this->privMailboxMtx);
				this->privMailbox.push(ShaderChangeMsg{});
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
