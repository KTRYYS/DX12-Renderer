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

//���Դ
struct POINT_LIGHT
{
	//λ��
	XMFLOAT3 Position;

	//��ʼ˥������
	float DecayStartDistance;

	//��ǿ
	XMFLOAT3 Intensity;
};

//����
struct TEXTURE_STRUCT
{
	ComPtr<ID3D12Resource> Texture;
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
};

//��������
enum GEOMETRY_TYPE
{
	common,
	common_instance
};

//��������
enum MESH_TYPE
{
	cube0,
	flat0,
};

//��Ӱ��ͼ
struct SHADOW_ITEM
{
	ComPtr<ID3D12DescriptorHeap> DescHeap;
	ComPtr<ID3D12Resource> VPMatBuffer;
	XMMATRIX PVMat;
};

//����
struct VERTEX
{
	XMFLOAT3 Pos;              //λ��
	XMFLOAT3 Normal;           //����
	XMFLOAT2 TexCoord;         //��������
	XMFLOAT4 Color;            //��ɫ

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

//֡��Դ-------------------------------------------------------------------------------------------------------------------------

//һ��֡��Դ
struct FRAME_DATA
{
	//VP����
	XMFLOAT4X4 VPMat;

	//���λ��
	XMFLOAT3 CameraPosition;

	//ʱ��
	float Time;
};

//�������ƽ�й�֡��Դ
struct LIGHT_FRAME_DATA
{
	XMFLOAT3 AmbientLight;
	float pad1;
	XMFLOAT3 ParallelLightIntensity;
	float pad2;
	XMFLOAT3 ParallelLightDirection;
};

//���Դ֡��Դ
struct POINT_LIGHT_FRAME_DATA
{
	POINT_LIGHT PointLight;
};

//����PBR��
struct PBR_ITEM
{
	XMFLOAT3 F0;                      //������ϵ��
	float Roughness;                //�ֲڶ�
	float Metallicity;             //������
	
	PBR_ITEM() = default;
	PBR_ITEM(XMFLOAT3 f0, float roughness, float metallicity)
		:F0(f0), Roughness(roughness), Metallicity(metallicity) {}
};

//���峣��֡��Դ
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

//������Դ-----------------------------------------------------------------------------------------------------------------------

//ÿ֡�������ȫ����Դ
struct DRAW_RESOURCE
{
	//֡��Դ
	ComPtr<ID3D12Resource> FrameBuffer;                                  //������
	D3D12_CONSTANT_BUFFER_VIEW_DESC FrameBufferViewDesc;                 //����������

	//ƽ�й�ͻ�����֡��Դ
	ComPtr<ID3D12Resource> AmbParaLightBuffer;
	D3D12_CONSTANT_BUFFER_VIEW_DESC AmbParaLightBufferViewDesc;

	//���Դ֡��Դ
	ComPtr<ID3D12Resource> PointLightBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC PointLightSRVDesc;

	//�������
	XMFLOAT4X4 VPMat;

	//��ʱ��Ⱦ��Դ
	vector<ComPtr<ID3D12CommandAllocator>> TempAllocatorList;
	mutex AllocatorMutex;

	vector<shared_ptr<GEOMETRY>> TempGeoList;
	vector<shared_ptr<GEOMETRY>> TempGeoList2;
	mutex GeoListMutex;

	//D3D��Դ
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE DSVHeapHandle;

	//����������ƽ����ж����
	int DrawThreadCompleteNum = 0;
	mutex DrawThreadNumMutex;
	condition_variable DrawCompleteCV;

//��Ӱ���
	vector<SHADOW_ITEM> ShadowItemList;                            //��Ӱ��Դ�б�

	D3D12_GPU_VIRTUAL_ADDRESS PointVPMatBufferAddr;

	XMMATRIX PointLightPVMat;                                      //��ԴPV����
	D3D12_CPU_DESCRIPTOR_HANDLE ShadowMapDSHandle;                 //��ԴDS��Դ

	ComPtr<ID3D12Resource> ShadowMapBufferArray;                   //��Ӱ��ͼ��Դ

	D3D12_SHADER_RESOURCE_VIEW_DESC ShadowMapSRVDesc;              //��Ӱ��ͼSRV����

	//VPT����
	vector<XMFLOAT4X4> PointVPTMatList;                           //���ԴVPN�����б�
	ComPtr<ID3D12Resource> PointLightVPTListBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC PointLightVPTListSRVDesc;

//��ȾĿ�����
	vector<ComPtr<ID3D12Resource>> RTBList;
	ComPtr<ID3D12DescriptorHeap> RTVHeap;
	ComPtr<ID3D12DescriptorHeap> GBufferCSUHeap;
	ComPtr<ID3D12DescriptorHeap> DeferredRenderSamplerDescHeap;

	ComPtr<ID3D12Resource> DeferredRenderCB;

	D3D12_CPU_DESCRIPTOR_HANDLE GBufferRTVHeapHandle;

	//DEFERRED_RENDER_DATA DeferredRenderData;

	//��ȾGBuffer����
	int GBufferNum = 3;

//SSAO
	ComPtr<ID3D12Resource> HBAOResource;
	D3D12_CONSTANT_BUFFER_VIEW_DESC HBAOResourceCBViewDesc;

	D3D12_CPU_DESCRIPTOR_HANDLE SSAORTVHeapHandle;

	ComPtr<ID3D12Resource> AOBlurWeightCB;
	ComPtr<ID3D12Resource> AOBlurResource;
	ComPtr<ID3D12DescriptorHeap> AOBlurCSUHeap;


//����
	float CurrentTime;

	void ReleaseTempResource()
	{
		TempAllocatorList.clear();
		TempGeoList.clear();
	}

};

//���������Դ
struct GEOMETRY_DRAW_RESOURCE
{
	
	ComPtr<ID3D12Resource> GeoFrameResource;

	//��������
	ComPtr<ID3D12DescriptorHeap> CSUDescHeap;
	ComPtr<ID3D12DescriptorHeap> SamplerDescHeap;

	ComPtr<ID3D12Resource> Texture;

	//ʵ���Ƿ�ı�
	bool isInstanceChange = false;

	//ʵ����
	int InstanceNum;
};