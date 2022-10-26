#pragma

#include "framework.h"
#include "DX12-Renderer.h"
#include"D3DAPP.h"

D3DAPP* D3DPtr;

LRESULT WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (D3DPtr)
	{
		D3DPtr->WndProc(msg, wparam, lparam);
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);            //默认处理消息
}

int WINAPI wWinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int mCmdShow)
{

	//开启控制台
	AllocConsole();
	FILE* stream1;
	freopen_s(&stream1, "CONOUT$", "w", stdout);

	UINT ClientWidth = 800;                                      //窗口客户区宽度
	UINT ClientHeight = 600;                                     //高度

//窗口初始化
	const wchar_t class_name[] = L"D3D学习DEMO";                 //窗口名

	//窗口属性
	WNDCLASS wc = {};
	wc.style = CS_VREDRAW | CS_HREDRAW;                          //如果改变窗口高/宽度则重绘窗口
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);       //防止背景重绘
	wc.hInstance = hinstance;
	wc.lpszClassName = class_name;                               //窗口类名
	wc.lpfnWndProc = WinProc;                                    //窗口过程

	//注册窗口
	RegisterClass(&wc);

	//创建窗口实例
	HWND hwnd = CreateWindowEx(
		0, class_name, L"D3D学习DEMO",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		ClientWidth, ClientHeight, NULL, NULL, hinstance, NULL);

	//显示窗口
	ShowWindow(hwnd, mCmdShow);

//D3D
	D3DAPP D3DApp(hwnd, ClientWidth, ClientHeight);
	D3DPtr = &D3DApp;

	thread th([] {D3DPtr->Run(); });
	th.detach();

//消息循环
	MSG msg;
	while (1)
	{
		if (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}