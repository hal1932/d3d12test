#include <stdlib.h>
#include <crtdbg.h>

#include <memory>

#include <wrl.h>
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <iostream>

#include "lib/lib.h"
#include "Graphics.h"

using Microsoft::WRL::ComPtr;

const int cScreenWidth = 1280;
const int cScreenHeight = 720;
const int cBufferCount = 2;


__declspec(align(256))
struct ModelTransform
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

	std::unique_ptr<fbx::Model> modelPtr[100];

	ResourceViewHeap cbSrUavHeap;

	ModelTransform modelTransform;

	std::unique_ptr<Resource> modelTransformCbvPtr[100];
	void* modelTransformBuffer[100];

	Resource* pTextureSrv[100];

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
};
Scene* pScene = nullptr;

bool SetupScene(Graphics& g)
{
	auto pDevice = g.DevicePtr();
	auto pNativeDevice = pDevice->NativePtr();

	pScene->modelPtr[0] = std::make_unique<fbx::Model>();
	pScene->modelPtr[0]->LoadFromFile("assets/test_a.fbx");
	pScene->modelPtr[0]->UpdateResources(pDevice);
	pScene->modelPtr[0]->UpdateSubresources(g.GraphicsListPtr(0), g.CommandQueuePtr());

	for (auto i = 1; i < _countof(pScene->modelPtr); ++i)
	{
		pScene->modelPtr[i] = std::unique_ptr<fbx::Model>(pScene->modelPtr[0]->CreateReference());
	}
	
	pScene->cbSrUavHeap.CreateHeap(pDevice, { HeapDesc::ViewType::CbSrUaView, _countof(pScene->modelPtr) * 2 });

	for (auto i = 0; i < _countof(pScene->modelPtr); ++i)
	{
		pScene->modelTransformCbvPtr[i] = std::unique_ptr<Resource>(
			pScene->cbSrUavHeap.CreateConstantBufferView({ sizeof(pScene->modelTransform), D3D12_TEXTURE_LAYOUT_ROW_MAJOR }));

		pScene->pTextureSrv[i] = pScene->cbSrUavHeap.CreateShaderResourceView(
			{ D3D12_SRV_DIMENSION_TEXTURE2D,{ pScene->modelPtr[i]->MeshPtr(0)->MaterialPtr()->TexturePtr() } });

		pScene->modelTransformBuffer[i] = pScene->modelTransformCbvPtr[i]->Map(0);
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

		ThrowIfFailed(pNativeDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&pScene->pRootSignature)));
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
		desc.pRootSignature = pScene->pRootSignature.Get();
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

		ThrowIfFailed(pNativeDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pScene->pPipelineState)));
	}

	pScene->rotateAngle = 0.f;

	return true;
}

void Calc()
{
	pScene->rotateAngle += 0.01f;

	for (auto i = 0; i < 10; ++i)
	{
		for (auto j = 0; j < 10; ++j)
		{
			const auto index = i * 10 + j;
			auto pModel = pScene->modelPtr[index].get();

			auto t = pModel->TransformPtr();
			t->SetScaling(0.1f, 0.1f, 0.1f);
			t->SetRotation(0.0f, pScene->rotateAngle, 0.0f);
			t->SetTranslation(-1.0f + i * 0.25f, -1.0f + j * 0.25f, 0.0f);
			t->UpdateMatrix();
		}
	}
}

void DrawModel(ID3D12GraphicsCommandList* pCmdList, fbx::Model* pModel, Resource* pTexSrv, Resource* pCbv, void* cbuffer)
{
	pScene->modelTransform.World = pModel->TransformPtr()->Matrix();
	memcpy(cbuffer, &pScene->modelTransform, sizeof(pScene->modelTransform));

	pCmdList->SetGraphicsRootDescriptorTable(0, pCbv->GpuDescriptorHandle());
	pCmdList->SetGraphicsRootDescriptorTable(1, pTexSrv->GpuDescriptorHandle());

	const auto pMesh = pModel->MeshPtr(0);
	for (auto i = 0; i < pModel->MeshCount(); ++i)
	{
		const auto pMesh = pModel->MeshPtr(i);

		auto vbView = pMesh->VertexBuffer()->GetVertexBufferView(sizeof(fbx::Mesh::Vertex));
		pCmdList->IASetVertexBuffers(0, 1, &vbView);

		auto ibView = pMesh->IndexBuffer()->GetIndexBufferView(DXGI_FORMAT_R16_UINT);
		pCmdList->IASetIndexBuffer(&ibView);

		pCmdList->DrawIndexedInstanced(pMesh->IndexCount(), 1, 0, 0, 0);
	}
}

void Draw(Graphics& g)
{
	pScene->modelTransform.View = DirectX::XMMatrixLookAtLH({ 0.0f, 0.0f, -5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
	pScene->modelTransform.Proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, g.ScreenPtr()->AspectRatio(), 1.0f, 1000.f);

	g.ClearCommand();

	auto pGraphicsList = g.GraphicsListPtr(0);
	auto pNativeGraphicsList = pGraphicsList->AsGraphicsList();

	pGraphicsList->Open(pScene->pPipelineState.Get());

	{
		auto heap = pScene->cbSrUavHeap.NativePtr();
		pNativeGraphicsList->SetDescriptorHeaps(1, &heap);

		pNativeGraphicsList->SetGraphicsRootSignature(pScene->pRootSignature.Get());
	}

	const auto& screen = g.ScreenPtr()->Desc();
	pScene->viewport = { 0.0f, 0.0f, (float)screen.Width, (float)screen.Height, 0.0f, 1.0f };
	pNativeGraphicsList->RSSetViewports(1, &pScene->viewport);

	pScene->scissorRect = { 0, 0, screen.Width, screen.Height };
	pNativeGraphicsList->RSSetScissorRects(1, &pScene->scissorRect);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = g.CurrentRenderTargetPtr()->NativePtr();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	pNativeGraphicsList->ResourceBarrier(1, &barrier);

	auto handleRTV = g.CurrentRenderTargetPtr()->CpuDescriptorHandle();
	auto handleDSV = g.DepthStencilPtr()->CpuDescriptorHandle();

	pNativeGraphicsList->OMSetRenderTargets(1, &handleRTV, FALSE, &handleDSV);

	FLOAT clearValue[] = { 0.2f, 0.2f, 0.5f, 1.0f };
	pNativeGraphicsList->ClearRenderTargetView(handleRTV, clearValue, 0, nullptr);
	pNativeGraphicsList->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	pNativeGraphicsList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto i = 0; i < _countof(pScene->modelPtr); ++i)
	{
		auto pModel = pScene->modelPtr[i].get();
		auto pTexSrv = pScene->pTextureSrv[i];
		auto pCbv = pScene->modelTransformCbvPtr[i].get();
		auto cbuffer = pScene->modelTransformBuffer[i];

		DrawModel(pNativeGraphicsList, pModel, pTexSrv, pCbv, cbuffer);
	}

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	pNativeGraphicsList->ResourceBarrier(1, &barrier);

	pGraphicsList->Close();

	g.SubmitCommand(pGraphicsList);
	g.SwapBuffers();

	g.WaitForCommandExecution();
}

void ShutdownScene()
{
	for (auto& modelPtr : pScene->modelPtr)
	{
		modelPtr.reset();
	}

	for (auto i = 0; i < _countof(pScene->modelTransformCbvPtr); ++i)
	{
		pScene->modelTransformCbvPtr[i]->Unmap(0);
	}
}

int MainImpl(int, char**)
{
	fbx::Model::Setup();

	Window window;
	window.Setup(GetModuleHandle(nullptr), _TEXT("d3d12test"));

	window.Move(300, 200);
	window.Resize(1280, 720);
	window.Open();

	Graphics graphics;

	window.SetEventHandler(WindowEvent::Resize, [&window, &graphics](auto e)
	{
		graphics.WaitForCommandExecution();

		auto pArg = static_cast<ResizeEventArg*>(e);
		graphics.ResizeScreen(pArg->Width, pArg->Height);
	});

	graphics.Setup(true);
	graphics.AddGraphicsComandList(1);

	ScreenContextDesc desc = {};
	desc.BufferCount = cBufferCount;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.Width = cScreenWidth;
	desc.Height = cScreenHeight;
	desc.OutputWindow = window.Handle();
	desc.Windowed = true;

	graphics.ResizeScreen(desc);

	pScene = new Scene();
	SetupScene(graphics);

	window.MessageLoop([&graphics]()
	{
		Calc();
		Draw(graphics);
	});

	graphics.WaitForCommandExecution();

	ShutdownScene();
	SafeDelete(&pScene);

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
