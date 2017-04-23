#pragma once

#include <Windows.h>
#include <string>

#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "dxgi.lib")

typedef std::basic_string<TCHAR> tstring;

template<class T>
inline void SafeRelease(T** ppObj)
{
	if (*ppObj != nullptr)
	{
		(*ppObj)->Release();
		*ppObj = nullptr;
	}
}

inline void SafeCloseHandle(HANDLE* pHandle)
{
	if (*pHandle != nullptr)
	{
		CloseHandle(*pHandle);
		*pHandle = nullptr;
	}
}

template<class T>
inline void SafeDelete(T** ppObj)
{
	if (*ppObj != nullptr)
	{
		delete *ppObj;
		*ppObj = nullptr;
	}
}

template<class T>
inline void SafeDeleteArray(T** ppObjs)
{
	if (*ppObjs != nullptr)
	{
		delete[] *ppObjs;
		*ppObjs = nullptr;
	}
}

inline
tstring GetLastErrorMessage(HRESULT hr = 0)
{
	if (hr == 0)
	{
		hr = GetLastError();
	}

	LPVOID msg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg, 0, nullptr);
	std::string msgStr((LPTSTR)msg);
	LocalFree(msg);
	return msgStr;
}

template<class T = std::exception>
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		auto err = GetLastErrorMessage(hr);
		throw T(err.c_str());
	}
}