#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <vector>

class Device;
class CommandQueue;

struct ScreenContextDesc
{
	int BufferCount;
	DXGI_FORMAT Format;
	int Width;
	int Height;
	HWND OutputWindow;
	bool Windowed;

	ScreenContextDesc()
		: BufferCount(2),
		Format(DXGI_FORMAT_UNKNOWN),
		Width(0), Height(0),
		OutputWindow(nullptr),
		Windowed(true)
	{}
};

class ScreenContext
{
public:
	ScreenContext();
	~ScreenContext();

	UINT FrameIndex() { return frameIndex_; }
	const ScreenContextDesc& Desc() { return desc_; }

	void Create(Device* pDevice, CommandQueue* pCommandQueue, const ScreenContextDesc& desc);
	void UpdateFrameIndex();
	void SwapBuffers();

public: // internal
	HRESULT GetBackBufferView(UINT index, ID3D12Resource** ppView);

private:
	Device* pDevice_;

	ScreenContextDesc desc_;
	IDXGISwapChain3* pSwapChain_;
	
	UINT frameIndex_;
};

