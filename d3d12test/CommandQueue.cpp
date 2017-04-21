#include "CommandQueue.h"
#include "common.h"
#include "Device.h"
#include "GpuFence.h"

CommandQueue::CommandQueue()
	: pCommandQueue_(nullptr),
	pGpuFence_(nullptr)
{}

CommandQueue::~CommandQueue()
{
	SafeDelete(&pGpuFence_);
	SafeRelease(&pCommandQueue_);
}

HRESULT CommandQueue::Create(Device* pDevice)
{
	D3D12_COMMAND_QUEUE_DESC rawDesc = {};
	rawDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	rawDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	HRESULT result;

	result = pDevice->Get()->CreateCommandQueue(&rawDesc, IID_PPV_ARGS(&pCommandQueue_));
	if (FAILED(result))
	{
		return result;
	}

	pGpuFence_ = new GpuFence();

	result = pGpuFence_->Create(pDevice);
	if (FAILED(result))
	{
		return result;
	}

	return result;
}

void CommandQueue::Enqueue(ID3D12CommandList* pCommandList)
{
	ID3D12CommandList* ppCmdLists[] = { pCommandList };
	pCommandQueue_->ExecuteCommandLists(1, ppCmdLists);
}

void CommandQueue::Enqueue(int count, ID3D12CommandList* pCommandLists[])
{
	pCommandQueue_->ExecuteCommandLists(count, pCommandLists);
}

HRESULT CommandQueue::WaitForExecution()
{
	HRESULT result;

	pGpuFence_->IncrementValue();

	result = pCommandQueue_->Signal(pGpuFence_->Get(), pGpuFence_->CurrentValue());
	if (FAILED(result))
	{
		return result;
	}

	result = pGpuFence_->WaitForCompletion();
	if (FAILED(result))
	{
		return result;
	}

	return result;
}
