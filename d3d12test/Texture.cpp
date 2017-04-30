#include "Texture.h"
#include "common.h"
#include "Device.h"
#include "Resource.h"
#include <DirectXTex.h>

#pragma comment(lib, "DirectXTex.lib")

Texture::Texture()
	: pData_(nullptr),
	pResource_(nullptr)
{}

Texture::~Texture()
{
	SafeDelete(&pResource_);
	SafeDelete(&pData_);
}

HRESULT Texture::LoadFromFile(const wchar_t* filepath)
{
	HRESULT result;

	SafeDelete(&pData_);
	pData_ = new DirectX::TexMetadata();
	result = DirectX::GetMetadataFromDDSFile(filepath, DirectX::DDS_FLAGS_NONE, *pData_);

	return result;
}

HRESULT Texture::UpdateResources(Device* pDevice)
{
	HRESULT result;

	SafeDelete(&pResource_);

	ID3D12Resource* pNativeResource;
	result = DirectX::CreateTexture(pDevice->NativePtr(), *pData_, &pNativeResource);
	if (FAILED(result))
	{
		return result;
	}

	pResource_ = new Resource(pNativeResource);

	return result;
}
