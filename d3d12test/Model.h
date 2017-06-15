#pragma once
#include "lib\lib.h"
#include <memory>

__declspec(align(256))
struct ModelTransform
{
	DirectX::XMMATRIX World;
	DirectX::XMMATRIX View;
	DirectX::XMMATRIX Proj;
};

class Model
{
public:
	Model() {}

	~Model()
	{
		modelPtr_.reset();
		transformCbvPtr_->Unmap(0);
	}

	int BufferCount() { return 2; }
	Transform* TransformPtr() { return modelPtr_->TransformPtr(); }
	const fbx::Model& FbxModel() { return *modelPtr_; }

	void Setup(Device* pDevice, const char* filepath)
	{
		modelPtr_ = std::make_unique<fbx::Model>();
		modelPtr_->LoadFromFile(filepath);
		modelPtr_->UpdateResources(pDevice);
	}

	void SetupAsReference(Model* pModel)
	{
		modelPtr_ = std::unique_ptr<fbx::Model>(pModel->modelPtr_->CreateReference());
	}
	
	void SetupBuffers(ResourceViewHeap* pHeap)
	{
		const CsvDesc csvDesc = { sizeof(ModelTransform), D3D12_TEXTURE_LAYOUT_ROW_MAJOR };
		transformCbvPtr_ = std::unique_ptr<Resource>(pHeap->CreateConstantBufferView(csvDesc));

		transformPtr_ = transformCbvPtr_->Map(0);
		
		const SrvDesc srvDesc = {
			D3D12_SRV_DIMENSION_TEXTURE2D,
			{ modelPtr_->MeshPtr(0)->MaterialPtr()->TexturePtr() } };
		pTextureSrv_ = pHeap->CreateShaderResourceView(srvDesc);
	}

	void UpdateSubresources(CommandList* pCmdList, CommandQueue* pCmdQueue)
	{
		modelPtr_->UpdateSubresources(pCmdList, pCmdQueue);
	}

	void SetShaderHash(ulonglong hash)
	{
		shaderHash_ = hash;
	}

	void SetTransform(const ModelTransform& transform)
	{
		memcpy(transformPtr_, &transform, sizeof(ModelTransform));
	}

	void SetRootDescriptorTable(ID3D12GraphicsCommandList* pNativeCmdList)
	{
		pNativeCmdList->SetGraphicsRootDescriptorTable(0, transformCbvPtr_->GpuDescriptorHandle());
		pNativeCmdList->SetGraphicsRootDescriptorTable(1, pTextureSrv_->GpuDescriptorHandle());
	}

	void Draw(ID3D12GraphicsCommandList* pNativeCmdList)
	{
		for (auto i = 0; i < modelPtr_->MeshCount(); ++i)
		{
			const auto pMesh = modelPtr_->MeshPtr(i);

			auto vbView = pMesh->VertexBuffer()->GetVertexBufferView(sizeof(fbx::Mesh::Vertex));
			pNativeCmdList->IASetVertexBuffers(0, 1, &vbView);

			auto ibView = pMesh->IndexBuffer()->GetIndexBufferView(DXGI_FORMAT_R16_UINT);
			pNativeCmdList->IASetIndexBuffer(&ibView);

			pNativeCmdList->DrawIndexedInstanced(pMesh->IndexCount(), 1, 0, 0, 0);
		}
	}

private:
	std::unique_ptr<fbx::Model> modelPtr_;

	std::unique_ptr<Resource> transformCbvPtr_;
	void* transformPtr_;
	Resource* pTextureSrv_;

	ulonglong shaderHash_;
};

