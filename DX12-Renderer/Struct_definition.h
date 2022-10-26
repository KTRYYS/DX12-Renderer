#pragma once

#include<d3d12.h>
#include<DirectXColors.h>
#include<DirectXPackedVector.h>
#include<DirectXMath.h>
#include<windows.h>
#include<windowsx.h>
#include<string>
#include<WRL.h>
#include<vector>
#include<mutex>

using namespace std;
using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

class GEOMETRY;

//点光源
struct POINT_LIGHT
{
	//位置
	XMFLOAT3 Position;

	//开始衰减距离
	float DecayStartDistance;

	//光强
	XMFLOAT3 Intensity;
};

//纹理
struct TEXTURE_STRUCT
{
	ComPtr<ID3D12Resource> Texture;
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
};

//物体类型
enum GEOMETRY_TYPE
{
	common,
	common_instance
};

//物体网格
enum MESH_TYPE
{
	cube0,
	flat0,
};

//阴影贴图
struct SHADOW_ITEM
{
	ComPtr<ID3D12DescriptorHeap> DescHeap;
	ComPtr<ID3D12Resource> VPMatBuffer;
	XMMATRIX PVMat;
};

//顶点
struct VERTEX
{
	XMFLOAT3 Pos;              //位置
	XMFLOAT3 Normal;           //向量
	XMFLOAT2 TexCoord;         //纹理坐标
	XMFLOAT4 Color;            //颜色

	VERTEX() = default;
	VERTEX(XMFLOAT3 pos, XMFLOAT4 color) :Pos(pos), Color(color) {};
	VERTEX(XMFLOAT3 pos, XMFLOAT3 normal, XMFLOAT2 texCoord) :Pos(pos), Normal(normal), TexCoord(texCoord) {};
};

struct SSAO_RESOURCE
{
	XMFLOAT4 NormalList[14];
};

struct AO_BUFFER_BLUR_DATA
{
	int Width;
	int Height;
};

//帧资源-------------------------------------------------------------------------------------------------------------------------

//一般帧资源
struct FRAME_DATA
{
	//VP矩阵
	XMFLOAT4X4 VPMat;

	//相机位置
	XMFLOAT3 CameraPosition;

	//时间
	float Time;
};

//环境光和平行光帧资源
struct LIGHT_FRAME_DATA
{
	XMFLOAT3 AmbientLight;
	float pad1;
	XMFLOAT3 ParallelLightIntensity;
	float pad2;
	XMFLOAT3 ParallelLightDirection;
};

//点光源帧资源
struct POINT_LIGHT_FRAME_DATA
{
	POINT_LIGHT PointLight;
};

//物体PBR项
struct PBR_ITEM
{
	XMFLOAT3 F0;                      //菲涅尔系数
	float Roughness;                //粗糙度
	float Metallicity;             //金属度
	
	PBR_ITEM() = default;
	PBR_ITEM(XMFLOAT3 f0, float roughness, float metallicity)
		:F0(f0), Roughness(roughness), Metallicity(metallicity) {}
};

//物体常量帧资源
struct GEOMETRY_FRAME_DATA
{
	XMFLOAT4X4 MMat = {};
	PBR_ITEM PBRItem;

	GEOMETRY_FRAME_DATA() = default;
	GEOMETRY_FRAME_DATA(XMFLOAT4X4 mMat, PBR_ITEM pBRItem) : MMat(mMat), PBRItem(pBRItem) {}
};

struct DEFERRED_RENDER_DATA
{
	XMFLOAT4X4 InverseViewMat;
	float ClientWidth;
	float ClientHeight;
	float A;
	float B;
	float TanHalfFov;
};

//绘制资源-----------------------------------------------------------------------------------------------------------------------

//每帧绘制相关全局资源
struct DRAW_RESOURCE
{
	//帧资源
	ComPtr<ID3D12Resource> FrameBuffer;                                  //缓冲区
	D3D12_CONSTANT_BUFFER_VIEW_DESC FrameBufferViewDesc;                 //缓冲区描述

	//平行光和环境光帧资源
	ComPtr<ID3D12Resource> AmbParaLightBuffer;
	D3D12_CONSTANT_BUFFER_VIEW_DESC AmbParaLightBufferViewDesc;

	//点光源帧资源
	ComPtr<ID3D12Resource> PointLightBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC PointLightSRVDesc;

	//物体相关
	XMFLOAT4X4 VPMat;

	//临时渲染资源
	vector<ComPtr<ID3D12CommandAllocator>> TempAllocatorList;
	mutex AllocatorMutex;

	vector<shared_ptr<GEOMETRY>> TempGeoList;
	vector<shared_ptr<GEOMETRY>> TempGeoList2;
	mutex GeoListMutex;

	//D3D资源
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE DSVHeapHandle;

	//所有物体绘制结束判断相关
	int DrawThreadCompleteNum = 0;
	mutex DrawThreadNumMutex;
	condition_variable DrawCompleteCV;

//阴影相关
	vector<SHADOW_ITEM> ShadowItemList;                            //阴影资源列表

	D3D12_GPU_VIRTUAL_ADDRESS PointVPMatBufferAddr;

	XMMATRIX PointLightPVMat;                                      //光源PV矩阵
	D3D12_CPU_DESCRIPTOR_HANDLE ShadowMapDSHandle;                 //光源DS资源

	ComPtr<ID3D12Resource> ShadowMapBufferArray;                   //阴影贴图资源

	D3D12_SHADER_RESOURCE_VIEW_DESC ShadowMapSRVDesc;              //阴影贴图SRV描述

	//VPT矩阵
	vector<XMFLOAT4X4> PointVPTMatList;                           //点光源VPN矩阵列表
	ComPtr<ID3D12Resource> PointLightVPTListBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC PointLightVPTListSRVDesc;

//渲染目标相关
	vector<ComPtr<ID3D12Resource>> RTBList;
	ComPtr<ID3D12DescriptorHeap> RTVHeap;
	ComPtr<ID3D12DescriptorHeap> GBufferCSUHeap;
	ComPtr<ID3D12DescriptorHeap> DeferredRenderSamplerDescHeap;

	ComPtr<ID3D12Resource> DeferredRenderCB;

	D3D12_CPU_DESCRIPTOR_HANDLE GBufferRTVHeapHandle;

	//DEFERRED_RENDER_DATA DeferredRenderData;

	//渲染GBuffer数量
	int GBufferNum = 3;

//SSAO
	ComPtr<ID3D12Resource> HBAOResource;
	D3D12_CONSTANT_BUFFER_VIEW_DESC HBAOResourceCBViewDesc;

	D3D12_CPU_DESCRIPTOR_HANDLE SSAORTVHeapHandle;

	ComPtr<ID3D12Resource> AOBlurWeightCB;
	ComPtr<ID3D12Resource> AOBlurResource;
	ComPtr<ID3D12DescriptorHeap> AOBlurCSUHeap;


//其它
	float CurrentTime;

	void ReleaseTempResource()
	{
		TempAllocatorList.clear();
		TempGeoList.clear();
	}

};

//物体绘制资源
struct GEOMETRY_DRAW_RESOURCE
{
	
	ComPtr<ID3D12Resource> GeoFrameResource;

	//描述符堆
	ComPtr<ID3D12DescriptorHeap> CSUDescHeap;
	ComPtr<ID3D12DescriptorHeap> SamplerDescHeap;

	ComPtr<ID3D12Resource> Texture;

	//实例是否改变
	bool isInstanceChange = false;

	//实例数
	int InstanceNum;
};