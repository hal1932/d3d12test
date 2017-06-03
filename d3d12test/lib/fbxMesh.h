#pragma once
#include "fbxsdk.h"
#include <DirectXMath.h>
#include <Windows.h>

class Device;
class Resource;
class CommandList;
class CommandQueue;

namespace fbx
{
	class Material;

	class Mesh
	{
	public:
		struct Vertex
		{
			DirectX::XMFLOAT3 Position;
			DirectX::XMFLOAT3 Normal;
			DirectX::XMFLOAT2 Texture0;
		};

	public:
		Mesh();
		~Mesh();

		Material* MaterialPtr() { return pMaterial_; }

		Resource* VertexBuffer() { return pVertexBuffer_; }
		int VertexCount() { return *pVertexCount_; }

		Resource* IndexBuffer() { return pIndexBuffer_; }
		int IndexCount() { return *pIndexCount_; }

		HRESULT UpdateResources(FbxMesh* pMesh, Device* pDevice);
		HRESULT UpdateSubresources(CommandList* pCommandList, CommandQueue* pCommandQueue);

		Mesh* CreateReference();

	private:
		bool isReference_ = false;

		Resource* pVertexBuffer_ = nullptr;
		int* pVertexCount_ = nullptr;

		Resource* pIndexBuffer_ = nullptr;
		int* pIndexCount_ = nullptr;

		Material* pMaterial_ = nullptr;

		void Setup_();
		void UpdateVertexResources_(FbxMesh* pMesh, Device* pDevice);
		void UpdateIndexResources_(FbxMesh* pMesh, Device* pDevice);
	};

}// namespace fbx
