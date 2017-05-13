#pragma once
#include "lib/lib.h"
#include <vector>
#include <memory>

class Graphics;

class ModelManager
{
public:
	~ModelManager();

	//int AddFromFile(const char* filepath);
	//HRESULT UpdateResources(Graphics* pGraphics, int startIndex = -1, int count = 0);
	//HRESULT UpdateSubresources(Graphics* pGraphics, int startIndex = -1, int count = 0);

private:
	std::vector<std::unique_ptr<fbx::Model>> modelPtrs_;
};

