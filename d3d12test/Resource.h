#pragma once
#include <d3d12.h>
#include <memory>

class Device;

struct ResourceDesc
{
	D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_DIMENSION Dimension;
	int Width;
	int Height = 1;
	short Depth = 1;
	short MipLevels = 1;
	int SampleCount = 1;
	D3D12_TEXTURE_LAYOUT Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	D3D12_RESOURCE_STATES States;
};

class Resource
{
public:
	Resource();
	~Resource();

	ID3D12Resource* NativePtr() { return pResource_; }

	HRESULT CreateCommited(Device* pDevice, const ResourceDesc& desc);

	HRESULT CreateVertexBuffer(Device* pDevice, int size)
	{
		ResourceDesc desc = {};
		desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = size;
		desc.States = D3D12_RESOURCE_STATE_GENERIC_READ;

		return CreateCommited(pDevice, desc);
	}

	void* Map(int subresource)
	{
		void* pData;
		auto result = pResource_->Map(subresource, nullptr, &pData);
		return (SUCCEEDED(result)) ? pData : nullptr;
	}

	void Unmap(int subresource) { pResource_->Unmap(subresource, nullptr); }

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(int stride)
	{
		return
		{
			pResource_->GetGPUVirtualAddress(),
			static_cast<UINT>(desc_.Width),
			static_cast<UINT>(stride)
		};
	}

private:
	ID3D12Resource* pResource_;
	ResourceDesc desc_;
};

