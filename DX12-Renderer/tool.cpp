#pragma once

#include"tool.h"

ostream& operator <<(ostream& os, XMFLOAT3 float3)
{
	os << "(" << float3.x << "  " << float3.y << "  " << float3.z << ")";
	return os;
}

ostream& operator <<(ostream& os, XMFLOAT4 float4)
{
	os << "(" << float4.x << "  " << float4.y << "  " << float4.z << " " << float4.w << ")";
	return os;
}

ostream& operator <<(ostream& os, XMFLOAT4X4 mat)
{
	os << "\n" << mat._11 << " " << mat._12 << " " << mat._13 << " " << mat._14 << "\n" <<
		mat._21 << " " << mat._22 << " " << mat._23 << " " << mat._24 << "\n" <<
		mat._31 << " " << mat._32 << " " << mat._33 << " " << mat._34 << "\n" <<
		mat._41 << " " << mat._42 << " " << mat._43 << " " << mat._44 << "\n" << endl;

	return os;
}

XMMATRIX CalcuWorldMat(XMFLOAT3 Trans, XMFLOAT3 Rotat, XMFLOAT3 Scale)
{
	//缩放矩阵
	XMMATRIX ScaleMat = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

	//旋转矩阵
	XMMATRIX RotatMat = XMMatrixRotationX(Rotat.x) * XMMatrixRotationY(Rotat.y) * XMMatrixRotationZ(Rotat.z);

	//平移矩阵
	XMMATRIX TransMat = XMMatrixTranslation(Trans.x, Trans.y, Trans.z);

	//世界矩阵
	return ScaleMat * RotatMat * TransMat;
}

XMMATRIX CalcuProjMat(float angleSize, float width, float height, float nearDistance, float farDistance)
{
	float FovAngelY = angleSize * XM_PI;
	float AspectRatio = (float)width / (float)height;

	return XMMatrixPerspectiveFovLH(FovAngelY, AspectRatio, nearDistance, farDistance);
}

XMFLOAT4X4 LoadMat(XMMATRIX mat)
{
	XMFLOAT4X4 Mat;
	XMStoreFloat4x4(&Mat, XMMatrixTranspose(mat));
	return Mat;
}

XMFLOAT3 Normalize(XMFLOAT3 v)
{
	XMVECTOR V = XMLoadFloat3(&v);
	XMVECTOR NormalV = XMVector3Normalize(V);

	XMFLOAT3 re;
	XMStoreFloat3(&re, NormalV);

	return re;
}

void UpdateUB(void* DataSrc, UINT CBSize, ComPtr<ID3D12Resource> ConstantBuffer, UINT Subresource, D3D12_RANGE* ReadRange)
{
	//获得复制目标区域指针
	void* DataDst;
	ThrowIfFailed(ConstantBuffer->Map(Subresource, ReadRange, &DataDst));

	//复制
	memcpy(DataDst, DataSrc, CBSize);

	//释放映射
	ConstantBuffer->Unmap(Subresource, ReadRange);
}

float Gaussian(float x, float sigma)
{
	return exp(-(x * x) / (2 * (sigma * sigma)));
}