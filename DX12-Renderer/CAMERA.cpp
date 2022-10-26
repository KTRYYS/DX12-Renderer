#pragma once

#include"CAMERA.h"
#include<Windows.h>

CAMERA::CAMERA()
{
	Position = { 10,0,0 };
	HorizontalAngle = XM_PI;
	VerticalAngle = 0;

	//thread th(&CAMERA::test, this);
	//th.detach();
}

void CAMERA::test()
{
	while (1)
	{
		lock(PosMutex, AngleMutex);
		cout << "相机位置：" << Position << endl;
		cout << "相机视角 水平：" << HorizontalAngle << "垂直：" << VerticalAngle << endl;
		PosMutex.unlock();
		AngleMutex.unlock();
		Sleep(300);
	}
}

XMFLOAT3 CAMERA::GetAngle()
{
	float hor;
	float ver;
	{
		lock_guard<mutex> guard(AngleMutex);
		hor = HorizontalAngle;
		ver = VerticalAngle;
	}

	XMFLOAT3 angle;
	angle.x = cosf(hor) * cosf(ver);
	angle.y = sinf(hor);
	angle.z = cosf(hor) * sinf(ver);

	return angle;
}

void CAMERA::Move(float forward, float right, float up)
{
	float HorAng;

	{
		lock_guard<mutex> guard(AngleMutex);
		HorAng = HorizontalAngle;
	}

	//前后方向
	XMFLOAT3 Forward{ cosf(HorAng),0,sinf(HorAng) };

	//左右方向
	XMFLOAT3 Right{ cosf(HorAng - XM_PI / 2),0,sinf(HorAng - XM_PI / 2) };

	//上下方向
	XMFLOAT3 Up{ 0,1,0 };

	lock_guard<mutex> guard(PosMutex);
	Position += Forward * forward;
	Position += Right * right;
	Position += Up * up;
}

void CAMERA::SetPosition(XMFLOAT3 MoveDistance)
{
	lock_guard<mutex> guard(PosMutex);
	Position += MoveDistance;
}

void CAMERA::SetAngle(float horiz, float verti)
{
	lock_guard<mutex> guard(AngleMutex);
	HorizontalAngle += horiz;
	VerticalAngle += verti;
}

XMMATRIX  CAMERA::CameraFrameSource(XMFLOAT3& CameraPos)
{
	float AngleX, AngleY, AngleZ;
	XMFLOAT3 position;

	{
		lock(PosMutex, AngleMutex);

		AngleX = cosf(HorizontalAngle) * cosf(VerticalAngle);
		AngleY = sinf(VerticalAngle);
		AngleZ = sinf(HorizontalAngle) * cosf(VerticalAngle);

		position = Position;
		PosMutex.unlock();
		AngleMutex.unlock();
	}

	//返回摄像机位置
	CameraPos = { position.x,position.y,position.z};
	
	//摄像机空间变换矩阵
	XMFLOAT4 cameraPos = { position.x,position.y,position.z,1.f };
	XMVECTOR Pos = XMLoadFloat4(&cameraPos);
	XMVECTOR Target = XMVectorSet(position.x + AngleX, position.y + AngleY, position.z + AngleZ, 1);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	return XMMatrixLookAtLH(Pos, Target, Up);
}