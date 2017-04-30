#pragma once
#include "Windows.h"

namespace DirectX
{
	struct TexMetadata;
}

class Device;
class Resource;

class Texture
{
public:
	Texture();
	~Texture();

	Resource* ResourcePtr() { return pResource_; }

	HRESULT LoadFromFile(const wchar_t* filepath);
	HRESULT UpdateResources(Device* pDevice);

private:
	DirectX::TexMetadata* pData_;
	Resource* pResource_;
};

