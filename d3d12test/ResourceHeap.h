#pragma once
#include <d3d12.h>
#include <vector>

class Device;
class ScreenContext;

struct RtvHeapDesc
{
	int BufferCount;
};

struct DsvHeapDesc
{
	int BufferCount;
};

struct DsvDesc
{
	int Width;
	int Height;
	DXGI_FORMAT Format;
	float ClearDepth;
	unsigned char ClearStencil;
};

class ResourceHeap
{
public:
	ResourceHeap();
	~ResourceHeap();

	ID3D12Resource* NativeResourcePtr(int index) { return resourcePtrs_[index]; }

	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle(int index);

	HRESULT CreateRenderTargetViewHeap(Device* pDevice, const RtvHeapDesc& desc);
	HRESULT CreateRenderTargetViewFromBackBuffer(ScreenContext* pScreen);

	HRESULT CreateDepthStencilViewHeap(Device* pDevice, const DsvHeapDesc& desc);
	HRESULT CreateDepthStencilView(ScreenContext* pContext, const DsvDesc& desc);

private:
	Device* pDevice_;

	ID3D12DescriptorHeap *pDescriptorHeap_;
	UINT descriptorSize_;
	UINT resourceCount_;
	std::vector<ID3D12Resource*> resourcePtrs_;
};

