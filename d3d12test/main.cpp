#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>  

#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <d3dcommon.h>
#include <DirectXMath.h>

#include <wrl.h>
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <iostream>

#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

typedef std::basic_string<TCHAR> tstring;

const int cScreenWidth = 1280;
const int cScreenHeight = 720;
const int cBufferCount = 2;

tstring GetLastErrorMessage()
{
	auto err = GetLastError();
	LPVOID msg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg, 0, nullptr);
	std::string msgStr((LPTSTR)msg);
	LocalFree(msg);
	return msgStr;
}

void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		auto err = GetLastErrorMessage();
		throw std::exception(err.c_str());
	}
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND SetupWindow(HINSTANCE hInstance, int width, int height)
{
	WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW,
		WindowProc,
		0, 0,
		hInstance,
		LoadIcon(hInstance, IDI_APPLICATION),
		LoadCursor(nullptr, IDC_ARROW),
		(HBRUSH)COLOR_WINDOW,
		nullptr,
		"d3d12test",
		LoadIcon(hInstance, IDI_APPLICATION)
	};

	if (!RegisterClassEx(&wcex))
	{
		std::cerr << "RegisterClassEx: " << GetLastErrorMessage();
		return nullptr;
	}

	auto hWnd = CreateWindow("d3d12test", TEXT("d3d12test_window"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd)
	{
		std::cerr << "CreateWindow: " << GetLastErrorMessage();
		return nullptr;
	}

	return hWnd;
}

void ShutdownWindow(HINSTANCE hInstance, HWND hWnd)
{
	UnregisterClass("d3d12test", hInstance);
	CloseWindow(hWnd);
}

struct Graphics
{
	ComPtr<ID3D12Device> pDevice;
	ComPtr<IDXGISwapChain3> pSwapChain;
	ComPtr<ID3D12CommandQueue> pCommandQueue;

	int frameIndex;

	ComPtr<ID3D12DescriptorHeap> pRtvHeap;
	UINT rtvDescriptorSize;

	ComPtr<ID3D12DescriptorHeap> pDsvHeap;
	UINT dsvDescriptorSize;

	ComPtr<ID3D12Resource> pRenderTargetViews[cBufferCount];
	ComPtr<ID3D12Resource> pDepthStencilView;

	ComPtr<ID3D12CommandAllocator> pCommandAllocator;
	
	ComPtr<ID3D12GraphicsCommandList> pCommandList;

	ComPtr<ID3D12Fence> pFence;
	UINT64 fenceValue;
	HANDLE fenceEvent;

	D3D12_VIEWPORT viewPort;
	D3D12_RECT scissorRect;
};
Graphics gfx;

bool SetupGraphics(HWND hWnd)
{
	auto dxgiFactoryFlags = 0U;

#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> pDebugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController))))
		{
			pDebugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> pFactory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pFactory)));

	ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&gfx.pDevice)));

	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		ThrowIfFailed(gfx.pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&gfx.pCommandQueue)));
	}

	{
		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferCount = cBufferCount;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.Width = cScreenWidth;
		desc.BufferDesc.Height = cScreenHeight;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.OutputWindow = hWnd;
		desc.SampleDesc.Count = 1;
		desc.Windowed = TRUE;

		ComPtr<IDXGISwapChain> pSwapChain;
		ThrowIfFailed(pFactory->CreateSwapChain(gfx.pCommandQueue.Get(), &desc, &pSwapChain));
		ThrowIfFailed(pSwapChain->QueryInterface(IID_PPV_ARGS(&gfx.pSwapChain)));

		gfx.frameIndex = gfx.pSwapChain->GetCurrentBackBufferIndex();
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = cBufferCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ThrowIfFailed(gfx.pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gfx.pRtvHeap)));

		gfx.rtvDescriptorSize = gfx.pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		
		ThrowIfFailed(gfx.pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gfx.pDsvHeap)));

		gfx.dsvDescriptorSize = gfx.pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	{
		auto handle = gfx.pRtvHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		for (UINT i = 0; i < cBufferCount; ++i)
		{
			ThrowIfFailed(gfx.pSwapChain->GetBuffer(i, IID_PPV_ARGS(&gfx.pRenderTargetViews[i])));
			gfx.pDevice->CreateRenderTargetView(gfx.pRenderTargetViews[i].Get(), &desc, handle);
			handle.ptr += gfx.rtvDescriptorSize;
		}
	}

	{
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_DEFAULT;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = cScreenWidth;
		desc.Height = cScreenHeight;
		desc.DepthOrArraySize = 1;
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.SampleDesc.Count = 1;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		ThrowIfFailed(gfx.pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&gfx.pDepthStencilView)));

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

		gfx.pDevice->CreateDepthStencilView(gfx.pDepthStencilView.Get(), &dsvDesc, gfx.pDsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	{
		ThrowIfFailed(gfx.pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gfx.pCommandAllocator)));
	}

	{
		ThrowIfFailed(gfx.pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gfx.pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&gfx.pCommandList)));
		gfx.pCommandList->Close();
	}

	{
		ThrowIfFailed(gfx.pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gfx.pFence)));
		gfx.fenceValue = 1;

		gfx.fenceEvent = CreateEventEx(nullptr, FALSE, 0, EVENT_ALL_ACCESS);
		if (!gfx.fenceEvent)
		{
			std::cerr << "CreateEventEx: " << GetLastErrorMessage();
			return false;
		}
	}

	{
		gfx.viewPort = {};
		gfx.viewPort.Width = (FLOAT)cScreenWidth;
		gfx.viewPort.Height = (FLOAT)cScreenHeight;
		gfx.viewPort.MinDepth = 0.0f;
		gfx.viewPort.MaxDepth = 1.0f;
	}

	{
		gfx.scissorRect = {};
		gfx.scissorRect.right = cScreenWidth;
		gfx.scissorRect.bottom = cScreenHeight;
	}

	return true;
}

__declspec(align(256))
struct TransformBuffer
{
	DirectX::XMMATRIX World;
	DirectX::XMMATRIX View;
	DirectX::XMMATRIX Proj;
};

struct Scene
{
	ComPtr<ID3D12RootSignature> pRootSignature;
	ComPtr<ID3D12PipelineState> pPipelineState;
	float rotateAngle;

	ComPtr<ID3D12Resource> pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	TransformBuffer transformBuffer;
	ComPtr<ID3D12DescriptorHeap> pCbvHeap;
	ComPtr<ID3D12Resource> pConstantBuffer;
	UINT8* pCbvData;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
};
Scene scene;

struct Vertex
{
	float Position[3];
	float Normal[3];
	float TexCoord[2];
	float Color[4];
};

ComPtr<ID3DBlob> CompileShader(LPCWSTR filepath, const TCHAR* entryPoint, const TCHAR* profile)
{
	ComPtr<ID3DBlob> shader;
	ComPtr<ID3DBlob> error;

	ThrowIfFailed(D3DCompileFromFile(filepath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, profile, D3DCOMPILE_DEBUG, 0, &shader, &error));

	return shader;
}

bool SetupScene()
{
	{
		Vertex vertices[] =
		{
			{ {  0.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
		};

		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = sizeof(vertices);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		ThrowIfFailed(gfx.pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&scene.pVertexBuffer)));

		UINT8* pData;
		ThrowIfFailed(scene.pVertexBuffer->Map(0, nullptr, (void**)&pData));
		memcpy(pData, vertices, sizeof(vertices));
		scene.pVertexBuffer->Unmap(0, nullptr);

		scene.vertexBufferView.BufferLocation = scene.pVertexBuffer->GetGPUVirtualAddress();
		scene.vertexBufferView.SizeInBytes = sizeof(vertices);
		scene.vertexBufferView.StrideInBytes = sizeof(Vertex);
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		ThrowIfFailed(gfx.pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&scene.pCbvHeap)));
	}

	{
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = sizeof(scene.transformBuffer);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		ThrowIfFailed(gfx.pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&scene.pConstantBuffer)));

		D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
		viewDesc.BufferLocation = scene.pConstantBuffer->GetGPUVirtualAddress();
		viewDesc.SizeInBytes = sizeof(scene.transformBuffer);

		gfx.pDevice->CreateConstantBufferView(&viewDesc, scene.pCbvHeap->GetCPUDescriptorHandleForHeapStart());

		// リソースを解放するまでMapしっぱなしでOK
		ThrowIfFailed(scene.pConstantBuffer->Map(0, nullptr, (void**)&scene.pCbvData));

		scene.transformBuffer.World = DirectX::XMMatrixIdentity();
		scene.transformBuffer.View = DirectX::XMMatrixLookAtLH({ 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
		scene.transformBuffer.Proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, (float)cScreenWidth / (float)cScreenHeight, 1.0f, 1000.f);

		memcpy(scene.pCbvData, &scene.transformBuffer, sizeof(scene.transformBuffer));
	}

	{
		D3D12_DESCRIPTOR_RANGE range = {};
		range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		range.NumDescriptors = 1;
		range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		param.DescriptorTable.NumDescriptorRanges = 1;
		param.DescriptorTable.pDescriptorRanges = &range;

		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = 1;
		desc.pParameters = &param;
		desc.NumStaticSamplers = 0;
		desc.pStaticSamplers = nullptr;
		desc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		ComPtr<ID3DBlob> pSignature;
		ComPtr<ID3DBlob> pError;

		ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError));

		ThrowIfFailed(gfx.pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&scene.pRootSignature)));
	}

	{
		auto pVS = CompileShader(L"assets/SimpleVS.hlsl", _T("VSFunc"), _T("vs_5_0"));
		auto pPS = CompileShader(L"assets/SimplePS.hlsl", _T("PSFunc"), _T("ps_5_0"));

		D3D12_INPUT_ELEMENT_DESC inputElements[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "VTX_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC descRS = {};
		descRS.FillMode = D3D12_FILL_MODE_SOLID;
		descRS.CullMode = D3D12_CULL_MODE_NONE;
		descRS.DepthClipEnable = TRUE;

		D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {
			FALSE, FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};

		D3D12_BLEND_DESC descBS = {};
		descBS.AlphaToCoverageEnable = FALSE;
		descBS.IndependentBlendEnable = FALSE;
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			descBS.RenderTarget[i] = descRTBS;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = { inputElements, _countof(inputElements) };
		desc.pRootSignature = scene.pRootSignature.Get();
		desc.VS = { reinterpret_cast<UINT8*>(pVS->GetBufferPointer()), pVS->GetBufferSize() };
		desc.PS = { reinterpret_cast<UINT8*>(pPS->GetBufferPointer()), pPS->GetBufferSize() };
		desc.RasterizerState = descRS;
		desc.BlendState = descBS;
		desc.DepthStencilState.DepthEnable = FALSE;
		desc.DepthStencilState.StencilEnable = FALSE;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		desc.SampleDesc.Count = 1;

		ThrowIfFailed(gfx.pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&scene.pPipelineState)));
	}

	scene.rotateAngle = 0.f;

	return true;
}

void WaitForCommandExecution()
{
	// コマンド実行の完了待ち
	const UINT64 fence = gfx.fenceValue;
	ThrowIfFailed(gfx.pCommandQueue->Signal(gfx.pFence.Get(), fence));

	++gfx.fenceValue;

	if (gfx.pFence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(gfx.pFence->SetEventOnCompletion(fence, gfx.fenceEvent));
		WaitForSingleObject(gfx.fenceEvent, INFINITE);
	}

	// バックバッファのインデックスを更新
	gfx.frameIndex = gfx.pSwapChain->GetCurrentBackBufferIndex();
}

void Draw()
{
	scene.rotateAngle += 0.1f;

	scene.transformBuffer.World = DirectX::XMMatrixRotationY(scene.rotateAngle);
	memcpy(scene.pCbvData, &scene.transformBuffer, sizeof(scene.transformBuffer));

	gfx.pCommandAllocator->Reset();
	gfx.pCommandList->Reset(gfx.pCommandAllocator.Get(), scene.pPipelineState.Get());

	auto& pCmdList = gfx.pCommandList;

	pCmdList->SetDescriptorHeaps(1, scene.pCbvHeap.GetAddressOf());
	pCmdList->SetGraphicsRootSignature(scene.pRootSignature.Get());
	pCmdList->SetGraphicsRootDescriptorTable(0, scene.pCbvHeap->GetGPUDescriptorHandleForHeapStart());

	scene.viewport = { 0.0f, 0.0f, (float)cScreenWidth, (float)cScreenHeight, 0.0f, 1.0f };
	pCmdList->RSSetViewports(1, &scene.viewport);

	scene.scissorRect = { 0, 0, cScreenWidth, cScreenHeight };
	pCmdList->RSSetScissorRects(1, &scene.scissorRect);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = gfx.pRenderTargetViews[gfx.frameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	pCmdList->ResourceBarrier(1, &barrier);

	auto handleRTV = gfx.pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	handleRTV.ptr += (gfx.frameIndex * gfx.rtvDescriptorSize);

	auto handleDSV = gfx.pDsvHeap->GetCPUDescriptorHandleForHeapStart();

	pCmdList->OMSetRenderTargets(1, &handleRTV, FALSE, &handleDSV);

	FLOAT clearValue[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	pCmdList->ClearRenderTargetView(handleRTV, clearValue, 0, nullptr);
	pCmdList->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	pCmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pCmdList->IASetVertexBuffers(0, 1, &scene.vertexBufferView);

	pCmdList->DrawInstanced(3, 1, 0, 0);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	pCmdList->ResourceBarrier(1, &barrier);

	pCmdList->Close();

	ID3D12CommandList* ppCmdLists[] = { pCmdList.Get() };
	gfx.pCommandQueue->ExecuteCommandLists(_countof(ppCmdLists), ppCmdLists);

	gfx.pSwapChain->Present(1, 0);

	WaitForCommandExecution();
}

void ShutdownScene()
{
	scene.pConstantBuffer->Unmap(0, nullptr);

	scene.pConstantBuffer.Reset();
	scene.pCbvHeap.Reset();
	scene.pVertexBuffer.Reset();
	scene.pPipelineState.Reset();
	scene.pRootSignature.Reset();
}

void ShutdownGraphics()
{
	WaitForCommandExecution();

	CloseHandle(gfx.fenceEvent);

	for (auto i = 0; i < cBufferCount; ++i)
	{
		gfx.pRenderTargetViews[i].Reset();
	}

	gfx.pDepthStencilView.Reset();

	gfx.pSwapChain.Reset();
	gfx.pFence.Reset();
	gfx.pCommandList.Reset();
	gfx.pCommandAllocator.Reset();
	gfx.pCommandQueue.Reset();
	gfx.pDevice.Reset();
}

int MainImpl(int, char**)
{
	auto hInstance = GetModuleHandle(nullptr);

	auto hWnd = SetupWindow(hInstance, cScreenWidth, cScreenHeight);
	if (!hWnd)
	{
		return 1;
	}

	SetupGraphics(hWnd);

	ShowWindow(hWnd, SW_SHOWNORMAL);

	SetupScene();

	{
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));

		while (msg.message != WM_QUIT)
		{
			auto nextMsg = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
			if (nextMsg)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Draw();
			}
		}
	}

	ShutdownScene();
	ShutdownGraphics();
	ShutdownWindow(hInstance, hWnd);

	return 0;
}

int main(int argc, char** argv)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	return MainImpl(argc, argv);
}
