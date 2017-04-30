#pragma once
#include "Windows.h"
#include <string>

namespace DirectX
{
	struct TexMetadata;
}

class Device;
class Resource;
class CommandList;
class CommandQueue;

class Texture
{
public:
	Texture();
	~Texture();

	Resource* ResourcePtr() { return pResource_; }

	HRESULT LoadFromFile(const std::wstring& filepath);
	HRESULT UpdateResources(Device* pDevice);
	HRESULT UpdateSubresource(CommandList* pCommandList, CommandQueue* pCommandQueue);

private:
	DirectX::TexMetadata* pData_;
	std::wstring filepath_;
	Resource* pResource_;
};

