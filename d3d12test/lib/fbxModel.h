#pragma once
#include "common.h"
#include "Transform.h"
#include <fbxsdk.h>
#include <Windows.h>
#include <DirectXMath.h>
#include <vector>

#pragma comment(lib, "libfbxsdk-md.lib")

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

		int MeshCount() { return static_cast<int>(meshPtrs_.size()); }
		int MeshCount() const { return static_cast<int>(meshPtrs_.size()); }

		const ulonglong ShaderHash() { return shaderHash_; }
		const ulonglong ShaderHash() const { return shaderHash_; }

		Mesh* MeshPtr(int index) { return meshPtrs_[index]; }
		const Mesh* MeshPtr(int index) const { return meshPtrs_[index]; }

		HRESULT LoadFromFile(const char* filepath);
		HRESULT UpdateResources(Device* pDevice);
		HRESULT UpdateSubresources(CommandList* pCommandList, CommandQueue* pCommandQueue);

		void SetShaderHash(ulonglong hash) { shaderHash_ = hash; }

		Model* CreateReference();

	private:
		static FbxManager* spFbxManager_;

		bool isReference_ = false;

		tstring name_;
		FbxScene* pScene_ = nullptr;
		std::vector<Mesh*> meshPtrs_;

		Transform transform_;

		ulonglong shaderHash_;

		HRESULT UpdateResourcesRec_(FbxNode* pNode, Device* pDevice);
		void UpdateMaterialResources_(FbxGeometry* pMesh, Device* pDevice);
	};

}// namespace fbx
