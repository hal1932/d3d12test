#pragma once
#include <d3d12.h>
#include <memory>

class Device;
class CommandList;
class CommandQueue;
class GpuFence;

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
	Resource(ID3D12Resource* pResource, Device* pDevice);
	~Resource();

	ID3D12Resource* NativePtr() { return pResource_; }

	HRESULT CreateCommited(Device* pDevice, const ResourceDesc& desc);
	HRESULT CreateCommited(Device* pDevice, const ResourceDesc& desc, const D3D12_CLEAR_VALUE& clearValue);

	HRESULT CreateVertexBuffer(Device* pDevice, int size);
	HRESULT CreateIndexBuffer(Device* pDevice, int size);

	void* Map(int subresource);
	void Unmap(int subresource);

	MappedResource ScopedMap(int subresource);

	HRESULT UpdateSubresource(const D3D12_SUBRESOURCE_DATA* pData, CommandList* pCommandList, CommandQueue* pCommandQueue, int subresource);
	HRESULT UpdateSubresources(const D3D12_SUBRESOURCE_DATA* pData, CommandList* pCommandList, CommandQueue* pCommandQueue, int firstSubresource, int subresourceCount);

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(int stride);
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(DXGI_FORMAT format);

private:
	Device* pDevice_;
	ID3D12Resource* pResource_;
	ResourceDesc desc_;

	HRESULT CreateCommitedImpl_(Device* pDevice, const ResourceDesc& desc, const D3D12_CLEAR_VALUE* pClearValue);
	UINT64 GetSubresourcesFootprint_(int start, int count);
	HRESULT UpdateSubresourcesImpl_(const D3D12_SUBRESOURCE_DATA* pData, CommandList* pCommandList, ID3D12Resource* pIntermediate, UINT64 offset, UINT start, UINT count);
	HRESULT UpdateSubresourcesImpl_(const D3D12_SUBRESOURCE_DATA* pData, CommandList* pCommandList, ID3D12Resource* pIntermediate, UINT start, UINT count, UINT64 requiredSize, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts, UINT* pRowCounts, UINT64* pRowSizesInBytes);
	void CopySubresource_(const D3D12_MEMCPY_DEST* pDest, const D3D12_SUBRESOURCE_DATA* pData, UINT64 rowSizeInBytes, UINT rowCount, UINT sliceCount);
};
