#pragma once

#include<dxgi.h>
#include<direct.h>
#include<d3d12.h>
#include<DirectXMath.h>
#include<array>
#include<vector>
#include<DirectXColors.h>
#include<DirectXPackedVector.h>
#include<d3dcompiler.h>
#include<thread>
#include<future>

//辅助函数库，需要另外准备
#include"d3dx12.h"

//上传纹理所用
#include<ResourceUploadBatch.h>
#include<DDSTextureLoader.h>

//宏定义
#include"Macro.h"

//链接必要的D3D12库
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace std;
using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

class D3DBASE
{
protected:

#ifndef NDEBUG
	//调试层接口
	ComPtr<ID3D12Debug> DebugController;
	ComPtr<ID3D12Debug1> DebugController2;
#endif

	//窗口信息
	HWND hwnd;
	UINT ClientWidth;
	UINT ClientHeight;

	//DXGI和硬件部分
	ComPtr<IDXGIFactory> Factory;
	ComPtr<IDXGIAdapter> Adapter;
	ComPtr<ID3D12Device> Device;
	UINT GPUIndex = 0;
	UINT GPUNodeMask;

	//围栏
	ComPtr<ID3D12Fence> Fence;
	UINT FenceNum;

	//描述符大小
	UINT RTVSize;
	UINT DSVSize;
	UINT CSUSize;

	//主渲染命令队列
	ComPtr<ID3D12CommandQueue> MainCmdQueue;

	//交换链部分
	ComPtr<IDXGISwapChain> SwapChain;
	static const UINT SwapChainBufferNumber = 2;
	UINT BackBufferIndex = 0;
	ComPtr<ID3D12Resource> SwapChainBuffer[SwapChainBufferNumber];

	//渲染目标缓冲区
	DXGI_FORMAT RTFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	ComPtr<ID3D12DescriptorHeap> RTVHeap;

	//深度模板缓冲区
	DXGI_FORMAT DSFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ComPtr<ID3D12Resource> DSBuffer;
	ComPtr<ID3D12DescriptorHeap> DSVHeap;

	//裁剪矩形
	D3D12_VIEWPORT ViewPort;                                               //视口描述结构体
	D3D12_RECT SciRect;                                                    //裁剪矩形描述结构体

public:
	D3DBASE(HWND Hwnd, UINT Width, UINT Height);
	D3DBASE(D3DBASE&) = delete;

protected:

//调试相关
	//开启调试
	void OpenDebug();

	//显示硬件和适配器
	void LogAdapter();

//基础初始化
	//初始化DXGI和硬件设备接口
	void InitialDXGIandDevice();

	//初始化围栏
	void InitialFence();

	//获得描述符大小
	void GetDescriptorSize();

//命令相关
	//创建命令队列
	ComPtr<ID3D12CommandQueue> CreateCmdQueue(D3D12_COMMAND_LIST_TYPE CmdType = D3D12_COMMAND_LIST_TYPE_DIRECT, LPCWSTR Name = nullptr);

	//创建命令列表和命令分配器
	//ComPtr<ID3D12GraphicsCommandList> CreateCmdList(ComPtr<ID3D12CommandAllocator>& CmdAllocator, D3D12_COMMAND_LIST_TYPE CmdType = D3D12_COMMAND_LIST_TYPE_DIRECT, LPCWSTR Name = nullptr);
	ComPtr<ID3D12GraphicsCommandList> CreateCmdList(ComPtr<ID3D12CommandAllocator>& CmdAllocator, LPCWSTR Name = nullptr, D3D12_COMMAND_LIST_TYPE CmdType = D3D12_COMMAND_LIST_TYPE_DIRECT);

	//重置命令列表
	void CmdReset(ComPtr<ID3D12GraphicsCommandList>& CmdList, ComPtr<ID3D12CommandAllocator>& CmdAllocator, ID3D12PipelineState* PSO);

	//执行命令
	void ExeCmd(ComPtr<ID3D12GraphicsCommandList>& CmdList, ComPtr<ID3D12CommandQueue> &CmdQueue);

	//等待命令队列完成
	void FluCmdQueue(ComPtr<ID3D12CommandQueue> CmdQueue, HANDLE Event = NULL);

//流水线部分
	//创建交换链和渲染目标缓冲区
	void CreateSwapChain();

	//创建深度模板缓冲区
	void CreateDS();

	//设置视口和裁剪矩形
	void SetViewPortAndSciRect(ComPtr<ID3D12GraphicsCommandList>CmdList);

	//重制渲染目标缓冲区和深度模板缓冲区（大小）
	void ResizeRTandDS();

	//创建根签名
	ComPtr<ID3D12RootSignature> CreateRootSig(D3D12_ROOT_SIGNATURE_DESC RootSigDesc);

//PSO
	//创建基础PSO
	ComPtr<ID3D12PipelineState> CreateBasicPSO(ComPtr<ID3D12RootSignature> RootSignature,
		D3D12_SHADER_BYTECODE VSByteCodeDesc, D3D12_SHADER_BYTECODE PSByteCodeDesc,
		D3D12_INPUT_LAYOUT_DESC IAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	////创建曲面细分PSO
	//ComPtr<ID3D12PipelineState> CreateTessPSO(ComPtr<ID3D12RootSignature> RootSignature, D3D12_INPUT_LAYOUT_DESC IAInputDesc,
	//	D3D12_SHADER_BYTECODE VSByteCodeDesc, D3D12_SHADER_BYTECODE HSByteCodeDesc, D3D12_SHADER_BYTECODE DSByteCodeDesc,
	//	D3D12_SHADER_BYTECODE PSByteCodeDesc, D3D12_SHADER_BYTECODE GSByteCodeDesc = {});
	//
	////创建水的PSO
	//ComPtr<ID3D12PipelineState> CreateWaterPSO(ComPtr<ID3D12RootSignature> RootSignature, D3D12_INPUT_LAYOUT_DESC IAInputDesc,
	//	D3D12_SHADER_BYTECODE VSByteCodeDesc, D3D12_SHADER_BYTECODE HSByteCodeDesc, D3D12_SHADER_BYTECODE DSByteCodeDesc,
	//	D3D12_SHADER_BYTECODE PSByteCodeDesc, D3D12_SHADER_BYTECODE GSByteCodeDesc = {});

	//创建计算流水线PSO
	ComPtr<ID3D12PipelineState> CreateComputePSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE cSByteCodeDesc,
		D3D12_CACHED_PIPELINE_STATE cachedPSO = {});

//采样器


//资源相关
	//创建默认堆并更新默认堆资源
	ComPtr<ID3D12Resource> CreateDefaultBuffer(UINT64 ResourceSize, const void* ResourceData, LPCWSTR name = nullptr);
	
	ComPtr<ID3D12Resource> CreateUploadBuffer(UINT64 resourceSize);

	//创建常量缓冲区，默认上传堆，返回其描述符的描述
	D3D12_CONSTANT_BUFFER_VIEW_DESC CreateConstantBuffer(ComPtr<ID3D12Resource>& ConstantUploadBuffer, UINT DataSize , D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD);

	//上传纹理资源
	ComPtr<ID3D12Resource> UploadTexture(wstring File);

	//更新默认堆
	void UpdateDB(ComPtr<ID3D12Resource> defaultBuffer, void* sourceData, UINT dataSize, D3D12_RESOURCE_STATES beforeBarrier = D3D12_RESOURCE_STATE_COMMON);

//创建CSU缓冲区描述符及堆
	//创建综合堆
	ComPtr<ID3D12DescriptorHeap> CreateCSUHeap(vector<D3D12_CONSTANT_BUFFER_VIEW_DESC>& CBVDescList, vector<D3D12_SHADER_RESOURCE_VIEW_DESC>& SRVDescList,
		vector< D3D12_UNORDERED_ACCESS_VIEW_DESC>& UAVDescList, vector<ID3D12Resource*>& ShaderResource,
		vector<ID3D12Resource*>& UAResource1, vector<ID3D12Resource*>& UAResource2);

	//CBV
	ComPtr<ID3D12DescriptorHeap> CreateCSUHeap(vector<D3D12_CONSTANT_BUFFER_VIEW_DESC>& CBVDescList)
	{
		vector<D3D12_SHADER_RESOURCE_VIEW_DESC> v1;
		vector< D3D12_UNORDERED_ACCESS_VIEW_DESC> v2;
		vector<ID3D12Resource*> v3;
		vector<ID3D12Resource*> v4;
		vector<ID3D12Resource*> v5;

		CreateCSUHeap(CBVDescList, v1, v2, v3, v4, v5);
	}

//工具
	//创建描述符堆
	ComPtr<ID3D12DescriptorHeap> CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags, UINT DescNum = 1);

	//编译着色器
	D3D12_SHADER_BYTECODE CompileShader(ComPtr<ID3DBlob>& ShaderByteCode, wchar_t* FileName, char* FunctionName, LPCSTR ShaderVersion,
		D3D_SHADER_MACRO* ShaderMacro = NULL, ID3DInclude* HandleIncludeMacro = NULL, UINT Flag1 = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);

};