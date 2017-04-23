#pragma once
#include <d3d12.h>
#include <memory>

class Device;

struct ResourceDesc
{
	D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_DEFAULT;
	DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
	D3D12_RESOURCE_DIMENSION Dimension;
	int Width;
	int Height = 1;
	short Depth = 1;
	short MipLevels = 1;
	int SampleCount = 1;
	D3D12_TEXTURE_LAYOUT Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES States;
};

class MappedResource
{
public:
	MappedResource()
		: pResource_(nullptr)
	{}

	MappedResource(MappedResource& other)
		: pData_(other.pData_),
		pResource_(other.pResource_),
		subresource_(other.subresource_)
	{
		other.pData_ = nullptr;
		other.pResource_ = nullptr;
	}

	~MappedResource()
	{
		if (pData_ != nullptr)
		{
			pResource_->Unmap(subresource_, nullptr);
		}
	}

	MappedResource& Map(ID3D12Resource* pResource, int subresource)
	{
		auto result = pResource->Map(subresource, nullptr, &pData_);
		if (FAILED(result))
		{
			pData_ = nullptr;
		}

		pResource_ = pResource;
		subresource_ = subresource;

		return *this;
	}

	void* NativePtr() { return pData_; }

private:
	void* pData_;
	ID3D12Resource* pResource_;
	int subresource_;
};

class Resource
{
public:
	Resource();
	Resource(ID3D12Resource* pResource) : pResource_(pResource) {}
	~Resource();

	ID3D12Resource* NativePtr() { return pResource_; }

	HRESULT CreateCommited(Device* pDevice, const ResourceDesc& desc);
	HRESULT CreateCommited(Device* pDevice, const ResourceDesc& desc, const D3D12_CLEAR_VALUE& clearValue);

	HRESULT CreateVertexBuffer(Device* pDevice, int size)
	{
		ResourceDesc desc = {};
		desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = size;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
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

	MappedResource ScopedMap(int subresource)
	{
		return MappedResource().Map(pResource_, subresource);
	}

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

	HRESULT CreateCommitedImpl_(Device* pDevice, const ResourceDesc& desc, const D3D12_CLEAR_VALUE* pClearValue);
};

