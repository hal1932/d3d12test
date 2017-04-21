#pragma once
#include <d3d12.h>

class Device;

class GpuFence
{
public:
	GpuFence();
	~GpuFence();

	ID3D12Fence* NativePtr() { return pFence_; }
	UINT64 CurrentValue() { return fenceValue_; }

	HRESULT Create(Device* pDevice);
	void IncrementValue(){ ++fenceValue_; }
	HRESULT WaitForCompletion();

private:
	ID3D12Fence* pFence_;
	UINT64 fenceValue_;
	HANDLE fenceEvent_;
};

