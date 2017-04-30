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

		Resource* VertexBuffer() { return pVertexBuffer_; }
		int VertexCount() { return vertexCount_; }

		Resource* IndexBuffer() { return pIndexBuffer_; }
		int IndexCount() { return indexCount_; }

		HRESULT UpdateResources(FbxMesh* pMesh, Device* pDevice);
		HRESULT UpdateSubresources(CommandList* pCommandList, CommandQueue* pCommandQueue);

	private:
		Resource* pVertexBuffer_ = nullptr;
		int vertexCount_ = 0;

		Resource* pIndexBuffer_ = nullptr;
		int indexCount_ = 0;

		Material* pMaterial_ = nullptr;

		void UpdateVertexResources_(FbxMesh* pMesh, Device* pDevice);
		void UpdateIndexResources_(FbxMesh* pMesh, Device* pDevice);
	};

}// namespace fbx
