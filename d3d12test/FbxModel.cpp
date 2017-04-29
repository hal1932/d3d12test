#include "FbxModel.h"
#include "common.h"
#include "Device.h"
#include "Resource.h"
#include <iostream>
#include <vector>

namespace
{
	void XMFLoat3Normalize(DirectX::XMFLOAT3* pOut, const DirectX::XMFLOAT3* pIn)
	{
		auto v = DirectX::XMLoadFloat3(pIn);
		v = DirectX::XMVector3Normalize(v);
		DirectX::XMStoreFloat3(pOut, v);
	}
}

FbxManager* FbxModel::spFbxManager_ = nullptr;

void FbxModel::Setup()
{
	FbxModel::Shutdown();

	spFbxManager_ = fbxsdk::FbxManager::Create();

	auto pIOS = FbxIOSettings::Create(spFbxManager_, "IOSROOT");
	spFbxManager_->SetIOSettings(pIOS);

	auto fbxPath = FbxGetApplicationDirectory();
	spFbxManager_->LoadPluginsDirectory(fbxPath.Buffer());
}

void FbxModel::Shutdown()
{
	SafeDestroy(&spFbxManager_);
}

FbxModel::FbxModel()
{}

FbxModel::~FbxModel()
{
	SafeDestroy(&pScene_);

	SafeDelete(&pIndexBuffer_);
	SafeDelete(&pVertexBuffer_);
}


HRESULT FbxModel::LoadFromFile(const char* filepath)
{
	auto pSceneImporter = FbxImporter::Create(spFbxManager_, "");

	auto result = pSceneImporter->Initialize(filepath, -1, spFbxManager_->GetIOSettings());
	if (!result)
	{
		auto error = pSceneImporter->GetStatus().GetErrorString();
		std::cerr << error << std::endl;

		SafeDestroy(&pSceneImporter);
		return S_FALSE;
	}

	SafeDestroy(&pScene_);
	pScene_ = FbxScene::Create(spFbxManager_, "");

	result = pSceneImporter->Import(pScene_);
	if (!result)
	{
		auto error = pSceneImporter->GetStatus().GetErrorString();
		std::cerr << error << std::endl;

		SafeDestroy(&pSceneImporter);
		return S_FALSE;
	}

	SafeDestroy(&pSceneImporter);

	return S_OK;
}

HRESULT FbxModel::UpdateResources(Device* pDevice)
{
	auto pNode = pScene_->GetRootNode();
	return UpdateResourcesRec_(pNode, pDevice);
}

HRESULT FbxModel::UpdateResourcesRec_(FbxNode* pNode, Device* pDevice)
{
	if (!pNode)
	{
		return S_FALSE;
	}

	std::cout << pNode->GetName() << " " << pNode->GetTypeName() << std::endl;

	auto pAttribute = pNode->GetNodeAttribute();
	if (pAttribute)
	{
		pName_ = pAttribute->GetName();

		auto type = pAttribute->GetAttributeType();
		switch (type)
		{
			case FbxNodeAttribute::eMesh:
				UpdateMeshResources_(pNode->GetMesh(), pDevice);
				break;

			default:
				break;
		}
	}

	for (int i = 0; i < pNode->GetChildCount(); ++i)
	{
		UpdateResourcesRec_(pNode->GetChild(i), pDevice);
	}

	return S_OK;
}

void FbxModel::UpdateMeshResources_(FbxMesh* pMesh, Device* pDevice)
{
	{
		SafeDelete(&pVertexBuffer_);
		pVertexBuffer_ = new Resource();

		const auto controlPointCount = pMesh->GetControlPointsCount();
		pVertexBuffer_->CreateVertexBuffer(pDevice, sizeof(Vertex) * controlPointCount);

		{
			const auto pData = static_cast<Vertex*>(pVertexBuffer_->Map(0));

			// 位置
			{
				const auto controlPoints = pMesh->GetControlPoints();

				for (int i = 0; i < controlPointCount; ++i)
				{
					const auto& point = controlPoints[i];
					pData[i].Position.x = static_cast<float>(point.mData[0]);
					pData[i].Position.y = static_cast<float>(point.mData[1]);
					pData[i].Position.z = static_cast<float>(point.mData[2]);
				}
			}

			// GetDirectArray() するとシーン破棄時にアクセス違反で落ちるので
			// 以下 GetPolygonVertex 系の I/F で統一する

			// 法線
			{
				auto tmpNormalArrays = new std::vector<FbxVector4>[controlPointCount];

				for (int i = 0; i < pMesh->GetPolygonCount(); ++i)
				{
					for (int j = 0; j < pMesh->GetPolygonSize(i); ++j)
					{
						const auto dataIndex = pMesh->GetPolygonVertex(i, j);

						FbxVector4 normal;
						if (pMesh->GetPolygonVertexNormal(i, j, normal))
						{
							tmpNormalArrays[dataIndex].push_back(normal);
						}
					}
				}

				// 同じ頂点IDに複数の法線がはいってたら、とりあえず強制ソフトエッジ化
				for (int i = 0; i < controlPointCount; ++i)
				{
					if (tmpNormalArrays[i].size() > 1)
					{
						DirectX::XMFLOAT3 n = {};
						for (auto& normal : tmpNormalArrays[i])
						{
							n.x += static_cast<float>(normal.mData[0]);
							n.y += static_cast<float>(normal.mData[1]);
							n.z += static_cast<float>(normal.mData[2]);
						}

						const auto denomi = 1.0f / tmpNormalArrays[i].size();
						n.x *= denomi;
						n.y *= denomi;
						n.z *= denomi;

						XMFLoat3Normalize(&pData[i].Normal, &n);
					}
					else
					{
						const auto& normal = tmpNormalArrays[i][0];
						pData[i].Normal.x = static_cast<float>(normal[0]);
						pData[i].Normal.y = static_cast<float>(normal[1]);
						pData[i].Normal.z = static_cast<float>(normal[2]);
					}
				}

				SafeDeleteArray(&tmpNormalArrays);
			}

			// UV
			{
				for (int i = 0; i < pMesh->GetPolygonCount(); ++i)
				{
					for (int j = 0; j < pMesh->GetPolygonSize(i); ++j)
					{
						FbxVector2 uv;
						bool unmapped;
						if (pMesh->GetPolygonVertexUV(i, j, "map1", uv, unmapped))
						{
							const auto dataIndex = pMesh->GetPolygonVertex(i, j);
							auto& tex0 = pData[dataIndex].Texture0;

							tex0.x = static_cast<float>(uv[0]);
							tex0.y = static_cast<float>(uv[1]);
						}
					}
				}
			}

			pVertexBuffer_->Unmap(0);
		}

		vertexCount_ = controlPointCount;
	}

	{
		SafeDelete(&pIndexBuffer_);
		pIndexBuffer_ = new Resource();

		const auto indexCount = pMesh->GetPolygonVertexCount();
		pIndexBuffer_->CreateIndexBuffer(pDevice, sizeof(ushort) * indexCount);

		{
			const auto indices = pMesh->GetPolygonVertices();
			const auto pData = static_cast<ushort*>(pIndexBuffer_->Map(0));

			for (int i = 0; i < indexCount; ++i)
			{
				pData[i] = static_cast<ushort>(indices[i]);
			}

			pIndexBuffer_->Unmap(0);
		}

		indexCount_ = indexCount;
	}

	UpdateMaterialResources_(pMesh, pDevice);
}

void FbxModel::UpdateMaterialResources_(FbxGeometry* pGeom, Device* pDevice)
{
	auto pNode = pGeom->GetNode();
	if (!pNode)
	{
		return;
	}

	for (int i = 0; i < pNode->GetMaterialCount(); ++i)
	{
		auto pMaterial = pNode->GetMaterial(i);

		std::cout << pMaterial->GetName() << " " << pMaterial->GetClassId().GetName() << std::endl;

		// とりあえずランバートだけ対応
		if (pMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
		{
			auto pLambert = static_cast<FbxSurfaceLambert*>(pMaterial);
			for (int j = 0; j < 3; ++j)
			{
				std::cout << pLambert->Diffuse.Get()[j] << " ";
			}
			std::cout << std::endl;
		}

		// テクスチャ
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
				}
			}
		}
	}
}
