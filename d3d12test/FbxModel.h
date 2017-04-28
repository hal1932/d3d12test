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

class FbxModel
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
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

private:
	static FbxManager* spFbxManager_;

	FbxImporter* pSceneImporter_ = nullptr;
	FbxScene* pScene_ = nullptr;

	const char* pName_ = nullptr;

	Resource* pVertexBuffer_ = nullptr;
	int vertexCount_;

	Resource* pIndexBuffer_ = nullptr;
	int indexCount_;

	HRESULT UpdateResourcesRec_(const FbxNode* pNode, Device* pDevice);
	void UpdateMeshResources_(const FbxMesh* pMesh, Device* pDevice);
	void LoadMeshPolygons_();
	void LoadMeshMaterial_();
	void LoadMeshTexture_();
};

