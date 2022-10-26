#pragma once

#include"Time.h"

void Time::CalcuFrames()
{
	//��õ�ǰʱ�䲢����ʱ��
	__int64 CurrentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);
	float DifferTime = (CurrentTime - AppLastTime) * TimingInterval;
	AppLastTime = CurrentTime;
	
	{
		lock_guard<mutex> lg(FramesMutex);
		CurrentFrameRate = 1 / DifferTime;
		MinFrameRate = min(CurrentFrameRate, MinFrameRate);
		MaxFrameRate = max(CurrentFrameRate, MaxFrameRate);
		AvgFrameRate = TotalFrames / ((CurrentTime - AppStartTime) * TimingInterval);
	}
}