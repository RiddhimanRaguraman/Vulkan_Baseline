//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef WINDOW_H
#define WINDOW_H

//---------------------------------------------------------------------------
// class Window
//
// Owns a single Win32 window: registers the window class, creates the HWND
// and pumps its message queue. Nothing Vulkan lives in here -- the window
// only hands out the HWND / HINSTANCE that VulkanSurface needs to build a
// VkSurfaceKHR from.
//
// Windows.h is already pulled in by the force-included Framework.h.
//---------------------------------------------------------------------------

namespace Azul
{
	class Window
	{
	public:
		//-----------------------------------------------------------------
		// Constructors / Destructors
		//-----------------------------------------------------------------
		Window();
		Window(const Window &) = delete;
		Window &operator = (const Window &) = delete;
		~Window();

		//-----------------------------------------------------------------
		// Lifetime
		//-----------------------------------------------------------------

		// Registers the class and creates the window. width/height are the
		// CLIENT area size -- the border is added on top of that.
		bool Create(const char *pTitle, int width, int height);

		// Safe to call more than once; the destructor calls it too.
		void Destroy();

		//-----------------------------------------------------------------
		// Message pump
		//-----------------------------------------------------------------

		// Drains every pending message. Returns false once WM_QUIT arrives,
		// which is the signal to drop out of the main loop.
		bool ProcessMessages();

		//-----------------------------------------------------------------
		// Accessors
		//-----------------------------------------------------------------
		HWND GetHandle() const;
		HINSTANCE GetModule() const;

	private:
		static LRESULT CALLBACK privWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		// Name we register the window class under.
		static const char *const privClassName;

		// Data
		HWND      privHandle;
		HINSTANCE privModule;
		bool      privClassRegistered;
	};
}

#endif   // WINDOW_H

// ---  End of File ---
