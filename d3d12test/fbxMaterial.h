#pragma once
#include "fbxsdk.h"
#include <Windows.h>

class Device;
class Texture;
class CommandList;
class CommandQueue;

namespace fbx
{
	class Material
	{
	public:
		Material();
		~Material();

		Texture* TexturePtr() { return pTexture_; }

		HRESULT UpdateResources(FbxGeometry* pGeom, Device* pDevice);
		HRESULT UpdateSubresources(CommandList* pCommandList, CommandQueue* pCommandQueue);

	private:
		Texture* pTexture_ = nullptr;
	};

}// namespace fbx
