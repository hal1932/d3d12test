#include "CommandContainer.h"
#include "common.h"
#include "Device.h"
#include "CommandList.h"

CommandContainer::~CommandContainer()
{
	SafeRelease(&pCommandAllocator_);
}

HRESULT CommandContainer::Create(Device* pDevice, SubmitType type)
{
	HRESULT result;

	result = pDevice->NativePtr()->CreateCommandAllocator(
		static_cast<D3D12_COMMAND_LIST_TYPE>(type),
		IID_PPV_ARGS(&pCommandAllocator_));

	pDevice_ = pDevice;

	return result;
}

CommandList* CommandContainer::CreateCommandList()
{
	auto pList = new CommandList(this);

	HRESULT result;

	result = pList->Create(pDevice_);
	if (FAILED(result))
	{
		return nullptr;
	}

	return pList;
}
