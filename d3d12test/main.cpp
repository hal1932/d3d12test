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
#include "Model.h"

using Microsoft::WRL::ComPtr;

const int cScreenWidth = 1280;
const int cScreenHeight = 720;
const int cBufferCount = 2;
const int cModelGridSize = 10;


struct Scene
{
	ComPtr<ID3D12RootSignature> pRootSignature;
	ComPtr<ID3D12PipelineState> pPipelineState;
	float rotateAngle;

	Model models[cModelGridSize * cModelGridSize * cModelGridSize];

	ResourceViewHeap cbSrUavHeap;

	ModelTransform modelTransform;

	Camera camera;
	ShaderManager shaders;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	CommandListManager commandLists;
};
Scene* pScene = nullptr;

bool SetupScene(Graphics& g)
{
	auto pDevice = g.DevicePtr();
	auto pNativeDevice = pDevice->NativePtr();

	auto& commandListPtrs = pScene->commandLists.CreateCommandLists("main", 0);
	pScene->commandLists.CommitExecutionOrders();

	auto pCommandList = g.CreateCommandList(CommandList::SubmitType::Direct, 1);
	commandListPtrs.push_back(pCommandList);

	auto& rootModel = pScene->models[0];
	rootModel.Setup(pDevice, "assets/test_a.fbx");
	rootModel.UpdateSubresources(pCommandList, g.CommandQueuePtr());

	for (auto i = 1; i < _countof(pScene->models); ++i)
	{
		pScene->models[i].SetupAsReference(&rootModel);
	}
	
	pScene->cbSrUavHeap.CreateHeap(pDevice, { HeapDesc::ViewType::CbSrUaView, _countof(pScene->models) * 2 });

	for (auto& model : pScene->models)
	{
		model.SetupBuffers(&pScene->cbSrUavHeap);
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
		for (auto& model : pScene->models)
		{
			pScene->shaders.LoadFromModelMaterials(&model.FbxModel());
		}

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
		desc.InputLayout = pScene->shaders.InputLayout("Simple");
		desc.pRootSignature = pScene->pRootSignature.Get();
		desc.VS = pScene->shaders.VertexShader("Simple");
		desc.PS = pScene->shaders.PixelShader("Simple");
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

CpuStopwatchBatch sw;

void Calc()
{
	sw.Start(100, "calc");

	pScene->rotateAngle += 0.01f;

	for (auto i = 0; i < cModelGridSize; ++i)
	{
		for (auto j = 0; j < cModelGridSize; ++j)
		{
			for (auto k = 0; k < cModelGridSize; ++k)
			{
				const auto index = i * cModelGridSize * cModelGridSize + j * cModelGridSize + k;

				auto& model = pScene->models[index];

				auto* t = model.TransformPtr();
				t->SetScaling(0.1f, 0.1f, 0.1f);
				t->SetRotation(0.0f, pScene->rotateAngle, 0.0f);
				t->SetTranslation(-2.0f + i * 0.3f, -2.0f + j * 0.3f, -1.0f + k * 0.3f);
				t->UpdateMatrix();
			}
		}
	}

	sw.Stop(100);
}

void DrawModel(Model* pModel, ID3D12GraphicsCommandList* pCmdList)
{
	sw.Start(300, "model");

	sw.Start(301, "model-cbuffer");
	{
		pScene->modelTransform.World = pModel->TransformPtr()->Matrix();
		pModel->SetTransform(pScene->modelTransform);
	}
	sw.Stop(301);

	sw.Start(302, "model-descriptor");
	{
		pModel->SetRootDescriptorTable(pCmdList);
	}
	sw.Stop(302);

	sw.Start(303, "model-draw");
	pModel->Draw(pCmdList);
	sw.Stop(303);

	sw.Stop(300);
}

void Draw(Graphics& g, GpuStopwatch* pStopwatch)
{
	sw.Start(200, "all");

	sw.Start(201, "transform");
	{
		auto& c = pScene->camera;

		c.SetPosition({ 3.0f, 3.0f, -6.0f });
		c.SetFocus({ 0.0f, 0.0f, 0.0f });
		c.SetUp({ 0.0f, 1.0f, 0.0f });

		c.SetFovY(DirectX::XM_PIDIV4);
		c.SetAspect(g.ScreenPtr()->AspectRatio());
		c.SetNearPlane(0.1f);
		c.SetFarPlane(1000.0f);

		c.UpdateMatrix();

		pScene->modelTransform.View = c.View();
		pScene->modelTransform.Proj = c.Proj();
	}
	sw.Stop(201);

	auto pGraphicsList = pScene->commandLists.GetCommandList("main")[0];
	auto pNativeGraphicsList = pGraphicsList->GraphicsList();

	sw.Start(203, "open");
	{
		pGraphicsList->Open(pScene->pPipelineState.Get());
	}
	sw.Stop(203);

	pStopwatch->Start(g.CommandQueuePtr()->NativePtr(), pNativeGraphicsList);

	sw.Start(204, "cbv_descriptor");
	{
		auto heap = pScene->cbSrUavHeap.NativePtr();
		pNativeGraphicsList->SetDescriptorHeaps(1, &heap);
	}
	sw.Stop(204);

	sw.Start(205, "rootsig");
	{
		pNativeGraphicsList->SetGraphicsRootSignature(pScene->pRootSignature.Get());
	}
	sw.Stop(205);

	sw.Start(206, "RS");
	{
		const auto& screen = g.ScreenPtr()->Desc();
		pScene->viewport = { 0.0f, 0.0f, (float)screen.Width, (float)screen.Height, 0.0f, 1.0f };
		pNativeGraphicsList->RSSetViewports(1, &pScene->viewport);

		pScene->scissorRect = { 0, 0, screen.Width, screen.Height };
		pNativeGraphicsList->RSSetScissorRects(1, &pScene->scissorRect);
	}
	sw.Stop(206);

	D3D12_RESOURCE_BARRIER barrier = {};

	sw.Start(207, "OM");
	{
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
	}
	sw.Stop(207);

	sw.Start(208, "IA");
	{
		pNativeGraphicsList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	sw.Stop(208);

	sw.Start(209, "models");
	{
		for (auto& model : pScene->models)
		{
			DrawModel(&model, pNativeGraphicsList);
		}
	}
	sw.Stop(209);

	sw.Start(210, "wait_render");
	{
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		pNativeGraphicsList->ResourceBarrier(1, &barrier);
	}
	sw.Stop(210);

	pStopwatch->Stop();

	sw.Start(211, "shutdown");
	{
		pGraphicsList->Close();
	}
	sw.Stop(211);

	sw.Start(212, "submit_cmd");
	{
		g.SubmitCommand(pGraphicsList);
	}
	sw.Stop(212);

	sw.Start(213, "swap");
	{
		g.SwapBuffers();
	}
	sw.Stop(213);

	sw.Start(214, "wait_cmd");
	{
		g.WaitForCommandExecution();
	}
	sw.Stop(214);

	sw.Stop(200);
	if (sw.DumpAll(200, 60))
	{
		sw.Reset();
	}
}

void ShutdownScene()
{
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

	CpuStopwatch sw;
	GpuStopwatch gsw;
	gsw.Create(graphics.DevicePtr()->NativePtr(), 1);

	FrameCounter counter(&sw, &gsw);

	window.MessageLoop([&graphics, &counter]()
	{
		counter.CpuWatchPtr()->Start();

		Calc();
		Draw(graphics, counter.GpuWatchPtr());

		counter.CpuWatchPtr()->Stop();
		counter.NextFrame();

		if (counter.CpuTime() > 1000.0)
		{
			const auto frames = counter.FrameCount();
			printf(
				"fps: %d, CPU: %.4f %%, GPU: %.4f %%\n",
				counter.FrameCount(),
				counter.CpuUtilization(60),
				counter.GpuUtilization(60));
			counter.Reset();
		}
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
