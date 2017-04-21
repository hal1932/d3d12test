#pragma once
#include <d3d12.h>
#include <vector>

class Device;
class CommandList;

class CommandContainer
{
public:
	enum class SubmitType
	{
		Direct = D3D12_COMMAND_LIST_TYPE_DIRECT,
		Bundle = D3D12_COMMAND_LIST_TYPE_BUNDLE,
	};

public:
	CommandContainer();
	~CommandContainer();

	ID3D12CommandAllocator* NativePtr() { return pCommandAllocator_; }
	SubmitType Type() { return type_; }

	HRESULT Create(Device* pDevice, SubmitType type = SubmitType::Direct);

	CommandList* AddGraphicsList();
	CommandList* AddBundle() { return nullptr; }
	void ClearState() { pCommandAllocator_->Reset(); }

private:
	Device* pDevice_;
	ID3D12CommandAllocator* pCommandAllocator_;
	SubmitType type_;
	std::vector<CommandList*> itemPtrs_;
};

