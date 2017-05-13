#include "MaterialPrefab.h"
#include <fbxsdk.h>

using namespace fbxsdk;

MaterialPrefab::~MaterialPrefab()
{
}

HRESULT MaterialPrefab::LoadFbx(fbxsdk::FbxSurfaceMaterial* pMaterial)
{
	auto result = S_OK;

	// とりあえずランバートだけ
	if (pMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
	{
		type_ = Type::Lambert;
		
		auto pLambert = reinterpret_cast<FbxSurfaceLambert*>(pMaterial);
		
		const auto& diffuse = pLambert->Diffuse.Get();
		diffuse_.x = static_cast<float>(diffuse[0]);
		diffuse_.y = static_cast<float>(diffuse[1]);
		diffuse_.z = static_cast<float>(diffuse[2]);
		diffuse_.w = 1.0f;
	}

	// テクスチャ
	{
		int layerIndex;
		FBXSDK_FOR_EACH_TEXTURE(layerIndex)
		{
			auto prop = pMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[layerIndex]);

			if (prop.GetSrcObjectCount<FbxFileTexture>() > 0)
			{
				// とりあえず 1 枚だけ
				auto pFileTexture = prop.GetSrcObject<FbxFileTexture>(0);

				auto prefabPtr = std::make_unique<TexturePrefab>();
				result = prefabPtr->LoadFbx(pFileTexture);
				if (FAILED(result))
				{
					return result;
				}
				textures_[tstring(prop.GetName())] = std::move(prefabPtr);
			}
		}
	}

	return result;
}
