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
	SafeDelete(&pIndexBuffer_);
	SafeDelete(&pVertexBuffer_);
	SafeDestroy(&pScene_);
	SafeDestroy(&pSceneImporter_);
}

HRESULT FbxModel::LoadFromFile(const char* filepath)
{
	SafeDestroy(&pSceneImporter_);
	pSceneImporter_ = FbxImporter::Create(spFbxManager_, "");

	auto result = pSceneImporter_->Initialize(filepath, -1, spFbxManager_->GetIOSettings());
	if (!result)
	{
		auto error = pSceneImporter_->GetStatus().GetErrorString();
		std::cerr << error << std::endl;
		return S_FALSE;
	}

	SafeDestroy(&pScene_);
	pScene_ = FbxScene::Create(spFbxManager_, filepath);

	result = pSceneImporter_->Import(pScene_);
	if (!result)
	{
		auto error = pSceneImporter_->GetStatus().GetErrorString();
		std::cerr << error << std::endl;
		return S_FALSE;
	}

	return S_OK;
}

HRESULT FbxModel::UpdateResources(Device* pDevice)
{
	auto pNode = pScene_->GetRootNode();
	return UpdateResourcesRec_(pNode, pDevice);
}

HRESULT FbxModel::UpdateResourcesRec_(const FbxNode* pNode, Device* pDevice)
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
				UpdateMeshResources_(static_cast<const FbxMesh*>(pAttribute), pDevice);
				break;
		}
	}

	for (int i = 0; i < pNode->GetChildCount(); ++i)
	{
		UpdateResourcesRec_(pNode->GetChild(i), pDevice);
	}

	return S_OK;
}

void FbxModel::UpdateMeshResources_(const FbxMesh* pMesh, Device* pDevice)
{
	const auto polyCount = pMesh->GetPolygonCount();

	SafeDelete(&pVertexBuffer_);
	pVertexBuffer_ = new Resource();

	const auto controlPointCount = pMesh->GetControlPointsCount();
	pVertexBuffer_->CreateVertexBuffer(pDevice, sizeof(Vertex) * controlPointCount);

	{
		const auto pData = static_cast<Vertex*>(pVertexBuffer_->Map(0));

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
		{
			const auto pElementNormal = pMesh->GetElementNormal();

			switch (pElementNormal->GetMappingMode())
			{

#if false
				// 手許のデータでテストができないのでとりあえず無効化しておく
				case FbxGeometryElement::eByControlPoint:
				{
					const auto normals = pElementNormal->GetDirectArray();

					switch (pElementNormal->GetReferenceMode())
					{
						case FbxGeometryElement::eDirect:
							for (int i = 0; i < controlPointCount; ++i)
							{
								const auto& normal = normals.GetAt(i);
								for (int j = 0; j < 3; ++j)
								{
									pData[i].Normal[j] = static_cast<float>(normal.mData[j]);
								}
							}
							break;

						case FbxGeometryElement::eIndexToDirect:
							for (int i = 0; i < controlPointCount; ++i)
							{
								const auto index = pElementNormal->GetIndexArray().GetAt(i);
								const auto& normal = normals.GetAt(index);
								for (int j = 0; j < 3; ++j)
								{
									pData[i].Normal[j] = static_cast<float>(normal.mData[j]);
								}
							}
							break;

						default:
							break;
					}
					break;
				}
#endif

				case FbxGeometryElement::eByPolygonVertex:
				{
					std::vector<FbxVector4>* tmpNormalArrays = nullptr;

					switch (pElementNormal->GetReferenceMode())
					{
						case FbxGeometryElement::eDirect:
							tmpNormalArrays = new std::vector<FbxVector4>[controlPointCount];

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

							// とりあえず強制ソフトエッジ化
							for (int i = 0; i < controlPointCount; ++i)
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

							break;

#if false
						case FbxGeometryElement::eIndexToDirect:
							// 手許のデータでテストができないのでとりあえず無効化しておく
							break;
#endif

						default:
							break;
					}

					SafeDeleteArray(&tmpNormalArrays);

					break;
				}

				default:
					break;
			}
		}

		pVertexBuffer_->Unmap(0);
	}

	vertexCount_ = controlPointCount;

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
