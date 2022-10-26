#pragma once
#include<DirectXMath.h>

using namespace DirectX;

//摄像机位姿参数
XMFLOAT3 CameraPos;                                                    //摄像机坐标
XMFLOAT3 CameraTarget;                                                 //观察点
XMFLOAT3 CameraUp;                                                     //摄像机向上方向

//视野参数
float FovAngleY;                                                       //垂直视场角
float NearDistance;                                                    //近平面距离
float FarDistance;                                                     //远平面距离

//变换矩阵
XMMATRIX ViewMat;                                                      //观察矩阵
XMMATRIX ProjMat;                                                      //投影矩阵

//矩阵计算相关
	//计算世界矩阵
XMMATRIX CalcuWorlddMat(XMFLOAT3 Scale = { 1,1,1 }, XMFLOAT3 Rotat = { 0,0,0 }, XMFLOAT3 Trans = { 0,0,0 });
XMMATRIX CalcuWorlddMat(XMFLOAT3 Scale = { 1,1,1 }, XMFLOAT3 Axis = { 0,1,0 }, float Angle = 0, XMFLOAT3 Trans = { 0,0,0 });

//计算观察矩阵
void CalcuViewMat();

//计算投影矩阵
void CacuProjMat();

//计算复合变换矩阵
XMFLOAT4X4 CalcuWldVProjMat(XMMATRIX WorldMat);

XMMATRIX CalcuWorlddMat(XMFLOAT3 Scale, XMFLOAT3 Rotat, XMFLOAT3 Trans)
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

XMMATRIX CalcuWorlddMat(XMFLOAT3 Scale, XMFLOAT3 Axis, float Angle, XMFLOAT3 Trans)
{
	//缩放矩阵
	XMMATRIX ScaleMat = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

	//旋转矩阵
	XMVECTOR AxisVector = XMLoadFloat3(&Axis);
	XMMATRIX RotatMat = XMMatrixRotationAxis(AxisVector, Angle);

	//平移矩阵
	XMMATRIX TransMat = XMMatrixTranslation(Trans.x, Trans.y, Trans.z);

	//世界矩阵
	return ScaleMat * RotatMat * TransMat;
}

void CalcuViewMat()
{
	XMVECTOR camerapos = XMVectorSet(CameraPos.x, CameraPos.y, CameraPos.z, 1.0f);
	XMVECTOR cameratarget = XMVectorSet(CameraTarget.x, CameraTarget.y, CameraTarget.z, 1.0f);
	XMVECTOR cameraup = XMVectorSet(CameraUp.x, CameraUp.y, CameraUp.z, 0.f);

	ViewMat = XMMatrixLookAtLH(camerapos, cameratarget, cameraup);
}

void CacuProjMat()
{
	float AspectRatio = (float)ClientWidth / (float)ClientHeight;

	ProjMat = XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearDistance, FarDistance);
}

XMFLOAT4X4 CalcuWldVProjMat(XMMATRIX WorldMat)
{
	XMFLOAT4X4 ComposMat;

	XMStoreFloat4x4(&ComposMat, XMMatrixTranspose(WorldMat * ViewMat * ProjMat));

	return ComposMat;
}