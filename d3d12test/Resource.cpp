#include "Resource.h"
#include "common.h"
#include "Device.h"

Resource::Resource()
	: pResource_(nullptr)
{}

Resource::Resource(ID3D12Resource* pResource)
	: pResource_(pResource)
{
	D3D12_HEAP_PROPERTIES heapProp;
	D3D12_HEAP_FLAGS heapFlags;
	pResource->GetHeapProperties(&heapProp, &heapFlags);

	const auto& desc = pResource->GetDesc();

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

	desc_ = desc;

	return result;
}
