#pragma once
#include "common.h"
#include "TexturePrefab.h"
#include <DirectXMath.h>
#include <memory>
#include <map>

namespace fbxsdk
{
	class FbxSurfaceMaterial;
}

class MaterialPrefab
{
public:
	enum class Type
	{
		Lambert,
	};

public:
	~MaterialPrefab();

	HRESULT LoadFbx(fbxsdk::FbxSurfaceMaterial* pMaterial);

private:
	Type type_;
	DirectX::XMFLOAT4 diffuse_;
	std::map<tstring, std::unique_ptr<TexturePrefab>> textures_;
};

