#pragma once

#include<DirectXMath.h>
#include<string>
#include<iostream>
#include"Macro.h"
#include<d3d12.h>
#include<cmath>

using namespace std;
using namespace DirectX;

/******************************************************数学相关工具**********************************************************************/
inline void operator += (XMFLOAT3& num1, XMFLOAT3 num2)
{
	num1.x += num2.x;
	num1.y += num2.y;
	num1.z += num2.z;
}

inline XMFLOAT3 operator * (XMFLOAT3 num1, float num2)
{
	return { num1.x * num2,num1.y * num2,num1.z * num2 };
}

ostream& operator <<(ostream& os, XMFLOAT3 float3);

ostream& operator <<(ostream& os, XMFLOAT4 float4);

ostream& operator <<(ostream& os, XMFLOAT4X4 mat);

XMMATRIX CalcuWorldMat(XMFLOAT3 Trans, XMFLOAT3 Rotat, XMFLOAT3 Scale);

XMMATRIX CalcuProjMat(float angleSize, float width, float height, float nearDistance = 1.f, float farDistance = 1000.f);

XMFLOAT4X4 LoadMat(XMMATRIX mat);

XMFLOAT3 Normalize(XMFLOAT3 v);

float Gaussian(float x, float sigma = 1);

/**********************************************************D3D相关工具******************************************************************/

//更新上传堆
void UpdateUB(void* DataSrc, UINT CBSize, ComPtr<ID3D12Resource> ConstantBuffer, UINT Subresource = 0, D3D12_RANGE* ReadRange = nullptr);