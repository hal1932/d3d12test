#include "UpdateSubresources.h"
#include "common.h"
#include "Device.h"
#include "CommandList.h"
#include "Resource.h"
#include <Windows.h>

UINT64 GetSubresourcesFootprint_(ID3D12Device* pDevice, int start, int count, const D3D12_RESOURCE_DESC& desc);

HRESULT UpdateSubresourcesImpl_(
	ID3D12Device* pDevice,
	const D3D12_SUBRESOURCE_DATA* pData,
	ID3D12GraphicsCommandList* pCommandList,
	Resource* pDestResource, const D3D12_RESOURCE_DESC& destResourceDesc,
	ID3D12Resource* pIntermediate, const D3D12_RESOURCE_DESC& intermediateResourceDesc,
	UINT64 offset, UINT start, UINT count);

HRESULT UpdateSubresourcesImpl_(
	const D3D12_SUBRESOURCE_DATA* pData,
	ID3D12GraphicsCommandList* pCommandList,
	ID3D12Resource* pDestResource, const D3D12_RESOURCE_DESC& destResourceDesc,
	ID3D12Resource* pIntermediate, const D3D12_RESOURCE_DESC& intermidiateResourceDesc,
	UINT start, UINT count, UINT64 requiredSize,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts, UINT* pRowCounts, UINT64* pRowSizesInBytes);

void CopySubresource_(const D3D12_MEMCPY_DEST* pDest, const D3D12_SUBRESOURCE_DATA* pSource, UINT64 rowSizeInBytes, UINT rowCount, UINT sliceCount);

HRESULT UpdateSubresources(
	Device* pDevice,
	const D3D12_SUBRESOURCE_DATA* pData, int firstSubresource, int subresourceCount,
	Resource* pDestinationResource,
	CommandList* pDestinationCommandList,
	Resource* pIntermediateResource)
{
	HRESULT result;

	auto pNativeDevice = pDevice->NativePtr();
	const auto destResourceDesc = pDestinationResource->NativePtr()->GetDesc();

	ResourceDesc intermediateDesc = {};
	intermediateDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	intermediateDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	intermediateDesc.Width = GetSubresourcesFootprint_(pNativeDevice, firstSubresource, subresourceCount, destResourceDesc);
	intermediateDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	intermediateDesc.States = D3D12_RESOURCE_STATE_GENERIC_READ;

	result = pIntermediateResource->CreateCommited(pDevice, intermediateDesc);
	if (FAILED(result))
	{
		return result;
	}

	pDestinationCommandList->Open(nullptr);
	auto pNativeCommandList = pDestinationCommandList->AsGraphicsList();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = pDestinationResource->NativePtr();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	pNativeCommandList->ResourceBarrier(1, &barrier);

	result = UpdateSubresourcesImpl_(
		pNativeDevice,
		pData,
		pNativeCommandList,
		pDestinationResource, destResourceDesc,
		pIntermediateResource->NativePtr(), pIntermediateResource->NativePtr()->GetDesc(),
		0, firstSubresource, subresourceCount);
	if (FAILED(result))
	{
		return result;
	}

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	pNativeCommandList->ResourceBarrier(1, &barrier);

	pNativeCommandList->Close();

	return result;
}

UINT64 GetSubresourcesFootprint_(ID3D12Device* pDevice, int start, int count, const D3D12_RESOURCE_DESC& desc)
{
	UINT64 result;
	pDevice->GetCopyableFootprints(
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

HRESULT UpdateSubresourcesImpl_(
	ID3D12Device* pDevice,
	const D3D12_SUBRESOURCE_DATA* pData,
	ID3D12GraphicsCommandList* pCommandList,
	Resource* pDestResource, const D3D12_RESOURCE_DESC& destResourceDesc,
	ID3D12Resource* pIntermediate, const D3D12_RESOURCE_DESC& intermediateResourceDesc,
	UINT64 offset, UINT start, UINT count)
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

	UINT64 requiredSize;
	pDevice->GetCopyableFootprints(
		&destResourceDesc,
		start, count,
		offset,
		pLayouts, pRowCounts, pRowSizesInBytes,
		&requiredSize);

	result = UpdateSubresourcesImpl_(
		pData,
		pCommandList,
		pDestResource->NativePtr(), destResourceDesc,
		pIntermediate, intermediateResourceDesc,
		start, count, requiredSize,
		pLayouts, pRowCounts, pRowSizesInBytes);

	HeapFree(GetProcessHeap(), 0, buffer);

	return result;
}

HRESULT UpdateSubresourcesImpl_(
	const D3D12_SUBRESOURCE_DATA* pData,
	ID3D12GraphicsCommandList* pCommandList,
	ID3D12Resource* pDestResource, const D3D12_RESOURCE_DESC& destResourceDesc,
	ID3D12Resource* pIntermediate, const D3D12_RESOURCE_DESC& intermidiateResourceDesc,
	UINT start, UINT count, UINT64 requiredSize,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts, UINT* pRowCounts, UINT64* pRowSizesInBytes)
{
	HRESULT result;

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

	if (destResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		D3D12_BOX box = {};
		box.top = 0;
		box.left = static_cast<UINT>(pLayouts[0].Offset);
		box.bottom = 1;
		box.right = static_cast<UINT>(pLayouts[0].Offset + pLayouts[0].Footprint.Width);
		box.front = 0;
		box.back = 1;

		pCommandList->CopyBufferRegion(
			pDestResource, 0,
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
			dst.pResource = pDestResource;
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = start + i;

			pCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}
	}

	return result;
}

void CopySubresource_(const D3D12_MEMCPY_DEST* pDest, const D3D12_SUBRESOURCE_DATA* pSource, UINT64 rowSizeInBytes, UINT rowCount, UINT sliceCount)
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