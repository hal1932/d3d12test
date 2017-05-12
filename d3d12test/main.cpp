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

#include "common.h"
#include "Window.h"
#include "Device.h"
#include "ScreenContext.h"
#include "GpuFence.h"
#include "CommandQueue.h"
#include "CommandContainer.h"
#include "CommandList.h"
#include "ResourceViewHeap.h"
#include "Resource.h"
#include "Shader.h"
#include "fbxModel.h"
#include "fbxMesh.h"
#include "fbxMaterial.h"

#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

typedef std::basic_string<TCHAR> tstring;

const int cScreenWidth = 1280;
const int cScreenHeight = 720;
const int cBufferCount = 2;


struct Graphics
{
	Device device;
	ScreenContext screen;

	ResourceViewHeap renderTargetViewHeap;
	std::vector<Resource*> renderTargetViewPrts;

	ResourceViewHeap depthStencilViewHeap;
	Resource* pDepthStencilView;

	CommandQueue commandQueue;

	CommandContainer commandContainer;
	CommandList* pCommandList;

	D3D12_VIEWPORT viewPort;
	D3D12_RECT scissorRect;
};
Graphics gfx;

bool SetupGraphics(HWND hWnd)
{
	gfx.device.EnableDebugLayer();
	gfx.device.Create();

	gfx.commandQueue.Create(&gfx.device);

	{
		ScreenContextDesc desc;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.Width = cScreenWidth;
		desc.Height = cScreenHeight;
		desc.OutputWindow = hWnd;

		gfx.screen.Create(&gfx.device, &gfx.commandQueue, desc);
	}

	gfx.renderTargetViewHeap.CreateHeap(&gfx.device, { HeapDesc::ViewType::RenderTargetView, 2 });
	gfx.renderTargetViewPrts = gfx.renderTargetViewHeap.CreateRenderTargetViewFromBackBuffer(&gfx.screen);

	gfx.depthStencilViewHeap.CreateHeap(&gfx.device, { HeapDesc::ViewType::DepthStencilView, 1 });
	gfx.pDepthStencilView = gfx.depthStencilViewHeap.CreateDepthStencilView(
		&gfx.screen,
		{ cScreenWidth, cScreenHeight, DXGI_FORMAT_D24_UNORM_S8_UINT, 1.0f, 0 });

	gfx.commandContainer.Create(&gfx.device);

	gfx.pCommandList = gfx.commandContainer.AddGraphicsList();
	gfx.pCommandList->Close();
	
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

	fbx::Model* pModel;

	TransformBuffer transformBuffer;
	ResourceViewHeap cbSrUavHeap;
	Resource* pTransformCbv;
	Resource* pTextureSrv;
	void* pCbvData;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
};
Scene scene;

bool SetupScene()
{
	auto pNativeDevice = gfx.device.NativePtr();

	scene.pModel = new fbx::Model();
	scene.pModel->LoadFromFile("assets/test_a.fbx");
	scene.pModel->UpdateResources(&gfx.device);
	scene.pModel->UpdateSubresources(gfx.pCommandList, &gfx.commandQueue);
	
	scene.cbSrUavHeap.CreateHeap(&gfx.device, { HeapDesc::ViewType::CbSrUaView, 2 });
	scene.pTransformCbv = scene.cbSrUavHeap.CreateConstantBufferView({ sizeof(scene.transformBuffer), D3D12_TEXTURE_LAYOUT_ROW_MAJOR });

	{
		scene.pTextureSrv = scene.cbSrUavHeap.CreateShaderResourceView({ D3D12_SRV_DIMENSION_TEXTURE2D,{ scene.pModel->MeshPtr(0)->MaterialPtr()->TexturePtr() } });

		scene.pCbvData = scene.pTransformCbv->Map(0);

		scene.transformBuffer.World = DirectX::XMMatrixIdentity();
		scene.transformBuffer.View = DirectX::XMMatrixLookAtLH({ 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
		scene.transformBuffer.Proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, (float)cScreenWidth / (float)cScreenHeight, 1.0f, 1000.f);

		memcpy(scene.pCbvData, &scene.transformBuffer, sizeof(scene.transformBuffer));
	}

	{
		D3D12_DESCRIPTOR_RANGE range[2];
		range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		range[0].NumDescriptors = 1;
		range[0].BaseShaderRegister = 0;
		range[0].RegisterSpace = 0;
		range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range[1].NumDescriptors = 1;
		range[1].BaseShaderRegister = 0;
		range[1].RegisterSpace = 0;
		range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER param[2];
		param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		param[0].DescriptorTable.NumDescriptorRanges = 1;
		param[0].DescriptorTable.pDescriptorRanges = &range[0];

		param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		param[1].DescriptorTable.NumDescriptorRanges = 1;
		param[1].DescriptorTable.pDescriptorRanges = &range[1];

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = _countof(param);
		desc.pParameters = param;
		desc.NumStaticSamplers = 1;
		desc.pStaticSamplers = &sampler;
		desc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		ComPtr<ID3DBlob> pSignature;
		ComPtr<ID3DBlob> pError;

		ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError));

		ThrowIfFailed(pNativeDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&scene.pRootSignature)));
	}

	{
		Shader vs;
		vs.CreateFromSourceFile({ L"assets/SimpleVS.hlsl", _T("VSFunc"), _T("vs_5_0") });
		vs.CreateInputLayout();

		Shader ps;
		ps.CreateFromSourceFile({ L"assets/SimplePS.hlsl", _T("PSFunc"), _T("ps_5_0") });

		D3D12_RASTERIZER_DESC descRS = {};
		descRS.FillMode = D3D12_FILL_MODE_SOLID;
		descRS.CullMode = D3D12_CULL_MODE_BACK;
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
		desc.InputLayout = vs.NativeInputLayout();
		desc.pRootSignature = scene.pRootSignature.Get();
		desc.VS = vs.NativeByteCode();
		desc.PS = ps.NativeByteCode();
		desc.RasterizerState = descRS;
		desc.BlendState = descBS;
		desc.DepthStencilState.DepthEnable = TRUE;
		desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthStencilState.StencilEnable = FALSE;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.SampleDesc.Count = 1;

		ThrowIfFailed(pNativeDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&scene.pPipelineState)));
	}

	scene.rotateAngle = 0.f;

	return true;
}

void WaitForCommandExecution()
{
	gfx.commandQueue.WaitForExecution();

	// バックバッファのインデックスを更新
	gfx.screen.UpdateFrameIndex();
}

void Draw()
{
	scene.rotateAngle += 0.01f;

	scene.transformBuffer.World = DirectX::XMMatrixRotationY(scene.rotateAngle);
	memcpy(scene.pCbvData, &scene.transformBuffer, sizeof(scene.transformBuffer));

	gfx.commandContainer.ClearState();
	gfx.pCommandList->Open(scene.pPipelineState.Get());

	auto pCmdList = gfx.pCommandList->AsGraphicsList();

	{
		auto heap = scene.cbSrUavHeap.NativePtr();
		pCmdList->SetDescriptorHeaps(1, &heap);

		pCmdList->SetGraphicsRootSignature(scene.pRootSignature.Get());

		pCmdList->SetGraphicsRootDescriptorTable(0, scene.cbSrUavHeap.GpuHandle(0));
		pCmdList->SetGraphicsRootDescriptorTable(1, scene.cbSrUavHeap.GpuHandle(1));
	}

	scene.viewport = { 0.0f, 0.0f, (float)cScreenWidth, (float)cScreenHeight, 0.0f, 1.0f };
	pCmdList->RSSetViewports(1, &scene.viewport);

	scene.scissorRect = { 0, 0, cScreenWidth, cScreenHeight };
	pCmdList->RSSetScissorRects(1, &scene.scissorRect);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = gfx.renderTargetViewPrts[gfx.screen.FrameIndex()]->NativePtr();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	pCmdList->ResourceBarrier(1, &barrier);

	auto handleRTV = gfx.renderTargetViewHeap.CpuHandle(gfx.screen.FrameIndex());
	auto handleDSV = gfx.depthStencilViewHeap.CpuHandle(0);

	pCmdList->OMSetRenderTargets(1, &handleRTV, FALSE, &handleDSV);

	FLOAT clearValue[] = { 0.2f, 0.2f, 0.5f, 1.0f };
	pCmdList->ClearRenderTargetView(handleRTV, clearValue, 0, nullptr);
	pCmdList->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	pCmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const auto& pMesh = scene.pModel->MeshPtr(0);
	auto vbView = pMesh->VertexBuffer()->GetVertexBufferView(sizeof(fbx::Mesh::Vertex));
	pCmdList->IASetVertexBuffers(0, 1, &vbView);

	auto ibView = pMesh->IndexBuffer()->GetIndexBufferView(DXGI_FORMAT_R16_UINT);
	pCmdList->IASetIndexBuffer(&ibView);

	pCmdList->DrawIndexedInstanced(pMesh->IndexCount(), 1, 0, 0, 0);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	pCmdList->ResourceBarrier(1, &barrier);

	gfx.pCommandList->Close();

	gfx.commandQueue.Submit(gfx.pCommandList);

	gfx.screen.SwapBuffers();

	WaitForCommandExecution();
}

void ShutdownScene()
{
	SafeDelete(&scene.pModel);
	scene.pTransformCbv->Unmap(0);
	SafeDelete(&scene.pTransformCbv);
	scene.pPipelineState.Reset();
	scene.pRootSignature.Reset();
}

void ShutdownGraphics()
{
	SafeDelete(&gfx.pDepthStencilView);
	for (auto pResource : gfx.renderTargetViewPrts)
	{
		SafeDelete(&pResource);
	}
	WaitForCommandExecution();
}

int MainImpl(int, char**)
{
	fbx::Model::Setup();

	auto hInstance = GetModuleHandle(nullptr);

	Window window;
	window.Setup(GetModuleHandle(nullptr), _TEXT("d3d12test"));

	window.Move(300, 200);
	window.Resize(1280, 720);
	window.Open();

	window.SetEventHandler(WindowEvent::Resize, [](auto e)
	{
		auto arg = static_cast<ResizeEventArg*>(e);
	});

	SetupGraphics(window.Handle());
	SetupScene();

	{
		MSG msg = {};
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

	window.Close();

	fbx::Model::Shutdown();

	return 0;
}

int main(int argc, char** argv)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	return MainImpl(argc, argv);
}
