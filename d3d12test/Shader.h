#pragma once
#include <d3d12.h>
#include <vector>
#include <string>

struct ShaderDesc
{
	LPCWSTR FilePath = nullptr;
	LPCTSTR EntryPoint;
	LPCTSTR Profile;

	ShaderDesc(LPCWSTR filePath, LPCTSTR entryPoint, LPCTSTR profile)
		: FilePath(filePath), EntryPoint(entryPoint), Profile(profile)
	{}
};

struct CompiledShaderDesc
{
	LPCWSTR FilePath = nullptr;

	CompiledShaderDesc(LPCWSTR filePath)
		: FilePath(filePath)
	{}
};

class Shader
{
public:
	Shader();
	~Shader();

	ID3DBlob* NativePtr() { return pBlob_; }

	D3D12_INPUT_LAYOUT_DESC NativeInputLayout() { return pInputLayout_->NativeObj(); }

	D3D12_SHADER_BYTECODE NativeByteCode()
	{
		return{ pBlob_->GetBufferPointer(), pBlob_->GetBufferSize() };
	}

	HRESULT CreateFromSourceFile(const ShaderDesc& desc);
	HRESULT CreateFromCompiledFile(const CompiledShaderDesc& desc);

	HRESULT CreateInputLayout();

private:
	ID3DBlob* pBlob_;

	class InputLayout
	{
	public:
		InputLayout();
		~InputLayout();

		D3D12_INPUT_LAYOUT_DESC NativeObj() { return{ pElements_, elementCount_ }; }
		HRESULT Create(ID3DBlob *pBlob);

	private:
		D3D12_INPUT_ELEMENT_DESC* pElements_;
		UINT elementCount_;
		std::string* pSemanticNames_;
	};
	InputLayout* pInputLayout_;
};

