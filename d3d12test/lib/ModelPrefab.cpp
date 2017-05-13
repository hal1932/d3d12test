#include "ModelPrefab.h"
#include "common.h"
#include "Model.h"
#include "MeshPrefab.h"
#include <fbxsdk.h>
#include <iostream>

using namespace fbxsdk;

FbxManager* ModelPrefab::spFbxManager_ = nullptr;

void ModelPrefab::Setup()
{
	ModelPrefab::Shutdown();

	spFbxManager_ = fbxsdk::FbxManager::Create();

	auto pIOS = FbxIOSettings::Create(spFbxManager_, "IOSROOT");
	spFbxManager_->SetIOSettings(pIOS);

	auto fbxPath = FbxGetApplicationDirectory();
	spFbxManager_->LoadPluginsDirectory(fbxPath.Buffer());
}

void ModelPrefab::Shutdown()
{
	SafeDestroy(&spFbxManager_);
}

ModelPrefab::~ModelPrefab()
{
}

HRESULT ModelPrefab::LoadFromFbxFile(const char* filepath)
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

	auto pScene = FbxScene::Create(spFbxManager_, "");

	result = pSceneImporter->Import(pScene);
	if (!result)
	{
		auto error = pSceneImporter->GetStatus().GetErrorString();
		std::cerr << error << std::endl;

		SafeDestroy(&pScene);
		SafeDestroy(&pSceneImporter);
		return S_FALSE;
	}

	auto pNode = pScene->GetRootNode();
	name_ = tstring(pNode->GetName());

	auto loadResult = LoadFbxRec_(pNode);
	if (FAILED(loadResult))
	{
		SafeDestroy(&pScene);
		SafeDestroy(&pSceneImporter);
		return loadResult;
	}

	SafeDestroy(&pScene);
	SafeDestroy(&pSceneImporter);

	return loadResult;
}

Model* ModelPrefab::Instantiate(const char* name)
{
	return nullptr;
}

HRESULT ModelPrefab::LoadFbxRec_(FbxNode* pNode)
{
	if (!pNode)
	{
		return S_FALSE;
	}

	auto result = S_OK;

	auto pAttribute = pNode->GetNodeAttribute();
	if (pAttribute)
	{
		auto type = pAttribute->GetAttributeType();
		switch (type)
		{
			case FbxNodeAttribute::eMesh:
			{
				auto meshPtr = std::make_unique<MeshPrefab>();
				result = meshPtr->LoadFbxRec(pNode->GetMesh());
				if (FAILED(result))
				{
					return result;
				}
				meshPrefabPtrs_.push_back(std::move(meshPtr));
				break;
			}

			default:
				break;
		}
	}

	for (auto i = 0; i < pNode->GetChildCount(); ++i)
	{
		result = LoadFbxRec_(pNode->GetChild(i));
		if (FAILED(result))
		{
			return result;
		}
	}

	return result;
}
