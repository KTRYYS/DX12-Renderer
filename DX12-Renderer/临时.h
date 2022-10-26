#pragma once
#include<DirectXMath.h>

using namespace DirectX;

//�����λ�˲���
XMFLOAT3 CameraPos;                                                    //���������
XMFLOAT3 CameraTarget;                                                 //�۲��
XMFLOAT3 CameraUp;                                                     //��������Ϸ���

//��Ұ����
float FovAngleY;                                                       //��ֱ�ӳ���
float NearDistance;                                                    //��ƽ�����
float FarDistance;                                                     //Զƽ�����

//�任����
XMMATRIX ViewMat;                                                      //�۲����
XMMATRIX ProjMat;                                                      //ͶӰ����

//����������
	//�����������
XMMATRIX CalcuWorlddMat(XMFLOAT3 Scale = { 1,1,1 }, XMFLOAT3 Rotat = { 0,0,0 }, XMFLOAT3 Trans = { 0,0,0 });
XMMATRIX CalcuWorlddMat(XMFLOAT3 Scale = { 1,1,1 }, XMFLOAT3 Axis = { 0,1,0 }, float Angle = 0, XMFLOAT3 Trans = { 0,0,0 });

//����۲����
void CalcuViewMat();

//����ͶӰ����
void CacuProjMat();

//���㸴�ϱ任����
XMFLOAT4X4 CalcuWldVProjMat(XMMATRIX WorldMat);

XMMATRIX CalcuWorlddMat(XMFLOAT3 Scale, XMFLOAT3 Rotat, XMFLOAT3 Trans)
{
	//���ž���
	XMMATRIX ScaleMat = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

	//��ת����
	XMMATRIX RotatMat = XMMatrixRotationX(Rotat.x) * XMMatrixRotationY(Rotat.y) * XMMatrixRotationZ(Rotat.z);

	//ƽ�ƾ���
	XMMATRIX TransMat = XMMatrixTranslation(Trans.x, Trans.y, Trans.z);

	//�������
	return ScaleMat * RotatMat * TransMat;
}

XMMATRIX CalcuWorlddMat(XMFLOAT3 Scale, XMFLOAT3 Axis, float Angle, XMFLOAT3 Trans)
{
	//���ž���
	XMMATRIX ScaleMat = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

	//��ת����
	XMVECTOR AxisVector = XMLoadFloat3(&Axis);
	XMMATRIX RotatMat = XMMatrixRotationAxis(AxisVector, Angle);

	//ƽ�ƾ���
	XMMATRIX TransMat = XMMatrixTranslation(Trans.x, Trans.y, Trans.z);

	//�������
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