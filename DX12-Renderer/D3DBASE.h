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

//���������⣬��Ҫ����׼��
#include"d3dx12.h"

//�ϴ���������
#include<ResourceUploadBatch.h>
#include<DDSTextureLoader.h>

//�궨��
#include"Macro.h"

//���ӱ�Ҫ��D3D12��
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
	//���Բ�ӿ�
	ComPtr<ID3D12Debug> DebugController;
	ComPtr<ID3D12Debug1> DebugController2;
#endif

	//������Ϣ
	HWND hwnd;
	UINT ClientWidth;
	UINT ClientHeight;

	//DXGI��Ӳ������
	ComPtr<IDXGIFactory> Factory;
	ComPtr<IDXGIAdapter> Adapter;
	ComPtr<ID3D12Device> Device;
	UINT GPUIndex = 0;
	UINT GPUNodeMask;

	//Χ��
	ComPtr<ID3D12Fence> Fence;
	UINT FenceNum;

	//��������С
	UINT RTVSize;
	UINT DSVSize;
	UINT CSUSize;

	//����Ⱦ�������
	ComPtr<ID3D12CommandQueue> MainCmdQueue;

	//����������
	ComPtr<IDXGISwapChain> SwapChain;
	static const UINT SwapChainBufferNumber = 2;
	UINT BackBufferIndex = 0;
	ComPtr<ID3D12Resource> SwapChainBuffer[SwapChainBufferNumber];

	//��ȾĿ�껺����
	DXGI_FORMAT RTFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	ComPtr<ID3D12DescriptorHeap> RTVHeap;

	//���ģ�建����
	DXGI_FORMAT DSFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ComPtr<ID3D12Resource> DSBuffer;
	ComPtr<ID3D12DescriptorHeap> DSVHeap;

	//�ü�����
	D3D12_VIEWPORT ViewPort;                                               //�ӿ������ṹ��
	D3D12_RECT SciRect;                                                    //�ü����������ṹ��

public:
	D3DBASE(HWND Hwnd, UINT Width, UINT Height);
	D3DBASE(D3DBASE&) = delete;

protected:

//�������
	//��������
	void OpenDebug();

	//��ʾӲ����������
	void LogAdapter();

//������ʼ��
	//��ʼ��DXGI��Ӳ���豸�ӿ�
	void InitialDXGIandDevice();

	//��ʼ��Χ��
	void InitialFence();

	//�����������С
	void GetDescriptorSize();

//�������
	//�����������
	ComPtr<ID3D12CommandQueue> CreateCmdQueue(D3D12_COMMAND_LIST_TYPE CmdType = D3D12_COMMAND_LIST_TYPE_DIRECT, LPCWSTR Name = nullptr);

	//���������б�����������
	//ComPtr<ID3D12GraphicsCommandList> CreateCmdList(ComPtr<ID3D12CommandAllocator>& CmdAllocator, D3D12_COMMAND_LIST_TYPE CmdType = D3D12_COMMAND_LIST_TYPE_DIRECT, LPCWSTR Name = nullptr);
	ComPtr<ID3D12GraphicsCommandList> CreateCmdList(ComPtr<ID3D12CommandAllocator>& CmdAllocator, LPCWSTR Name = nullptr, D3D12_COMMAND_LIST_TYPE CmdType = D3D12_COMMAND_LIST_TYPE_DIRECT);

	//���������б�
	void CmdReset(ComPtr<ID3D12GraphicsCommandList>& CmdList, ComPtr<ID3D12CommandAllocator>& CmdAllocator, ID3D12PipelineState* PSO);

	//ִ������
	void ExeCmd(ComPtr<ID3D12GraphicsCommandList>& CmdList, ComPtr<ID3D12CommandQueue> &CmdQueue);

	//�ȴ�����������
	void FluCmdQueue(ComPtr<ID3D12CommandQueue> CmdQueue, HANDLE Event = NULL);

//��ˮ�߲���
	//��������������ȾĿ�껺����
	void CreateSwapChain();

	//�������ģ�建����
	void CreateDS();

	//�����ӿںͲü�����
	void SetViewPortAndSciRect(ComPtr<ID3D12GraphicsCommandList>CmdList);

	//������ȾĿ�껺���������ģ�建��������С��
	void ResizeRTandDS();

	//������ǩ��
	ComPtr<ID3D12RootSignature> CreateRootSig(D3D12_ROOT_SIGNATURE_DESC RootSigDesc);

//PSO
	//��������PSO
	ComPtr<ID3D12PipelineState> CreateBasicPSO(ComPtr<ID3D12RootSignature> RootSignature,
		D3D12_SHADER_BYTECODE VSByteCodeDesc, D3D12_SHADER_BYTECODE PSByteCodeDesc,
		D3D12_INPUT_LAYOUT_DESC IAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	////��������ϸ��PSO
	//ComPtr<ID3D12PipelineState> CreateTessPSO(ComPtr<ID3D12RootSignature> RootSignature, D3D12_INPUT_LAYOUT_DESC IAInputDesc,
	//	D3D12_SHADER_BYTECODE VSByteCodeDesc, D3D12_SHADER_BYTECODE HSByteCodeDesc, D3D12_SHADER_BYTECODE DSByteCodeDesc,
	//	D3D12_SHADER_BYTECODE PSByteCodeDesc, D3D12_SHADER_BYTECODE GSByteCodeDesc = {});
	//
	////����ˮ��PSO
	//ComPtr<ID3D12PipelineState> CreateWaterPSO(ComPtr<ID3D12RootSignature> RootSignature, D3D12_INPUT_LAYOUT_DESC IAInputDesc,
	//	D3D12_SHADER_BYTECODE VSByteCodeDesc, D3D12_SHADER_BYTECODE HSByteCodeDesc, D3D12_SHADER_BYTECODE DSByteCodeDesc,
	//	D3D12_SHADER_BYTECODE PSByteCodeDesc, D3D12_SHADER_BYTECODE GSByteCodeDesc = {});

	//����������ˮ��PSO
	ComPtr<ID3D12PipelineState> CreateComputePSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE cSByteCodeDesc,
		D3D12_CACHED_PIPELINE_STATE cachedPSO = {});

//������


//��Դ���
	//����Ĭ�϶Ѳ�����Ĭ�϶���Դ
	ComPtr<ID3D12Resource> CreateDefaultBuffer(UINT64 ResourceSize, const void* ResourceData, LPCWSTR name = nullptr);
	
	ComPtr<ID3D12Resource> CreateUploadBuffer(UINT64 resourceSize);

	//����������������Ĭ���ϴ��ѣ�������������������
	D3D12_CONSTANT_BUFFER_VIEW_DESC CreateConstantBuffer(ComPtr<ID3D12Resource>& ConstantUploadBuffer, UINT DataSize , D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD);

	//�ϴ�������Դ
	ComPtr<ID3D12Resource> UploadTexture(wstring File);

	//����Ĭ�϶�
	void UpdateDB(ComPtr<ID3D12Resource> defaultBuffer, void* sourceData, UINT dataSize, D3D12_RESOURCE_STATES beforeBarrier = D3D12_RESOURCE_STATE_COMMON);

//����CSU����������������
	//�����ۺ϶�
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

//����
	//������������
	ComPtr<ID3D12DescriptorHeap> CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags, UINT DescNum = 1);

	//������ɫ��
	D3D12_SHADER_BYTECODE CompileShader(ComPtr<ID3DBlob>& ShaderByteCode, wchar_t* FileName, char* FunctionName, LPCSTR ShaderVersion,
		D3D_SHADER_MACRO* ShaderMacro = NULL, ID3DInclude* HandleIncludeMacro = NULL, UINT Flag1 = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);

};