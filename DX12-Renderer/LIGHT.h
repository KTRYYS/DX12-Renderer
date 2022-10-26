#pragma once

#include"Struct_definition.h"
#include"tool.h"
#include<DirectXMath.h>

using namespace DirectX::PackedVector;

class LIGHT
{
	//������
	XMFLOAT3 AmbientLight = { 0.3,0.3,0.3 };
	mutex AmbientLightMutex;

	//ƽ�й�
	XMFLOAT3 ParallelLightIntensity = { 0,0,0 };
	XMFLOAT3 ParallelLightDirection = { -1,-1,1 };
	mutex ParallelLightMutex;

	//���Դ
	vector<POINT_LIGHT> PointLightList;
	int MaxPointLightNum = 100;
	mutex PointLightMutex;

	//����ʱ�䣬���Լ�����Դλ��
	float RunTime = 0;

public:
	LIGHT()
	{
		AddPointLight({ 10, 10, 0 }, { 100,100,100 });
		AddPointLight({ 10, 10, -10 }, { 300,300,300 });
	}

//���������
	//���û�����
	void SetAmbientLight(XMFLOAT3 ambientLight)
	{
		lock_guard<mutex> lg(AmbientLightMutex);
		AmbientLight = ambientLight;
	}

//ƽ�й����
	//���ù�ǿ
	void SetParallelIntensity(XMFLOAT3 intensity)
	{
		lock_guard<mutex> lg(ParallelLightMutex);
		ParallelLightIntensity = intensity;
	}

	//���÷���
	void SetParallelDirection(XMFLOAT3 direction)
	{
		lock_guard<mutex> lg(ParallelLightMutex);
		ParallelLightDirection = direction;
	}

//���Դ���
	//���ӵ��Դ
	void AddPointLight(XMFLOAT3 position, XMFLOAT3 intensity, float DecayStartDistance = 1)
	{
		if (PointLightList.size() == MaxPointLightNum)
		{
			cout << "���Դ���ʧ�ܣ��Ѵﵽ���������" << MaxPointLightNum << ")" << endl;
		}
		else
		{
			lock_guard<mutex> lg(PointLightMutex);
			PointLightList.push_back({ position,DecayStartDistance ,intensity });
		}
	}

	//���ص��Դ�������
	int GetMaxPointLightNum()
	{
		return MaxPointLightNum;
	}

	//���ص��Դ����
	int GetPointLightNum()
	{
		return PointLightList.size();
	}

	//���ص��Դλ��
	XMFLOAT3 GetPointLightPosition(int index)
	{
		return PointLightList[index].Position;
	}

	//���õ��Դ���롢ǿ��
	void SetPointLightPosition(int index, XMFLOAT3 position)
	{
		PointLightList[index].Position = position;
	}
	void SetPointLightIntensity(int index, XMFLOAT3 intensity)
	{
		PointLightList[index].Intensity = intensity;
	}

//��Դ����
	//���»������ƽ�й�
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

	//���µ��Դ
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
