//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef QUEUE_MAN_H
#define QUEUE_MAN_H

#include "CircularData.h"

namespace Neelam
{
	//-----------------------------------------------------------------------
	// class QueueMan  (singleton)
	//
	// Owns one INBOX per consumer thread and routes commands into it. The
	// inbox belongs to the CONSUMER, not the producer -- that is what lets any
	// number of threads post to the engine without knowing anything about it.
	//
	// (The old ShaderWatcher mailbox had this backwards: the producer owned the
	// queue, so only that one producer could ever feed the engine.)
	//
	// One inbox today -- the engine thread. Actors get added as they appear:
	// SendFile / GetFileInQueue with the FileThread (§18 phase 3), and a game
	// inbox with the engine/game split (phase 4). Queues are NOT created for
	// threads that do not exist yet.
	//
	// Lifetime is Engine::Initialize -> Engine::Shutdown, which brackets every
	// worker thread: the watcher is started in Game::LoadContent and stopped in
	// Game::UnloadContent, both inside that window.
	//-----------------------------------------------------------------------
	class QueueMan
	{
	public:
		static void Create();

		// Drains and DELETES anything still queued -- commands are heap
		// allocated and their Execute() never ran, so this is the difference
		// between a clean run and Memory Tracking: FAIL (§13).
		static void Destroy();

		// Post to the engine thread. Returns false if the inbox is full, in
		// which case the command was NOT stored and the caller must delete it.
		static bool SendEngine(Command *pCmd);

		// Post to the file thread. Same contract as SendEngine, and it WAKES
		// the file thread -- which blocks rather than spinning.
		static bool SendFile(Command *pCmd);

		// For the consumers' own drain loops.
		static CircularData *GetEngineInQueue();
		static CircularData *GetFileInQueue();

		//-----------------------------------------------------------------
		// File-thread sleep/wake. The CV lives here, next to the queue it
		// guards, so FileThread does not have to own synchronization it does
		// not otherwise need.
		//
		// Only the FILE thread gets this treatment: it is idle most of the
		// time, so blocking saves a whole core. The ENGINE thread is never
		// idle -- it renders every frame -- so it drains by polling and a CV
		// would buy nothing (§18).
		//-----------------------------------------------------------------
		static void WaitForFile(const std::atomic<bool> &rRunning);
		static void WakeFile();

	private:
		QueueMan();
		QueueMan(const QueueMan &) = delete;
		QueueMan &operator = (const QueueMan &) = delete;
		~QueueMan();

		static QueueMan *privGetInstance();
		static QueueMan *posInstance;

		CircularData *poEngineIn;		// ---> to the engine thread
		CircularData *poFileIn;			// ---> to the file thread

		// Guards nothing but the wait itself -- CircularData has its own mutex.
		std::mutex              fileMtx;
		std::condition_variable fileCv;
		bool                    fileSignalled;
	};
}

#endif   // QUEUE_MAN_H

// ---  End of File ---
