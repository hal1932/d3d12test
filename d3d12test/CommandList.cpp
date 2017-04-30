#include "CommandList.h"
#include "common.h"
#include "Device.h"
#include "CommandContainer.h"

CommandList::CommandList(CommandContainer* pParent)
	: pNativeList_(nullptr),
	pParent_(pParent)
{}

CommandList::~CommandList()
{
	SafeRelease(&pNativeList_);
}

HRESULT CommandList::Create(Device* pDevice)
{
	HRESULT result;

	result = pDevice->NativePtr()->CreateCommandList(
		0,
		static_cast<D3D12_COMMAND_LIST_TYPE>(pParent_->Type()),
		pParent_->NativePtr(),
		nullptr,
		IID_PPV_ARGS(&pNativeList_));
	if (FAILED(result))
	{
		return result;
	}

	return result;
}

HRESULT CommandList::Open(ID3D12PipelineState* pPipelineState)
{
	switch (pParent_->Type())
	{
	case CommandContainer::SubmitType::Direct:
		return AsGraphicsList()->Reset(pParent_->NativePtr(), pPipelineState);

	default:
		return S_FALSE;
	}
}

void CommandList::Close()
{
	switch (pParent_->Type())
	{
	case CommandContainer::SubmitType::Direct:
		static_cast<ID3D12GraphicsCommandList*>(pNativeList_)->Close();
		break;

	default:
		break;
	}
}
