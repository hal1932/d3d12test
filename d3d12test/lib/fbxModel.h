#pragma once
#include "common.h"
#include "Transform.h"
#include <fbxsdk.h>
#include <Windows.h>
#include <DirectXMath.h>
#include <vector>

#ifdef _DEBUG
#	pragma comment(lib, "libfbxsdk-md.lib")
#else
#	pragma comment(lib, "libfbxsdk-mt.lib")
#endif

class Device;
class Resource;
class CommandList;
class CommandQueue;
class Texture;

namespace fbx
{
	class Mesh;

	class Model
	{
	public:
		Model();
		~Model();

		static void Setup();
		static void Shutdown();

		Transform* TransformPtr() { return &transform_; }

		int MeshCount() { return meshPtrs_.size(); }
		Mesh* MeshPtr(int index) { return meshPtrs_[index]; }

		HRESULT LoadFromFile(const char* filepath);
		HRESULT UpdateResources(Device* pDevice);
		HRESULT UpdateSubresources(CommandList* pCommandList, CommandQueue* pCommandQueue);

		Model* CreateReference();

	private:
		static FbxManager* spFbxManager_;

		bool isReference_ = false;

		tstring name_;
		FbxScene* pScene_ = nullptr;
		std::vector<Mesh*> meshPtrs_;

		Transform transform_;

		HRESULT UpdateResourcesRec_(FbxNode* pNode, Device* pDevice);
		void UpdateMaterialResources_(FbxGeometry* pMesh, Device* pDevice);
	};

}// namespace fbx
