#include "Resource.h"
#include "common.h"
#include "Device.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "GpuFence.h"
#include <d3dx12.h>

Resource::Resource()
	: pDevice_(nullptr),
	pResource_(nullptr)
{}

Resource::Resource(ID3D12Resource* pResource, Device* pDevice)
	: pDevice_(pDevice),
	pResource_(pResource)
{
	D3D12_HEAP_PROPERTIES heapProp;
	D3D12_HEAP_FLAGS heapFlags;
	pResource->GetHeapProperties(&heapProp, &heapFlags);

	const auto desc = pResource->GetDesc();

	desc_.HeapType = heapProp.Type;
	desc_.Format = desc.Format;
	desc_.Dimension = desc.Dimension;
	desc_.Width = static_cast<int>(desc.Width);
	desc_.Height = desc.Height;
	desc_.Depth = desc.DepthOrArraySize;
	desc_.MipLevels = desc.MipLevels;
	desc_.SampleCount = desc.SampleDesc.Count;
	desc_.Layout = desc.Layout;
	desc_.Flags = desc.Flags;
}

Resource::~Resource()
{
	SafeRelease(&pResource_);
	pDevice_ = nullptr;
}

HRESULT Resource::CreateCommited(Device* pDevice, const ResourceDesc& desc)
{
	return CreateCommitedImpl_(pDevice, desc, nullptr);
}

HRESULT Resource::CreateCommited(Device* pDevice, const ResourceDesc& desc, const D3D12_CLEAR_VALUE& clearValue)
{
	return CreateCommitedImpl_(pDevice, desc, &clearValue);
}

HRESULT Resource::CreateCommitedImpl_(Device* pDevice, const ResourceDesc& desc, const D3D12_CLEAR_VALUE* pClearValue)
{
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = desc.HeapType;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = desc.Dimension;
	resDesc.Format = desc.Format;
	resDesc.Width = static_cast<UINT64>(desc.Width);
	resDesc.Height = static_cast<UINT64>(desc.Height);
	resDesc.DepthOrArraySize = static_cast<UINT16>(desc.Depth);
	resDesc.MipLevels = static_cast<UINT16>(desc.MipLevels);
	resDesc.SampleDesc.Count = static_cast<UINT>(desc.SampleCount);
	resDesc.Flags = desc.Flags;
	resDesc.Layout = desc.Layout;

	HRESULT result;

	auto pNativeDevice = pDevice->NativePtr();

	result = pNativeDevice->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, desc.States,
		pClearValue,
		IID_PPV_ARGS(&pResource_));
	if (FAILED(result))
	{
		return result;
	}

	pDevice_ = pDevice;
	desc_ = desc;

	return result;
}

HRESULT Resource::CreateVertexBuffer(Device* pDevice, int size)
{
	ResourceDesc desc = {};
	desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.States = D3D12_RESOURCE_STATE_GENERIC_READ;

	return CreateCommited(pDevice, desc);
}

HRESULT Resource::CreateIndexBuffer(Device* pDevice, int size)
{
	return CreateVertexBuffer(pDevice, size);
}

void* Resource::Map(int subresource)
{
	void* pData;
	auto result = pResource_->Map(subresource, nullptr, &pData);
	return (SUCCEEDED(result)) ? pData : nullptr;
}

void Resource::Unmap(int subresource)
{
	pResource_->Unmap(subresource, nullptr);
}

MappedResource Resource::ScopedMap(int subresource)
{
	return MappedResource().Map(pResource_, subresource);
}

D3D12_VERTEX_BUFFER_VIEW Resource::GetVertexBufferView(int stride)
{
	return
	{
		pResource_->GetGPUVirtualAddress(),
		static_cast<UINT>(desc_.Width),
		static_cast<UINT>(stride)
	};
}

D3D12_INDEX_BUFFER_VIEW Resource::GetIndexBufferView(DXGI_FORMAT format)
{
	return
	{
		pResource_->GetGPUVirtualAddress(),
		static_cast<UINT>(desc_.Width),
		format
	};
}

HRESULT Resource::UpdateSubresource(const D3D12_SUBRESOURCE_DATA* pData, CommandList* pCommandList, CommandQueue* pCommandQueue, int subresource)
{
	return UpdateSubresources(pData, pCommandList, pCommandQueue, subresource, 1);
}

HRESULT Resource::UpdateSubresources(const D3D12_SUBRESOURCE_DATA* pData, CommandList* pCommandList, CommandQueue* pCommandQueue, int firstSubresource, int subresourceCount)
{
	HRESULT result;

	auto pNativeDevice = pDevice_->NativePtr();

	ID3D12Resource* pIntermediate;
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = GetSubresourcesFootprint_(firstSubresource, subresourceCount);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		result = pNativeDevice->CreateCommittedResource(
			&heapProp, D3D12_HEAP_FLAG_NONE,
			&desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&pIntermediate));
		if (FAILED(result))
		{
			return result;
		}
	}

	pCommandList->Open(nullptr);
	auto pNativeCommandList = pCommandList->AsGraphicsList();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = pResource_;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	pNativeCommandList->ResourceBarrier(1, &barrier);

	result = UpdateSubresourcesImpl_(pData, pCommandList, pIntermediate, 0, firstSubresource, subresourceCount);
	if (FAILED(result))
	{
		SafeRelease(&pIntermediate);
		return result;
	}

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	pNativeCommandList->ResourceBarrier(1, &barrier);

	pNativeCommandList->Close();

	pCommandQueue->Submit(pCommandList);

	result = pCommandQueue->WaitForExecution();
	if (FAILED(result))
	{
		SafeRelease(&pIntermediate);
		return result;
	}

	SafeRelease(&pIntermediate);

	return result;
}

UINT64 Resource::GetSubresourcesFootprint_(int start, int count)
{
	auto pNativeDevice = pDevice_->NativePtr();

	auto desc = pResource_->GetDesc();

	UINT64 result;
	pNativeDevice->GetCopyableFootprints(
		&desc,
		start, count,
		0, // UINT64 BaseOffset
		nullptr, // D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts
		nullptr, // UINT *pNumRows
		nullptr, // UINT64 *pRowSizeInBytes
		&result
	);

	return result;
}

HRESULT Resource::UpdateSubresourcesImpl_(const D3D12_SUBRESOURCE_DATA* pData, CommandList* pCommandList, ID3D12Resource* pIntermediate, UINT64 offset, UINT start, UINT count)
{
	HRESULT result;

	const auto singleBufferSize = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64));
	const auto bufferSize = singleBufferSize * count;

	if (bufferSize > SIZE_MAX)
	{
		return S_FALSE;
	}

	auto buffer = HeapAlloc(GetProcessHeap(), 0, static_cast<size_t>(bufferSize));
	if (!buffer)
	{
		return S_FALSE;
	}

	auto pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(buffer);
	auto pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + count);
	auto pRowCounts = reinterpret_cast<UINT*>(pRowSizesInBytes + count);

	auto desc = pResource_->GetDesc();

	auto pNativeDevice = pDevice_->NativePtr();

	UINT64 requiredSize;
	pNativeDevice->GetCopyableFootprints(
		&desc,
		start, count,
		offset,
		pLayouts, pRowCounts, pRowSizesInBytes,
		&requiredSize);

	result = UpdateSubresourcesImpl_(
		pData,
		pCommandList,
		pIntermediate,
		start, count,
		requiredSize,
		pLayouts, pRowCounts, pRowSizesInBytes);

	HeapFree(GetProcessHeap(), 0, buffer);

	return result;
}

HRESULT Resource::UpdateSubresourcesImpl_(const D3D12_SUBRESOURCE_DATA* pData, CommandList* pCommandList, ID3D12Resource* pIntermediate, UINT start, UINT count, UINT64 requiredSize, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts, UINT* pRowCounts, UINT64* pRowSizesInBytes)
{
	HRESULT result;

	const auto intermediateDesc = pIntermediate->GetDesc();
	const auto destinationDesc = NativePtr()->GetDesc();

	{
		void* pMappedData;
		result = pIntermediate->Map(0, nullptr, &pMappedData);
		if (FAILED(result))
		{
			return result;
		}

		for (UINT i = 0; i < count; ++i)
		{
			D3D12_MEMCPY_DEST dest = {};
			dest.pData = reinterpret_cast<byte*>(pMappedData) + pLayouts[i].Offset;
			dest.RowPitch = pLayouts[i].Footprint.RowPitch;
			dest.SlicePitch = pLayouts[i].Footprint.RowPitch * pRowCounts[i];
			CopySubresource_(&dest, &pData[i], pRowSizesInBytes[i], pRowCounts[i], pLayouts[i].Footprint.Depth);
		}

		pIntermediate->Unmap(0, nullptr);
	}

	if (destinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		D3D12_BOX box = {};
		box.top = 0;
		box.left = static_cast<UINT>(pLayouts[0].Offset);
		box.bottom = 1;
		box.right = static_cast<UINT>(pLayouts[0].Offset + pLayouts[0].Footprint.Width);
		box.front = 0;
		box.back = 1;

		pCommandList->AsGraphicsList()->CopyBufferRegion(
			NativePtr(), 0,
			pIntermediate, pLayouts[0].Offset,
			pLayouts[0].Footprint.Width);
	}
	else
	{
		for (UINT i = 0; i < count; ++i)
		{
			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.pResource = pIntermediate;
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = pLayouts[i];

			D3D12_TEXTURE_COPY_LOCATION dst = {};
			dst.pResource = NativePtr();
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = start + i;

			pCommandList->AsGraphicsList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}
	}

	return result;
}

void Resource::CopySubresource_(const D3D12_MEMCPY_DEST* pDest, const D3D12_SUBRESOURCE_DATA* pSource, UINT64 rowSizeInBytes, UINT rowCount, UINT sliceCount)
{
	for (UINT i = 0; i < sliceCount; ++i)
	{
		auto pDestSlice = reinterpret_cast<byte*>(pDest->pData) + pDest->SlicePitch * i;
		const auto pSrcSlice = reinterpret_cast<const byte*>(pSource->pData) + pSource->SlicePitch * i;

		for (UINT j = 0; j < rowCount; ++j)
		{
			memcpy(
				pDestSlice + pDest->RowPitch * j,
				pSrcSlice + pSource->RowPitch * j,
				rowSizeInBytes);
		}
	}
}
