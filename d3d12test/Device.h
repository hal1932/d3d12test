#pragma once
#include <d3d12.h>

class Device
{
public:
	Device();
	~Device();

	ID3D12Device* NativePtr() { return pDevice_; }

	bool IsDebugEnabled() { return (pDebug_ != nullptr); }
	
	HRESULT EnableDebugLayer();
	void Create();

private:
	ID3D12Device* pDevice_;
	ID3D12Debug* pDebug_;
};

