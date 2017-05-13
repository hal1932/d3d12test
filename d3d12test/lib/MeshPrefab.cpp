#include "MeshPrefab.h"
#include <fbxsdk.h>
#include <vector>

MeshPrefab::~MeshPrefab()
{
}

HRESULT MeshPrefab::LoadFbxRec(FbxMesh* pMesh)
{
	auto result = S_OK;

	result = LoadFbxVertices_(pMesh);
	if (FAILED(result))
	{
		return result;
	}

	result = LoadFbxIndices_(pMesh);
	if (FAILED(result))
	{
		return result;
	}

	auto pMeshNode = pMesh->GetNode();
	if (pMeshNode && pMeshNode->GetMaterialCount() > 0)
	{
		// とりあえずマテリアルは 1 個だけ
		materialPtr_ = std::make_unique<MaterialPrefab>();
		materialPtr_->LoadFbx(pMeshNode->GetMaterial(0));
	}

	return result;
}

HRESULT MeshPrefab::LoadFbxVertices_(FbxMesh* pMesh)
{
	vertices_.reset();

	const auto controlPointCount = pMesh->GetControlPointsCount();
	vertices_ = std::unique_ptr<Vertex[]>(new Vertex[controlPointCount]);

	// 位置
	{
		const auto controlPoints = pMesh->GetControlPoints();
		for (auto i = 0; i < controlPointCount; ++i)
		{
			const auto& position = controlPoints[i];
			auto& v = vertices_.get()[i];
			v.Position.x = static_cast<float>(position[0]);
			v.Position.y = static_cast<float>(position[1]);
			v.Position.z = static_cast<float>(position[2]);
		}
	}

	// 法線
	{
		auto tmpNormalArrays = new std::vector<FbxVector4>[controlPointCount];

		for (auto i = 0; i < pMesh->GetPolygonCount(); ++i)
		{
			for (auto j = 0; j < pMesh->GetPolygonSize(i); ++j)
			{
				const auto index = pMesh->GetPolygonVertex(i, j);

				FbxVector4 n;
				if (pMesh->GetPolygonVertexNormal(i, j, n))
				{
					tmpNormalArrays[index].push_back(n);
				}
			}
		}

		// 同じ頂点IDに複数の法線がはいってたら、とりあえず強制ソフトエッジ化
		for (int i = 0; i < controlPointCount; ++i)
		{
			auto& v = vertices_.get()[i];

			if (tmpNormalArrays[i].size() > 1)
			{
				DirectX::XMFLOAT3 n = {};
				for (auto& normal : tmpNormalArrays[i])
				{
					n.x += static_cast<float>(normal[0]);
					n.y += static_cast<float>(normal[1]);
					n.z += static_cast<float>(normal[2]);
				}

				const auto denomi = 1.0f / tmpNormalArrays[i].size();
				n.x *= denomi;
				n.y *= denomi;
				n.z *= denomi;

				XMFLoat3Normalize(&v.Normal, &n);
			}
			else
			{
				const auto& normal = tmpNormalArrays[i][0];
				v.Normal.x = static_cast<float>(normal[0]);
				v.Normal.y = static_cast<float>(normal[1]);
				v.Normal.z = static_cast<float>(normal[2]);
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
				// とりあえず map1 だけ
				FbxVector2 uv;
				bool unmapped;
				if (pMesh->GetPolygonVertexUV(i, j, "map1", uv, unmapped))
				{
					const auto index = pMesh->GetPolygonVertex(i, j);
					auto& tex0 = vertices_.get()[index].Texture0;

					tex0.x = static_cast<float>(uv[0]);
					tex0.y = static_cast<float>(uv[1]);
				}
			}
		}
	}

	return S_OK;
}

HRESULT MeshPrefab::LoadFbxIndices_(FbxMesh* pMesh)
{
	indices_.reset();

	const auto indexCount = pMesh->GetPolygonVertexCount();
	indices_ = std::unique_ptr<ushort[]>(new ushort[indexCount]);

	const auto indices = pMesh->GetPolygonVertices();
	for (auto i = 0; i < indexCount; ++i)
	{
		indices_.get()[i] = static_cast<ushort>(indices[i]);
	}

	return S_OK;
}
