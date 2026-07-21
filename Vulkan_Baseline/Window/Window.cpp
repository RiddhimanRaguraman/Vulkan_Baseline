//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "Window.h"

namespace Azul
{
	const char *const Window::privClassName = "AzulVulkanWindow";

	Window::Window()
		: privHandle(nullptr),
		  privModule(nullptr),
		  privClassRegistered(false)
	{
	}

	Window::~Window()
	{
		this->Destroy();
	}

	bool Window::Create(const char *pTitle, int width, int height)
	{
		this->privModule = GetModuleHandle(nullptr);

		WNDCLASSEX wc = {};
		wc.cbSize        = sizeof(WNDCLASSEX);
		wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc   = Window::privWndProc;
		wc.hInstance     = this->privModule;
		wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszClassName = Window::privClassName;

		if (RegisterClassEx(&wc) == 0)
		{
			Trace::out("Window: RegisterClassEx failed (%lu)\n", GetLastError());
			return false;
		}
		this->privClassRegistered = true;

		// CreateWindowEx takes the OUTER size, so grow the requested client
		// area by however much the chosen style's border needs.
		RECT rect;
		rect.left   = 0;
		rect.top    = 0;
		rect.right  = width;
		rect.bottom = height;
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

		this->privHandle = CreateWindowEx(
			0,
			Window::privClassName,
			pTitle,
			WS_OVERLAPPEDWINDOW,//WS_POPUP WS_OVERLAPPEDWINDOW
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			rect.right - rect.left,
			rect.bottom - rect.top,
			nullptr,
			nullptr,
			this->privModule,
			nullptr);

		if (this->privHandle == nullptr)
		{
			Trace::out("Window: CreateWindowEx failed (%lu)\n", GetLastError());
			return false;
		}

		ShowWindow(this->privHandle, SW_SHOW);
		UpdateWindow(this->privHandle);

		return true;
	}

	void Window::Destroy()
	{
		if (this->privHandle != nullptr)
		{
			DestroyWindow(this->privHandle);
			this->privHandle = nullptr;
		}

		if (this->privClassRegistered)
		{
			UnregisterClass(Window::privClassName, this->privModule);
			this->privClassRegistered = false;
		}
	}

	bool Window::ProcessMessages()
	{
		MSG msg;

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return false;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return true;
	}

	HWND Window::GetHandle() const
	{
		return this->privHandle;
	}

	HINSTANCE Window::GetModule() const
	{
		return this->privModule;
	}

	LRESULT CALLBACK Window::privWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_CLOSE:
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		default:
			break;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// ---  End of File ---
