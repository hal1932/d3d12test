#pragma once
#include "lib/lib.h"

class Graphics
{
public:
	~Graphics();

	HRESULT Setup(bool debug);

	Device* DevicePtr() { return &device_; }
	ScreenContext* ScreenPtr() { return &screen_; }
	CommandQueue* CommandQueuePtr() { return &commandQueue_; }
	CommandList* GraphicsListPtr(int index) { return graphicsListPtrs_[index]; }

	Resource* CurrentRenderTargetPtr() { return renderTargetPtrs_[screen_.FrameIndex()].get(); }
	Resource* DepthStencilPtr() { return depthStencilPtr_.get(); }

	HRESULT ResizeScreen(const ScreenContextDesc& desc);
	HRESULT ResizeScreen(int width, int height);

	HRESULT AddGraphicsComandList(int count);

	void ClearCommand();
	void SubmitCommand(CommandList* pCommandList);
	void SwapBuffers();
	HRESULT WaitForCommandExecution();

private:
	Device device_;
	ScreenContext screen_;

	ResourceViewHeap renderTargetHeap_;
	std::vector<std::unique_ptr<Resource>> renderTargetPtrs_;

	ResourceViewHeap depthStencilHeap_;
	std::unique_ptr<Resource> depthStencilPtr_;

	CommandQueue commandQueue_;
	CommandContainer graphicsListContainer_;
	std::vector<CommandList*> graphicsListPtrs_;
};

