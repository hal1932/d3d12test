#include "fbxMaterial.h"
#include "common.h"
#include "Device.h"
#include "Texture.h"
#include <iostream>

using namespace fbx;

Material::Material() {}

Material::~Material()
{
	SafeDelete(&pTexture_);
}

HRESULT Material::UpdateResources(FbxGeometry* pGeom, Device* pDevice)
{
	auto pNode = pGeom->GetNode();
	if (!pNode)
	{
		return S_FALSE;
	}

	for (int i = 0; i < pNode->GetMaterialCount(); ++i)
	{
		auto pMaterial = pNode->GetMaterial(i);

		std::cout << pMaterial->GetName() << " " << pMaterial->GetClassId().GetName() << std::endl;

		// �Ƃ肠���������o�[�g�����Ή�
		if (pMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
		{
			auto pLambert = static_cast<FbxSurfaceLambert*>(pMaterial);
			for (int j = 0; j < 3; ++j)
			{
				std::cout << pLambert->Diffuse.Get()[j] << " ";
			}
			std::cout << std::endl;
		}

		// �e�N�X�`��
		for (int j = 0; j < pMaterial->GetSrcObjectCount<FbxSurfaceMaterial>(); ++j)
		{
			auto pSurfaceMaterial = pMaterial->GetSrcObject<FbxSurfaceMaterial>(j);
			std::cout << pSurfaceMaterial->GetName() << std::endl;
		}

		int layerIndex;
		FBXSDK_FOR_EACH_TEXTURE(layerIndex)
		{
			auto prop = pMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[layerIndex]);

			for (int j = 0; j < prop.GetSrcObjectCount<FbxFileTexture>(); ++j)
			{
				auto pTexture = prop.GetSrcObject<FbxFileTexture>(j);
				if (pTexture)
				{
					std::cout << pTexture->GetName() << " " << prop.GetName() << " " << pTexture->GetFileName() << std::endl;

					wchar_t path[256];
					size_t n;
					mbstowcs_s(&n, path, pTexture->GetFileName(), 256);

					pTexture_ = new Texture();
					pTexture_->LoadFromFile(path);
					pTexture_->UpdateResources(pDevice);
				}
			}
		}
	}

	return S_OK;
}

HRESULT Material::UpdateSubresources(CommandList* pCommandList, CommandQueue* pCommandQueue)
{
	return pTexture_->UpdateSubresource(pCommandList, pCommandQueue);
}
