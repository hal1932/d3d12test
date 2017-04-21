#pragma once
#include <d3d12.h>

class Device;
class CommandContainer;

class CommandList
{
public:
	CommandList(CommandContainer* pParent);
	~CommandList();

	ID3D12GraphicsCommandList* AsGraphicsList() { return static_cast<ID3D12GraphicsCommandList*>(pNativeList_); }

	HRESULT Create(Device* pDevice);
	void Open(ID3D12PipelineState* pPipelineState);
	void Close();

private:
	ID3D12CommandList* pNativeList_;
	CommandContainer* pParent_;
};

