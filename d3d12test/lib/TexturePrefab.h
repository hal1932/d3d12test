#pragma once
#include <DirectXTex.h>
#include <Windows.h>

namespace DirectX
{
	class ScratchImage;
}
namespace fbxsdk
{
	class FbxFileTexture;
}

class TexturePrefab
{
public:
	~TexturePrefab();

	HRESULT LoadFbx(fbxsdk::FbxFileTexture* pFileTexture);

private:
	DirectX::ScratchImage image_;
};

