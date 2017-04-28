#include "FbxModel.h"
#include "common.h"
#include "Device.h"
#include "Resource.h"
#include <iostream>

FbxManager* FbxModel::spFbxManager_ = nullptr;

void FbxModel::Setup()
{
	FbxModel::Shutdown();

	spFbxManager_ = fbxsdk::FbxManager::Create();

	auto pIOS = FbxIOSettings::Create(spFbxManager_, "IOSROOT");
	spFbxManager_->SetIOSettings(pIOS);

	auto fbxPath = FbxGetApplicationDirectory();
	spFbxManager_->LoadPluginsDirectory(fbxPath.Buffer());
}

void FbxModel::Shutdown()
{
	SafeDestroy(&spFbxManager_);
}

FbxModel::FbxModel()
{}

FbxModel::~FbxModel()
{
	SafeDelete(&pIndexBuffer_);
	SafeDelete(&pVertexBuffer_);
	SafeDestroy(&pScene_);
	SafeDestroy(&pSceneImporter_);
}

HRESULT FbxModel::LoadFromFile(const char* filepath)
{
	SafeDestroy(&pSceneImporter_);
	pSceneImporter_ = FbxImporter::Create(spFbxManager_, "");

	auto result = pSceneImporter_->Initialize(filepath, -1, spFbxManager_->GetIOSettings());
	if (!result)
	{
		auto error = pSceneImporter_->GetStatus().GetErrorString();
		std::cerr << error << std::endl;
		return S_FALSE;
	}

	SafeDestroy(&pScene_);
	pScene_ = FbxScene::Create(spFbxManager_, filepath);

	result = pSceneImporter_->Import(pScene_);
	if (!result)
	{
		auto error = pSceneImporter_->GetStatus().GetErrorString();
		std::cerr << error << std::endl;
		return S_FALSE;
	}

	return S_OK;
}

HRESULT FbxModel::UpdateResources(Device* pDevice)
{
	auto pNode = pScene_->GetRootNode();
	return UpdateResourcesRec_(pNode, pDevice);
}

HRESULT FbxModel::UpdateResourcesRec_(const FbxNode* pNode, Device* pDevice)
{
	if (!pNode)
	{
		return S_FALSE;
	}

	std::cout << pNode->GetName() << " " << pNode->GetTypeName() << std::endl;

	auto pAttribute = pNode->GetNodeAttribute();
	if (pAttribute)
	{
		pName_ = pAttribute->GetName();

		auto type = pAttribute->GetAttributeType();
		switch (type)
		{
			case FbxNodeAttribute::eMesh:
				UpdateMeshResources_(static_cast<const FbxMesh*>(pAttribute), pDevice);
				break;
		}
	}

	for (int i = 0; i < pNode->GetChildCount(); ++i)
	{
		UpdateResourcesRec_(pNode->GetChild(i), pDevice);
	}

	return S_OK;
}

void FbxModel::UpdateMeshResources_(const FbxMesh* pMesh, Device* pDevice)
{
	const auto polyCount = pMesh->GetPolygonCount();

	const auto controlPoints = pMesh->GetControlPoints();
	const auto controlPointCount = pMesh->GetControlPointsCount();

	SafeDelete(&pVertexBuffer_);
	pVertexBuffer_ = new Resource();
	pVertexBuffer_->CreateVertexBuffer(pDevice, sizeof(Vertex) * controlPointCount);
	{
		const auto pData = static_cast<Vertex*>(pVertexBuffer_->Map(0));

		for (int i = 0; i < controlPointCount; ++i)
		{
			const auto& point = controlPoints[i];
			for (int j = 0; j < 3; ++j)
			{
				pData[i].Position[j] = static_cast<float>(point.mData[j]);
			}
		}

		pVertexBuffer_->Unmap(0);
	}

	vertexCount_ = controlPointCount;

	const auto indices = pMesh->GetPolygonVertices();
	const auto indexCount = pMesh->GetPolygonVertexCount();

	SafeDelete(&pIndexBuffer_);
	pIndexBuffer_ = new Resource();
	pIndexBuffer_->CreateIndexBuffer(pDevice, sizeof(ushort) * indexCount);

	{
		const auto pData = static_cast<ushort*>(pIndexBuffer_->Map(0));

		for (int i = 0; i < indexCount; ++i)
		{
			pData[i] = static_cast<ushort>(indices[i]);
		}

		pIndexBuffer_->Unmap(0);
	}

	indexCount_ = indexCount;
}
