#include "ScreenView.h"

#include "common.h"
#include "Device.h"
#include <wrl.h>

using Microsoft::WRL::ComPtr;

ScreenView::ScreenView()
	: pSwapChain_(nullptr),
	frameIndex_(0),
	pRtvHeap_(nullptr),
	pDsvHeap_(nullptr)
{}

ScreenView::~ScreenView()
{
	for (auto pView : rtvViewPtrs_)
	{
		SafeRelease(&pView);
	}
	SafeRelease(&pRtvHeap_);

	for (auto pView : dsvViewPtrs_)
	{
		SafeRelease(&pView);
	}
	SafeRelease(&pDsvHeap_);

	SafeRelease(&pSwapChain_);
}

void ScreenView::Create(Device* pDevice, ID3D12CommandQueue* pCommandQueue, const ScreenViewDesc& desc)
{
	DXGI_SWAP_CHAIN_DESC rawDesc = {};

	rawDesc.BufferCount = static_cast<UINT>(desc.BufferCount);
	rawDesc.BufferDesc.Format = desc.Format;
	rawDesc.BufferDesc.Width = static_cast<UINT>(desc.Width);
	rawDesc.BufferDesc.Height = static_cast<UINT>(desc.Height);
	rawDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	rawDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	rawDesc.OutputWindow = desc.OutputWindow;
	rawDesc.SampleDesc.Count = 1;
	rawDesc.Windowed = (desc.Windowed) ? TRUE : FALSE;

	if (rawDesc.BufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
	{
		rawDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	auto factoryFlags = 0U;
	if (pDevice->IsDebugEnabled())
	{
		factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}

	HRESULT result;

	ComPtr<IDXGIFactory4> pFactory;
	result = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&pFactory));
	ThrowIfFailed(result);

	ComPtr<IDXGISwapChain> pTmpSwapChain;
	result = pFactory->CreateSwapChain(pCommandQueue, &rawDesc, pTmpSwapChain.GetAddressOf());
	ThrowIfFailed(result);

	result = pTmpSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain_));
	ThrowIfFailed(result);

	frameIndex_ = pSwapChain_->GetCurrentBackBufferIndex();

	desc_ = desc;
	pDevice_ = pDevice;
}

HRESULT ScreenView::CreateRenderTargetViews()
{
	HRESULT result;

	if (pRtvHeap_ == nullptr)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = desc_.BufferCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		result = pDevice_->Get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRtvHeap_));
		if (FAILED(result))
		{
			return result;
		}

		rtvDescriptorSize_ = pDevice_->Get()->GetDescriptorHandleIncrementSize(desc.Type);
	}

	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.Format = desc_.Format;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	auto handle = pRtvHeap_->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < desc_.BufferCount; ++i)
	{
		ID3D12Resource* pBackBufferView;
		result = pSwapChain_->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&pBackBufferView));
		if (FAILED(result))
		{
			return result;
		}
		rtvViewPtrs_.push_back(pBackBufferView);

		pDevice_->Get()->CreateRenderTargetView(pBackBufferView, &desc, handle);
		handle.ptr += rtvDescriptorSize_;
	}

	return result;
}

HRESULT ScreenView::CreateDepthStencilView(const DepthStencilViewDesc& desc)
{
	HRESULT result;

	if (pDsvHeap_ == nullptr)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = 1;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		result = pDevice_->Get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pDsvHeap_));

		dsvDescriptorSize_ = pDevice_->Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = static_cast<UINT64>(desc_.Width);
	resDesc.Height = static_cast<UINT64>(desc_.Height);
	resDesc.DepthOrArraySize = 1;
	resDesc.Format = desc.Format;
	resDesc.SampleDesc.Count = 1;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = desc.Format;
	clearValue.DepthStencil.Depth = desc.ClearDepth;
	clearValue.DepthStencil.Stencil = desc.ClearStencil;

	ID3D12Resource* pDsv;
	result = pDevice_->Get()->CreateCommittedResource(
		&prop, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&pDsv));
	if (FAILED(result))
	{
		return result;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	pDevice_->Get()->CreateDepthStencilView(pDsv, &dsvDesc, pDsvHeap_->GetCPUDescriptorHandleForHeapStart());

	dsvViewPtrs_.push_back(pDsv);

	return result;
}

