#pragma once

#include <DirectXMath.h>
#include <Windows.h>
#include <string>

#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "dxgi.lib")

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;

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

template<class T>
inline void SafeDestroy(T** ppObj)
{
	if (*ppObj != nullptr)
	{
		(*ppObj)->Destroy();
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

template<class T>
inline void SafeDeleteSequence(T* pSeq)
{
	for (auto ptr : *pSeq)
	{
		SafeDelete(&ptr);
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

inline void XMFLoat3Normalize(DirectX::XMFLOAT3* pOut, const DirectX::XMFLOAT3* pIn)
{
	auto v = DirectX::XMLoadFloat3(pIn);
	v = DirectX::XMVector3Normalize(v);
	DirectX::XMStoreFloat3(pOut, v);
}
