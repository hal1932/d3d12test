#include "ResourceHeap.h"
#include "common.h"
#include "Device.h"
#include "ScreenContext.h"

ResourceHeap::ResourceHeap()
	: pDevice_(nullptr),
	pDescriptorHeap_(nullptr),
	descriptorSize_(0U)
{}

ResourceHeap::~ResourceHeap()
{
	for (auto pResource : resourcePtrs_)
	{
		SafeRelease(&pResource);
	}

	SafeRelease(&pDescriptorHeap_);
}

D3D12_CPU_DESCRIPTOR_HANDLE ResourceHeap::CpuHandle(int index)
{
	auto handle = pDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += index * descriptorSize_;
	return handle;
}

HRESULT ResourceHeap::CreateHeap(Device* pDevice, const HeapDesc& desc)
{
	switch (desc.ViewType)
	{
		case HeapDesc::ViewType::RenderTargetView:
			return CreateHeapImpl_(pDevice, desc, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

		case HeapDesc::ViewType::DepthStencilView:
			return CreateHeapImpl_(pDevice, desc, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

		case HeapDesc::ViewType::ConstantBufferView:
			return CreateHeapImpl_(pDevice, desc, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

		default:
			return S_FALSE;
	}
}

HRESULT ResourceHeap::CreateRenderTargetViewFromBackBuffer(ScreenContext* pScreen)
{
	D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
	viewDesc.Format = pScreen->Desc().Format;
	viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	HRESULT result;

	auto pNativeDevice = pDevice_->NativePtr();

	auto handle = CpuHandle(static_cast<int>(resourcePtrs_.size()));
	for (UINT i = 0; i < resourceCount_; ++i)
	{
		ID3D12Resource* pView;
		result = pScreen->GetBackBufferView(i, &pView);
		if (FAILED(result))
		{
			return result;
		}
		resourcePtrs_.push_back(pView);

		pNativeDevice->CreateRenderTargetView(pView, &viewDesc, handle);
		handle.ptr += descriptorSize_;
	}

	return result;
}

HRESULT ResourceHeap::CreateDepthStencilView(ScreenContext* pContext, const DsvDesc& desc)
{
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = static_cast<UINT64>(desc.Width);
	resDesc.Height = static_cast<UINT64>(desc.Height);
	resDesc.DepthOrArraySize = 1;
	resDesc.Format = desc.Format;
	resDesc.SampleDesc.Count = 1;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = desc.Format;
	clearValue.DepthStencil.Depth = desc.ClearDepth;
	clearValue.DepthStencil.Stencil = desc.ClearStencil;

	HRESULT result;

	auto pNativeDevice = pDevice_->NativePtr();

	ID3D12Resource* pView;
	result = pNativeDevice->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&pView));
	if (FAILED(result))
	{
		return result;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
	viewDesc.Format = desc.Format;
	viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	auto handle = CpuHandle(static_cast<int>(resourcePtrs_.size()));
	pNativeDevice->CreateDepthStencilView(pView, &viewDesc, handle);

	resourcePtrs_.push_back(pView);

	return result;
}

HRESULT ResourceHeap::CreateConstantBufferView(const CsvDesc& desc)
{
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = static_cast<UINT64>(desc.Size);
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = desc.Layout;

	HRESULT result;

	auto pNativeDevice = pDevice_->NativePtr();

	ID3D12Resource* pView;
	result = pNativeDevice->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pView));
	if (FAILED(result))
	{
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
	viewDesc.BufferLocation = pView->GetGPUVirtualAddress();
	viewDesc.SizeInBytes = static_cast<UINT>(desc.Size);

	auto handle = CpuHandle(static_cast<int>(resourcePtrs_.size()));
	pNativeDevice->CreateConstantBufferView(&viewDesc, handle);

	resourcePtrs_.push_back(pView);

	return result;
}

HRESULT ResourceHeap::CreateHeapImpl_(
	Device* pDevice,
	const HeapDesc& desc,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = static_cast<UINT>(desc.BufferCount);
	heapDesc.Type = type;
	heapDesc.Flags = flags;

	HRESULT result;

	auto pNativeDevice = pDevice->NativePtr();

	result = pNativeDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pDescriptorHeap_));
	if (FAILED(result))
	{
		return result;
	}

	pDevice_ = pDevice;
	descriptorSize_ = pNativeDevice->GetDescriptorHandleIncrementSize(heapDesc.Type);
	resourceCount_ = heapDesc.NumDescriptors;

	return result;
}
