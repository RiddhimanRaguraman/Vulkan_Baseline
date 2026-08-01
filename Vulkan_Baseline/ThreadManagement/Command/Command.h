//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef COMMAND_H
#define COMMAND_H

//---------------------------------------------------------------------------
// class Command  (abstract)
//
// One unit of cross-thread work. A producer thread NEWs a concrete command and
// posts it to the consumer's inbox (QueueMan); the consumer pops it and calls
// Execute() ON ITS OWN THREAD. That is the entire actor model -- no thread ever
// touches another thread's data directly.
//
// OWNERSHIP -- the rule that keeps the leak tracker (§11) quiet:
//   * The CONSUMER owns the command. Concrete Execute() bodies end with
//     `delete this`.
//   * If the post FAILS (inbox full -> QueueMan::Send* returns false) the
//     command was never handed over, so the SENDER must delete it.
//   * Anything still queued at shutdown is never executed, so QueueMan::Destroy
//     drains and deletes the remainder.
// Miss any of the three and a clean run turns into Memory Tracking: FAIL.
//
// No Handle member yet. That arrives with the FileThread (§18 phase 3), where a
// completion command can outlive the resource it names; nothing here outlives
// anything, so carrying one now would be dead weight.
//---------------------------------------------------------------------------

namespace Neelam
{
	class Command
	{
	public:
		Command() = default;
		Command(const Command &) = delete;
		Command &operator = (const Command &) = delete;
		virtual ~Command() = default;

		// Runs on the CONSUMER's thread. Implementations end with `delete this`.
		virtual void Execute() = 0;
	};
}

#endif   // COMMAND_H

// ---  End of File ---
