#pragma once

#include<Windows.h>
#include<mutex>
#include"tool.h"

class Time
{
protected:
	//CPU计时器时间间隔
	float TimingInterval;

private:
	//时间
	__int64 AppStartTime = 0;                      //开始计时时间
	__int64 AppLastTime = 0;                       //上次记录时间

	//帧相关
	int TotalFrames = 0;                           //总帧数
	int MinFrameRate = INT_MAX;                    //最低帧率
	int MaxFrameRate = INT_MIN;                    //最高帧率
	int AvgFrameRate = 0;                          //平均帧率
	int CurrentFrameRate = 0;                      //当前帧率

	mutex FramesMutex;

protected:
	Time()
	{
		//获得计时器计时间隔
		__int64 TimingFrequency;
		QueryPerformanceFrequency((LARGE_INTEGER*)&TimingFrequency);
		TimingInterval = 1 / (float)TimingFrequency;
	}

	//设置开始计时时间
	void StartTimer()
	{
		__int64 CurrentTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);
		AppStartTime = AppLastTime = CurrentTime;
	}

	//帧增加
	void AddFrames(int DifferFrame = 1)
	{
		TotalFrames += DifferFrame;
	}

	//帧计算
	void CalcuFrames();

	//返回帧率
	void GetFrames(int& cur, int& min, int& max, int& avg)
	{
		lock_guard<mutex> lg(FramesMutex);
		cur = CurrentFrameRate;
		min = MinFrameRate;
		max = MaxFrameRate;
		avg = AvgFrameRate;
	}

	//返回运行时间
	float GetRunTime()
	{
		return (AppLastTime - AppStartTime) * TimingInterval;
	}

};
