#include "ResourceViewHeap.h"
#include "common.h"
#include "Device.h"
#include "ScreenContext.h"
#include "Resource.h"

ResourceViewHeap::ResourceViewHeap()
	: pDevice_(nullptr),
	pDescriptorHeap_(nullptr),
	descriptorSize_(0U)
{}

ResourceViewHeap::~ResourceViewHeap()
{
	for (auto pResource : resourcePtrs_)
	{
		SafeDelete(&pResource);
	}

	SafeRelease(&pDescriptorHeap_);
}

D3D12_CPU_DESCRIPTOR_HANDLE ResourceViewHeap::CpuHandle(int index)
{
	auto handle = pDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += index * descriptorSize_;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceViewHeap::GpuHandle(int index)
{
	auto handle = pDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += index * descriptorSize_;
	return handle;
}

ID3D12Resource* ResourceViewHeap::NativeResourcePtr(int index)
{
	return ResourcePtr(index)->NativePtr();
}

HRESULT ResourceViewHeap::CreateHeap(Device* pDevice, const HeapDesc& desc)
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

HRESULT ResourceViewHeap::CreateRenderTargetViewFromBackBuffer(ScreenContext* pScreen)
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
		resourcePtrs_.push_back(new Resource(pView, pDevice_));

		pNativeDevice->CreateRenderTargetView(pView, &viewDesc, handle);
		handle.ptr += descriptorSize_;
	}

	return result;
}

HRESULT ResourceViewHeap::CreateDepthStencilView(ScreenContext* pContext, const DsvDesc& desc)
{
	ResourceDesc resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = desc.Width;
	resourceDesc.Height = desc.Height;
	resourceDesc.Format = desc.Format;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resourceDesc.States = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = desc.Format;
	clearValue.DepthStencil.Depth = desc.ClearDepth;
	clearValue.DepthStencil.Stencil = desc.ClearStencil;

	HRESULT result;

	auto pNativeDevice = pDevice_->NativePtr();

	auto pResource = new Resource();
	result = pResource->CreateCommited(pDevice_, resourceDesc, clearValue);
	if (FAILED(result))
	{
		return result;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
	viewDesc.Format = desc.Format;
	viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	auto handle = CpuHandle(static_cast<int>(resourcePtrs_.size()));
	pNativeDevice->CreateDepthStencilView(pResource->NativePtr(), &viewDesc, handle);

	resourcePtrs_.push_back(pResource);

	return result;
}

HRESULT ResourceViewHeap::CreateConstantBufferView(const CsvDesc& desc)
{
	ResourceDesc resourceDesc = {};
	resourceDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = desc.Size;
	resourceDesc.Layout = desc.Layout;
	resourceDesc.States = D3D12_RESOURCE_STATE_GENERIC_READ;

	HRESULT result;

	auto pNativeDevice = pDevice_->NativePtr();

	auto pResource = new Resource();
	result = pResource->CreateCommited(pDevice_, resourceDesc);
	if (FAILED(result))
	{
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
	viewDesc.BufferLocation = pResource->NativePtr()->GetGPUVirtualAddress();
	viewDesc.SizeInBytes = static_cast<UINT>(desc.Size);

	auto handle = CpuHandle(static_cast<int>(resourcePtrs_.size()));
	pNativeDevice->CreateConstantBufferView(&viewDesc, handle);

	resourcePtrs_.push_back(pResource);

	return result;
}

HRESULT ResourceViewHeap::CreateHeapImpl_(
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
