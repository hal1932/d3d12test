#include "TexturePrefab.h"
#include <fbxsdk.h>

using namespace fbxsdk;

TexturePrefab::~TexturePrefab()
{
	image_.Release();
}

HRESULT TexturePrefab::LoadFbx(FbxFileTexture* pFileTexture)
{
	auto result = S_OK;

	wchar_t filepath[256];
	size_t n;
	mbstowcs_s(&n, filepath, pFileTexture->GetFileName(), 256);

	DirectX::TexMetadata metadata;
	result = DirectX::GetMetadataFromDDSFile(filepath, DirectX::DDS_FLAGS_NONE, metadata);
	if (FAILED(result))
	{
		return result;
	}

	result = DirectX::LoadFromDDSFile(filepath, DirectX::DDS_FLAGS_NONE, &metadata, image_);
	if (FAILED(result))
	{
		return result;
	}

	return result;
}
