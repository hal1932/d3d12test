#include "Graphics.h"

Graphics::~Graphics()
{
	for (auto pList : graphicsListPtrs_)
	{
		SafeDelete(&pList);
	}
}

HRESULT Graphics::Setup(bool debug)
{
	auto result = S_OK;

	if (debug)
	{
		result = device_.EnableDebugLayer();
		if (FAILED(result))
		{
			return result;
		}
	}

	device_.Create();

	result = commandQueue_.Create(&device_);
	if (FAILED(result))
	{
		return result;
	}

	result = graphicsListContainer_.Create(&device_);
	if (FAILED(result))
	{
		return result;
	}

	return result;
}

HRESULT Graphics::ResizeScreen(const ScreenContextDesc& desc)
{
	auto result = S_OK;

	renderTargetPtrs_.clear();
	renderTargetHeap_.Reset();

	depthStencilPtr_.reset();
	depthStencilHeap_.Reset();

	screen_.Reset();
	screen_.Create(&device_, &commandQueue_, desc);

	result = renderTargetHeap_.CreateHeap(&device_, { HeapDesc::ViewType::RenderTargetView, desc.BufferCount });
	if (FAILED(result))
	{
		return result;
	}
	for (auto pResource : renderTargetHeap_.CreateRenderTargetViewFromBackBuffer(&screen_))
	{
		renderTargetPtrs_.push_back(std::unique_ptr<Resource>(pResource));
	}

	result = depthStencilHeap_.CreateHeap(&device_, { HeapDesc::ViewType::DepthStencilView, 1 });
	if (FAILED(result))
	{
		return result;
	}
	depthStencilPtr_ = std::unique_ptr<Resource>(
		depthStencilHeap_.CreateDepthStencilView(
			&screen_,{ desc.Width, desc.Height, DXGI_FORMAT_D24_UNORM_S8_UINT, 1.0f, 0 }));

	return result;
}

HRESULT Graphics::ResizeScreen(int width, int height)
{
	auto desc = screen_.Desc();
	desc.Width = width;
	desc.Height = height;
	return ResizeScreen(desc);
}

HRESULT Graphics::AddGraphicsComandList(int count)
{
	auto result = S_OK;

	std::vector<CommandList*> tmpLists;
	for (auto i = 0; i < count; ++i)
	{
		auto pList = graphicsListContainer_.CreateCommandList();
		if (pList == nullptr)
		{
			result = S_FALSE;
			break;
		}
		tmpLists.push_back(pList);
	}

	if (SUCCEEDED(result))
	{
		for (auto pList : tmpLists)
		{
			pList->Close();
			graphicsListPtrs_.push_back(pList);
		}
	}

	return result;
}

void Graphics::ClearCommand()
{
	graphicsListContainer_.ClearState();
}

void Graphics::SubmitCommand(CommandList* pCommandList)
{
	commandQueue_.Submit(pCommandList);
}

void Graphics::SwapBuffers()
{
	screen_.SwapBuffers();
}

HRESULT Graphics::WaitForCommandExecution()
{
	auto result = S_OK;

	result = commandQueue_.WaitForExecution();
	if (FAILED(result))
	{
		return result;
	}

	screen_.UpdateFrameIndex();

	return result;
}
