#include "Window.h"


namespace
{
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_DESTROY)
		{
			PostQuitMessage(0);
			return S_OK;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

Window::Window()
	: mHandle(nullptr)
{}


Window::~Window()
{
	Close();
}


HRESULT Window::Setup(HINSTANCE hInstance, LPCTSTR title)
{
	if (mHandle != nullptr)
	{
		Close();
	}

	HRESULT result;

	WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW,
		WindowProc,
		0, 0,
		hInstance,
		LoadIcon(hInstance, IDI_APPLICATION),
		LoadCursor(nullptr, IDC_ARROW),
		(HBRUSH)COLOR_WINDOW,
		nullptr,
		title,
		LoadIcon(hInstance, IDI_APPLICATION)
	};

	result = RegisterClassEx(&wcex);
	if (FAILED(result))
	{
		return result;
	}

	mHandle = CreateWindow(
		title,
		title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr,
		nullptr,
		hInstance,
		nullptr);
	if (mHandle == nullptr)
	{
		return S_FALSE;
	}

	mInstanceHandle = hInstance;
	mTitle = title;

	return S_OK;
}

HRESULT Window::Move(int x, int y)
{
	RECT rect;
	if (!GetWindowRect(mHandle, &rect))
	{
		return S_FALSE;
	}

	if (!MoveWindow(mHandle, x, y, rect.right - rect.left, rect.bottom - rect.top, TRUE))
	{
		return S_FALSE;
	}

	return S_OK;
}

HRESULT Window::Resize(int width, int height)
{
	RECT rect;
	if (!GetWindowRect(mHandle, &rect))
	{
		return S_FALSE;
	}

	if (!MoveWindow(mHandle, rect.left, rect.top, width, height, TRUE))
	{
		return S_FALSE;
	}

	return S_OK;
}


HRESULT Window::Open(int nCmdShow)
{
	if (!ShowWindow(mHandle, nCmdShow))
	{
		return S_FALSE;
	}
	return S_OK;
}


HRESULT Window::Close()
{
	if (mHandle == nullptr)
	{
		return S_OK;
	}

	if (!UnregisterClass(mTitle, mInstanceHandle))
	{
		return S_FALSE;
	}

	if (!CloseWindow(mHandle))
	{
		return S_FALSE;
	}

	mHandle = nullptr;
	mInstanceHandle = nullptr;
	mTitle = nullptr;

	return S_OK;
}

