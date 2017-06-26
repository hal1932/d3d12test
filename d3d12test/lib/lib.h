#pragma once

#include "common.h"
#include "Window.h"
#include "Device.h"
#include "ScreenContext.h"
#include "GpuFence.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "ResourceViewHeap.h"
#include "Resource.h"
#include "Shader.h"
#include "fbxModel.h"
#include "fbxMesh.h"
#include "fbxMaterial.h"
#include "CpuStopwatch.h"
#include "GpuStopwatch.h"
#include "FrameCounter.h"
#include "CpuStopwatchBatch.h"
#include "Camera.h"
#include "ShaderManager.h"
#include "CommandListManager.h"
#include "TaskQueue.h"
#include "ConstantBuffer.h"

#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
