#pragma once

#include <Windows.h>
#include <string>

typedef std::basic_string<TCHAR> tstring;

inline
tstring GetLastErrorMessage()
{
	auto err = GetLastError();
	LPVOID msg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg, 0, nullptr);
	std::string msgStr((LPTSTR)msg);
	LocalFree(msg);
	return msgStr;
}

template<class T>
inline void SafeRelease(T** pObj)
{
	if (*pObj != nullptr)
	{
		(*pObj)->Release();
		*pObj = nullptr;
	}
}

template<class T = std::exception>
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		auto err = GetLastErrorMessage();
		throw T(err.c_str());
	}
}