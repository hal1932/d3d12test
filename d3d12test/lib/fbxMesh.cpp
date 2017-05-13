#include "fbxMesh.h"
#include "common.h"
#include "Device.h"
#include "Resource.h"
#include "fbxMaterial.h"
#include <vector>

using namespace fbx;

Mesh::Mesh() {}

Mesh::~Mesh()
{
	SafeDelete(&pMaterial_);
	SafeDelete(&pIndexBuffer_);
	SafeDelete(&pVertexBuffer_);
}

HRESULT Mesh::UpdateResources(FbxMesh* pMesh, Device* pDevice)
{
	UpdateVertexResources_(pMesh, pDevice);
	UpdateIndexResources_(pMesh, pDevice);

	SafeDelete(&pMaterial_);
	pMaterial_ = new Material();

	return pMaterial_->UpdateResources(pMesh, pDevice);
}

HRESULT Mesh::UpdateSubresources(CommandList* pCommandList, CommandQueue* pCommandQueue)
{
	return pMaterial_->UpdateSubresources(pCommandList, pCommandQueue);
}

void Mesh::UpdateVertexResources_(FbxMesh* pMesh, Device* pDevice)
{
	SafeDelete(&pVertexBuffer_);
	pVertexBuffer_ = new Resource();

	const auto controlPointCount = pMesh->GetControlPointsCount();
	pVertexBuffer_->CreateVertexBuffer(pDevice, sizeof(Vertex) * controlPointCount);

	{
		const auto pData = static_cast<Vertex*>(pVertexBuffer_->Map(0));

		// �ʒu
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

		// GetDirectArray() ����ƃV�[���j�����ɃA�N�Z�X�ᔽ�ŗ�����̂�
		// �ȉ� GetPolygonVertex �n�� I/F �œ��ꂷ��

		// �@��
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

			// �������_ID�ɕ����̖@�����͂����Ă���A�Ƃ肠���������\�t�g�G�b�W��
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

void Mesh::UpdateIndexResources_(FbxMesh* pMesh, Device* pDevice)
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
