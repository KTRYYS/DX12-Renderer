#pragma once

#include<Windows.h>
#include<mutex>
#include"tool.h"

class Time
{
protected:
	//CPU��ʱ��ʱ����
	float TimingInterval;

private:
	//ʱ��
	__int64 AppStartTime = 0;                      //��ʼ��ʱʱ��
	__int64 AppLastTime = 0;                       //�ϴμ�¼ʱ��

	//֡���
	int TotalFrames = 0;                           //��֡��
	int MinFrameRate = INT_MAX;                    //���֡��
	int MaxFrameRate = INT_MIN;                    //���֡��
	int AvgFrameRate = 0;                          //ƽ��֡��
	int CurrentFrameRate = 0;                      //��ǰ֡��

	mutex FramesMutex;

protected:
	Time()
	{
		//��ü�ʱ����ʱ���
		__int64 TimingFrequency;
		QueryPerformanceFrequency((LARGE_INTEGER*)&TimingFrequency);
		TimingInterval = 1 / (float)TimingFrequency;
	}

	//���ÿ�ʼ��ʱʱ��
	void StartTimer()
	{
		__int64 CurrentTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);
		AppStartTime = AppLastTime = CurrentTime;
	}

	//֡����
	void AddFrames(int DifferFrame = 1)
	{
		TotalFrames += DifferFrame;
	}

	//֡����
	void CalcuFrames();

	//����֡��
	void GetFrames(int& cur, int& min, int& max, int& avg)
	{
		lock_guard<mutex> lg(FramesMutex);
		cur = CurrentFrameRate;
		min = MinFrameRate;
		max = MaxFrameRate;
		avg = AvgFrameRate;
	}

	//��������ʱ��
	float GetRunTime()
	{
		return (AppLastTime - AppStartTime) * TimingInterval;
	}

};
