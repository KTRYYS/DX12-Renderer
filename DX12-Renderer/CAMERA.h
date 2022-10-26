#pragma once

#include<DirectXMath.h>
#include<thread>
#include<mutex>
#include"tool.h"

using namespace std;
using namespace DirectX;

class CAMERA
{
public:
	CAMERA* mycamera = this;
//�������
	//λ��
	XMFLOAT3 Position;
	mutex PosMutex;

	//�ӽǽǶȣ�����������ԭ��Ϊԭ�㣩
	float HorizontalAngle;                          //ˮƽ��
	float VerticalAngle;                          //��ֱ��
	mutex AngleMutex;


	//���λ�ñ䶯
	void SetPosition(XMFLOAT3 MoveDistance);

	//��ȡ����Ƕ�
	XMFLOAT3 GetAngle();

public:
	CAMERA();

	//���ǰ���ƶ�
	void Move(float x, float y, float z);

	//����Ƕȱ䶯
	void SetAngle(float horz, float verti);

	//������֡��Դ
	XMMATRIX CameraFrameSource(XMFLOAT3& CameraPos);

	void test();

};



