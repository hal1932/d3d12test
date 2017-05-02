#pragma once
#include <d3d12.h>
#include <vector>

class Device;
class ScreenContext;
class Resource;
class Texture;

struct HeapDesc
{
	enum class ViewType
	{
		RenderTargetView,
		DepthStencilView,
		CbSrUaView, // ConstantBuffer/ShaderResource/UnorderedAccess View
	} ViewType;
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

struct CsvDesc
{
	int Size;
	D3D12_TEXTURE_LAYOUT Layout;
};

struct SrvDesc
{
	D3D12_SRV_DIMENSION Dimension;
	union
	{
		Texture* pTexture;
	};
};

class ResourceViewHeap
{
public:
	ResourceViewHeap();
	~ResourceViewHeap();

	ID3D12DescriptorHeap* NativePtr() { return pDescriptorHeap_; }

	Resource* ResourcePtr(int index) { return resourcePtrs_[index]; }
	ID3D12Resource* NativeResourcePtr(int index);

	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle(int index);
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle(int index);

	HRESULT CreateHeap(Device* pDevice, const HeapDesc& desc);

	HRESULT CreateRenderTargetViewFromBackBuffer(ScreenContext* pScreen);
	HRESULT CreateDepthStencilView(ScreenContext* pContext, const DsvDesc& desc);
	HRESULT CreateConstantBufferView(const CsvDesc& desc);
	HRESULT CreateShaderResourceView(const SrvDesc& desc);

private:
	Device* pDevice_;

	ID3D12DescriptorHeap* pDescriptorHeap_;
	UINT descriptorSize_;
	UINT resourceCount_;
	std::vector<Resource*> resourcePtrs_;

	HRESULT CreateHeapImpl_(
		Device* pDevice,
		const HeapDesc& desc,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags);
};

