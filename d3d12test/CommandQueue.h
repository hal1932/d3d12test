#pragma once
#include <d3d12.h>

class Device;
class GpuFence;

class CommandQueue
{
public:
	CommandQueue();
	~CommandQueue();

	ID3D12CommandQueue* Get() { return pCommandQueue_; }

	HRESULT Create(Device* pDevice);

	void SubmitSingleList(ID3D12CommandList* pCommandList);
	void SubmitMultipleLists(int count, ID3D12CommandList* pCommandLists[]);

	HRESULT WaitForExecution();

private:
	ID3D12CommandQueue* pCommandQueue_;
	GpuFence* pGpuFence_;
};

