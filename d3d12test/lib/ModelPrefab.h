#pragma once
#include "common.h"
#include <Windows.h>
#include <vector>
#include <memory>

#ifdef _DEBUG
#	pragma comment(lib, "libfbxsdk-md.lib")
#else
#	pragma comment(lib, "libfbxsdk-mt.lib")
#endif

class Model;
class MeshPrefab;

namespace fbxsdk
{
	class FbxManager;
	class FbxNode;
}

class ModelPrefab
{
public:
	static void Setup();
	static void Shutdown();

public:
	~ModelPrefab();

	HRESULT LoadFromFbxFile(const char* filepath);

	Model* Instantiate(const char* name = nullptr);

private:
	static fbxsdk::FbxManager* spFbxManager_;

	tstring name_;
	std::vector<std::unique_ptr<MeshPrefab>> meshPrefabPtrs_;

	HRESULT LoadFbxRec_(fbxsdk::FbxNode* pNode);
};

