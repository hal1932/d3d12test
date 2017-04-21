#include "Device.h"
#include "common.h"


Device::Device()
	: pDevice_(nullptr)
{}

Device::~Device()
{
	SafeRelease(&pDevice_);
	SafeRelease(&pDebug_);
}

HRESULT Device::EnableDebugLayer()
{
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug_));
	if (SUCCEEDED(result))
	{
		pDebug_->EnableDebugLayer();
	}
	return result;
}

void Device::Create()
{
	auto result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice_));
	ThrowIfFailed(result);
}
