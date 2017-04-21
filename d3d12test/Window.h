#pragma once
#include <Windows.h>

class Window
{
public:
	Window();
	~Window();

	HRESULT Setup(HINSTANCE hInstance, LPCTSTR title);
	HRESULT Move(int x, int y);
	HRESULT Resize(int width, int height);
	HRESULT Open(int nCmdShow = SW_SHOWNORMAL);
	HRESULT Close();

public:
	const HWND Handle() { return handle_; }

private:
	HWND handle_;
	HINSTANCE instanceHandle_;
	LPCTSTR title_;
};

