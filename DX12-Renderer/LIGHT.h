#pragma once

#include"Struct_definition.h"
#include"tool.h"
#include<DirectXMath.h>

using namespace DirectX::PackedVector;

class LIGHT
{
	//环境光
	XMFLOAT3 AmbientLight = { 0.3,0.3,0.3 };
	mutex AmbientLightMutex;

	//平行光
	XMFLOAT3 ParallelLightIntensity = { 0,0,0 };
	XMFLOAT3 ParallelLightDirection = { -1,-1,1 };
	mutex ParallelLightMutex;

	//点光源
	vector<POINT_LIGHT> PointLightList;
	int MaxPointLightNum = 100;
	mutex PointLightMutex;

	//运行时间，用以计算点光源位置
	float RunTime = 0;

public:
	LIGHT()
	{
		AddPointLight({ 10, 10, 0 }, { 100,100,100 });
		AddPointLight({ 10, 10, -10 }, { 300,300,300 });
	}

//环境光相关
	//设置环境光
	void SetAmbientLight(XMFLOAT3 ambientLight)
	{
		lock_guard<mutex> lg(AmbientLightMutex);
		AmbientLight = ambientLight;
	}

//平行光相关
	//设置光强
	void SetParallelIntensity(XMFLOAT3 intensity)
	{
		lock_guard<mutex> lg(ParallelLightMutex);
		ParallelLightIntensity = intensity;
	}

	//设置方向
	void SetParallelDirection(XMFLOAT3 direction)
	{
		lock_guard<mutex> lg(ParallelLightMutex);
		ParallelLightDirection = direction;
	}

//点光源相关
	//增加点光源
	void AddPointLight(XMFLOAT3 position, XMFLOAT3 intensity, float DecayStartDistance = 1)
	{
		if (PointLightList.size() == MaxPointLightNum)
		{
			cout << "点光源添加失败：已达到最大数量（" << MaxPointLightNum << ")" << endl;
		}
		else
		{
			lock_guard<mutex> lg(PointLightMutex);
			PointLightList.push_back({ position,DecayStartDistance ,intensity });
		}
	}

	//返回点光源最大数量
	int GetMaxPointLightNum()
	{
		return MaxPointLightNum;
	}

	//返回点光源数量
	int GetPointLightNum()
	{
		return PointLightList.size();
	}

	//返回点光源位置
	XMFLOAT3 GetPointLightPosition(int index)
	{
		return PointLightList[index].Position;
	}

	//设置点光源距离、强度
	void SetPointLightPosition(int index, XMFLOAT3 position)
	{
		PointLightList[index].Position = position;
	}
	void SetPointLightIntensity(int index, XMFLOAT3 intensity)
	{
		PointLightList[index].Intensity = intensity;
	}

//资源更新
	//更新环境光和平行光
	LIGHT_FRAME_DATA GetLightResource()
	{
		lock_guard<mutex> lg1(AmbientLightMutex);
		lock_guard<mutex> lg2(ParallelLightMutex);

		LIGHT_FRAME_DATA Resource;
		Resource.AmbientLight = AmbientLight;
		Resource.ParallelLightDirection = ParallelLightDirection;
		Resource.ParallelLightIntensity = ParallelLightIntensity;

		return Resource;
	}

	//更新点光源
	void UpdatePointLightResource(ComPtr<ID3D12Resource> pointLightResource, float time)
	{
		lock_guard<mutex> lg(PointLightMutex);

		float TimeDiffer = time - RunTime;
		RunTime = time;

		auto RotateMat = XMMatrixRotationY(TimeDiffer);

		for (int i = 0; i < PointLightList.size(); i++)
		{
			auto PosVec = XMLoadFloat3(&PointLightList[i].Position);
			PosVec = XMVector3TransformCoord(PosVec, RotateMat);

			XMStoreFloat3(&PointLightList[i].Position, PosVec);
		}

		UpdateUB(PointLightList.data(), sizeof(POINT_LIGHT_FRAME_DATA) * PointLightList.size(), pointLightResource);
	}
};
