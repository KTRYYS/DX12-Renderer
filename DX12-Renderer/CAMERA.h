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
//相机参数
	//位置
	XMFLOAT3 Position;
	mutex PosMutex;

	//视角角度（以世界坐标原点为原点）
	float HorizontalAngle;                          //水平角
	float VerticalAngle;                          //竖直角
	mutex AngleMutex;


	//相机位置变动
	void SetPosition(XMFLOAT3 MoveDistance);

	//获取相机角度
	XMFLOAT3 GetAngle();

public:
	CAMERA();

	//相机前后移动
	void Move(float x, float y, float z);

	//相机角度变动
	void SetAngle(float horz, float verti);

	//获得相机帧资源
	XMMATRIX CameraFrameSource(XMFLOAT3& CameraPos);

	void test();

};



