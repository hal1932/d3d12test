#include "Resource.h"
#include "common.h"
#include "Device.h"

Resource::Resource()
	: pResource_(nullptr)
{}

Resource::~Resource()
{
	SafeRelease(&pResource_);
}

HRESULT Resource::CreateCommited(Device* pDevice, const ResourceDesc& desc)
{
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = desc.HeapType;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = desc.Dimension;
	resDesc.Width = static_cast<UINT64>(desc.Width);
	resDesc.Height = static_cast<UINT64>(desc.Height);
	resDesc.DepthOrArraySize = static_cast<UINT16>(desc.Depth);
	resDesc.MipLevels = static_cast<UINT16>(desc.MipLevels);
	resDesc.SampleDesc.Count = static_cast<UINT>(desc.SampleCount);
	resDesc.Layout = desc.Layout;

	HRESULT result;

	auto pNativeDevice = pDevice->NativePtr();

	result = pNativeDevice->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, desc.States,
		nullptr, IID_PPV_ARGS(&pResource_));
	if (FAILED(result))
	{
		return result;
	}

	desc_ = desc;

	return result;
}
