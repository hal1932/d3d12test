#pragma once
#include "common.h"
#include "MaterialPrefab.h"
#include <DirectXMath.h>
#include <Windows.h>
#include <memory>

namespace fbxsdk
{
	class FbxMesh;
}

class MeshPrefab
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 Texture0;
	};

public:
	~MeshPrefab();

	HRESULT LoadFbxRec(fbxsdk::FbxMesh* pMesh);

private:
	std::unique_ptr<Vertex[]> vertices_;
	std::unique_ptr<ushort[]> indices_;
	std::unique_ptr<MaterialPrefab> materialPtr_;

	HRESULT LoadFbxVertices_(fbxsdk::FbxMesh* pMesh);
	HRESULT LoadFbxIndices_(fbxsdk::FbxMesh* pMesh);
};

