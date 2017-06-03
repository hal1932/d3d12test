#include "fbxModel.h"
#include "common.h"
#include "Device.h"
#include "Resource.h"
#include "Texture.h"
#include "fbxMesh.h"
#include <iostream>
#include <vector>

using namespace fbx;

FbxManager* Model::spFbxManager_ = nullptr;

void Model::Setup()
{
	Model::Shutdown();

	spFbxManager_ = fbxsdk::FbxManager::Create();

	auto pIOS = FbxIOSettings::Create(spFbxManager_, "IOSROOT");
	spFbxManager_->SetIOSettings(pIOS);

	auto fbxPath = FbxGetApplicationDirectory();
	spFbxManager_->LoadPluginsDirectory(fbxPath.Buffer());
}

void Model::Shutdown()
{
	SafeDestroy(&spFbxManager_);
}

Model::Model() {}

Model::~Model()
{
	SafeDeleteSequence(&meshPtrs_);
	SafeDestroy(&pScene_);
}


HRESULT Model::LoadFromFile(const char* filepath)
{
	auto pSceneImporter = FbxImporter::Create(spFbxManager_, "");

	auto result = pSceneImporter->Initialize(filepath, -1, spFbxManager_->GetIOSettings());
	if (!result)
	{
		auto error = pSceneImporter->GetStatus().GetErrorString();
		std::cerr << error << std::endl;

		SafeDestroy(&pSceneImporter);
		return S_FALSE;
	}

	SafeDestroy(&pScene_);
	pScene_ = FbxScene::Create(spFbxManager_, "");

	result = pSceneImporter->Import(pScene_);
	if (!result)
	{
		auto error = pSceneImporter->GetStatus().GetErrorString();
		std::cerr << error << std::endl;

		SafeDestroy(&pSceneImporter);
		return S_FALSE;
	}

	SafeDestroy(&pSceneImporter);

	return S_OK;
}

HRESULT Model::UpdateResources(Device* pDevice)
{
	SafeDeleteSequence(&meshPtrs_);

	auto pNode = pScene_->GetRootNode();
	return UpdateResourcesRec_(pNode, pDevice);
}

HRESULT Model::UpdateSubresources(CommandList* pCommandList, CommandQueue* pCommandQueue)
{
	HRESULT result;

	for (auto pMesh : meshPtrs_)
	{
		result |= pMesh->UpdateSubresources(pCommandList, pCommandQueue);
	}

	return result;
}

HRESULT Model::UpdateResourcesRec_(FbxNode* pNode, Device* pDevice)
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
			{
				auto pMesh = new Mesh();
				pMesh->UpdateResources(pNode->GetMesh(), pDevice);
				meshPtrs_.push_back(pMesh);
				break;
			}

			default:
				break;
		}
	}

	for (int i = 0; i < pNode->GetChildCount(); ++i)
	{
		UpdateResourcesRec_(pNode->GetChild(i), pDevice);
	}

	return S_OK;
}
