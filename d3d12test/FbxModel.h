#pragma once
#include <fbxsdk.h>
#include <Windows.h>
#include <DirectXMath.h>

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

class FbxModel
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 Texture0;
	};

public:
	FbxModel();
	~FbxModel();

	static void Setup();
	static void Shutdown();

	Resource* VertexBuffer() { return pVertexBuffer_; }
	int VertexCount() { return vertexCount_; }

	Resource* IndexBuffer() { return pIndexBuffer_; }
	int IndexCount() { return indexCount_; }

	HRESULT LoadFromFile(const char* filepath);
	HRESULT UpdateResources(Device* pDevice);
	HRESULT UpdateSubresources(CommandList* pCommandList, CommandQueue* pCommandQueue);

private:
	static FbxManager* spFbxManager_;

	FbxScene* pScene_ = nullptr;

	const char* pName_ = nullptr;

	Resource* pVertexBuffer_ = nullptr;
	int vertexCount_ = 0;

	Resource* pIndexBuffer_ = nullptr;
	int indexCount_ = 0;

	Texture* pTexture_ = nullptr;

	HRESULT UpdateResourcesRec_(FbxNode* pNode, Device* pDevice);
	void UpdateMeshResources_(FbxMesh* pMesh, Device* pDevice);
	void UpdateMaterialResources_(FbxGeometry* pMesh, Device* pDevice);
};

