#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <vector>

class Device;

struct ScreenViewDesc
{
	int BufferCount;
	DXGI_FORMAT Format;
	int Width;
	int Height;
	HWND OutputWindow;
	bool Windowed;

	ScreenViewDesc()
		: BufferCount(2),
		Format(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB),
		Width(0), Height(0),
		OutputWindow(nullptr),
		Windowed(true)
	{}
};

struct DepthStencilViewDesc
{
	DXGI_FORMAT Format;
	int Width;
	int Height;
	float ClearDepth;
	UINT8 ClearStencil;

	DepthStencilViewDesc()
		: Format(DXGI_FORMAT_D24_UNORM_S8_UINT),
		Width(0), Height(0),
		ClearDepth(1.0f), ClearStencil(0)
	{}
};

class ScreenContext
{
public:
	ScreenContext();
	~ScreenContext();

	IDXGISwapChain3* Get() { return pSwapChain_; }

	D3D12_CPU_DESCRIPTOR_HANDLE CurrentRtvHandle()
	{
		auto handle = pRtvHeap_->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += FrameIndex() * rtvDescriptorSize_;
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle(int index = 0)
	{
		auto handle = pDsvHeap_->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += index * dsvDescriptorSize_;
		return handle;
	}

	UINT FrameIndex() { return frameIndex_; }
	ID3D12Resource* RenderTargetView(UINT index) { return rtvViewPtrs_[index]; }

	void Create(Device* pDevice, ID3D12CommandQueue* pCommandQueue, const ScreenViewDesc& desc);

	HRESULT CreateRenderTargetViews();
	HRESULT CreateDepthStencilView(const DepthStencilViewDesc& desc);

	void UpdateFrameIndex() { frameIndex_ = pSwapChain_->GetCurrentBackBufferIndex(); }

private:
	Device* pDevice_;

	ScreenViewDesc desc_;
	IDXGISwapChain3* pSwapChain_;
	
	UINT frameIndex_;
	
	std::vector<ID3D12Resource*> rtvViewPtrs_;
	std::vector<ID3D12Resource*> dsvViewPtrs_;
	
	ID3D12DescriptorHeap* pRtvHeap_;
	ID3D12DescriptorHeap* pDsvHeap_;

	UINT rtvDescriptorSize_;
	UINT dsvDescriptorSize_;
};

