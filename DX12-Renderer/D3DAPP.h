#pragma once

#include"D3DBASE.h"
#include"CAMERA.h"
#include"GEOMETRY.h"
#include"Time.h"
#include"LIGHT.h"
#include<memory>
#include<unordered_map>

#include<fstream>

void operator += (unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator& iter, UINT k);

class D3DAPP :virtual public D3DBASE,virtual public CAMERA, public Time,public LIGHT
{
protected:
//����
	unordered_map<wstring, shared_ptr<GEOMETRY>> GeometryList;
	unordered_map<wstring, shared_ptr<GEOMETRY>> BlendGeometryList;

//��Ӱ��ͼ
	int ShadowWidht = 4096;
	int ShadowHeight = 4096;

	bool c = true;

	//��ԴͶӰ����
	XMMATRIX PointLightViewMat;
	
	//T����
	XMMATRIX TMat;

//������Դ
	unordered_map<wstring, TEXTURE_STRUCT> TextureList;

//����������
	unordered_map<string, D3D12_SAMPLER_DESC> SamplerDescList;

//��ǩ���б�
	unordered_map<string, ComPtr<ID3D12RootSignature>> RootSigList;

//��ɫ���б�
	struct SHADER
	{
		ComPtr<ID3DBlob> Code;
		D3D12_SHADER_BYTECODE Desc;
	};

	unordered_map<string, SHADER> VSList;                     //������ɫ���б�
	unordered_map<string, SHADER> HSList;                     //�����ɫ��
	unordered_map<string, SHADER> DSList;                     //����ɫ���б�
	unordered_map<string, SHADER> PSList;                     //������ɫ���б�
	unordered_map<string, SHADER> CSList;                     //������ɫ���б�

//���벼�������б�
	struct IADESC
	{
		vector<D3D12_INPUT_ELEMENT_DESC> ElementDesc;
		D3D12_INPUT_LAYOUT_DESC LayoutDesc;
	};
	unordered_map<string, IADESC> IADescList;

//PSO�б�
	/*����        ��ǩ��        ��ɫ��                   ���벼������
	* PSO_0       DEFRootSig    VS_0  PS_0               P_C
	*/
	unordered_map<string, ComPtr<ID3D12PipelineState>> PSOList;

//֡��Դ
	vector<shared_ptr<DRAW_RESOURCE>> DrawResourceList;

//���ڴ������
	//��ǰ��꣨���£�λ��
	int CurrentX;
	int CurrentY;

	//����ƶ�����
	int HorAngleMove = 0;
	int VerAngleMove = 0;
	bool isChangeAngle = false;
	mutex AngleMoveMutex;
	condition_variable AngleMoveCV;

	//�ӽ��ƶ��ٶ�
	float AngleSpeed = 0.001;
	mutex AngleSpeedMutex;

	//����ƶ���ֹʱ��
	__int64 ForwardStartTime = 0;
	__int64 BackwardStartTime = 0;
	__int64 RightwardStartTime = 0;
	__int64 LeftwardStartTime = 0;
	__int64 UpwardStartTime = 0;
	__int64 DownwardStartTime = 0;

	__int64 ForwardEndTime = 0;
	__int64 BackwardEndTime = 0;
	__int64 RightwardEndTime = 0;
	__int64 LeftwardEndTime = 0;
	__int64 UpwardEndTime = 0;
	__int64 DownwardEndTime = 0;

	mutex ForBackMoveTimeMutex;
	mutex RightLeftMoveTimeMutex;
	mutex UpDownMoveTimeMutex;

	bool WPress = false;
	bool SPress = false;
	bool DPress = false;
	bool APress = false;
	bool EPress = false;
	bool QPress = false;
	
	//�ƶ��ٶ�
	float CameraSpeed = 10;
	mutex CameraSpeedMutex;
	
	//
	condition_variable CameraMoveCV;

	//��������
	int NewWidth;
	int NewHeight;
	float AngleSize = 0.25;                            //�ӽǣ�*XM_PI��
	bool WndResize = false;
	mutex WndResizeMutex;
	condition_variable WndResizeCV;

//MSAA


//�������
	mutex DrawMutex;
	condition_variable DrawCV;

	int CompleteDrawNum = 3;
	mutex CompleteDrawNumMutex;

	HANDLE DrawCompleteEvent;

//����
	//ʵ����󲢷��߳�����
	int RealThreadNum;

public:
	D3DAPP(HWND Hwnd, UINT Width, UINT Height);

protected:
//��Դ
	//����ʵ����ȾĿ�껺����
	void RecreateRTs(shared_ptr<DRAW_RESOURCE> drawResource);

//�������
	//��ʼ��������Դ
	void InitialTextureList();

	//�������
	void AddTexture(wstring fileName, D3D12_SHADER_RESOURCE_VIEW_DESC sRVDesc);

//��Ӱ��ͼ���
	ComPtr<ID3D12Resource> InitialShadowMap(vector<SHADOW_ITEM>& shadowMapList);

//������
	//��ʼ���������б�
	void InitialSamplerDescList();

//֡��Դ
	//��ʼ��
	void InitialDrawResource();

	//�����������
	void UpdateDrawResource(int index);

//��ǩ��
	//��ʼ����ǩ���б�
	void InitialRootSigList();

	//������ǩ��
	ComPtr<ID3D12RootSignature> CreateDefaultRootSig();                        //����Ĭ�ϸ�ǩ��
	ComPtr<ID3D12RootSignature> CreateShadowMapRootSig();                      //������Ӱ��ͼ��ǩ��
	ComPtr<ID3D12RootSignature> CreateSSAORootSig();                           //����SSAO(HBAO)��ǩ��
	ComPtr<ID3D12RootSignature> CreateDeferredRenderRootSig();                      //�����ӳ���Ⱦ��ǩ��
	ComPtr<ID3D12RootSignature> CreateAOBlurSig();                            //����AOģ����ǩ��

	//���Ҹ�ǩ��
	string GetRootSigIndex(shared_ptr<GEOMETRY> geometry);
	string GetShadowMapRootSigIndex(shared_ptr<GEOMETRY> geometry);
	string GetGBufferRootSigIndex(shared_ptr<GEOMETRY> geometry);

//��ɫ��
	//��ʼ����ɫ���б�
	void InitialShaders();

	//�����ɫ��
	void AddShader(unordered_map<string, SHADER>& List, string Name, string Version, wstring File = L"Shaders.hlsl");

//���벼������
	void InitialIADesc();
	
	void AddIADesc(string Name, vector<D3D12_INPUT_ELEMENT_DESC>& ElementDesc);
	
//PSO
	//��ʼ��PSO�б�
	void InitialPSOList();

	//��Ӱ���PSO
	ComPtr<ID3D12PipelineState> CreateShadowMapPSO(ComPtr<ID3D12RootSignature> RootSignature, D3D12_SHADER_BYTECODE VSByteCodeDesc,
		D3D12_INPUT_LAYOUT_DESC IAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	//�����ӳ���ȾGBuffer����PSO
	ComPtr<ID3D12PipelineState> CreateGBufferPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
		D3D12_INPUT_LAYOUT_DESC iAInputDesc, vector<DXGI_FORMAT> rTFormatList, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	//����SSAO PSO
	ComPtr<ID3D12PipelineState> CreateSSAOPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
		D3D12_INPUT_LAYOUT_DESC iAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	//�����ӳ���ȾGBuffer����PSO
	ComPtr<ID3D12PipelineState> CreateDeferredRenderPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
		D3D12_INPUT_LAYOUT_DESC iAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	//����������ˮ��
	ComPtr<ID3D12PipelineState> CreateComputePSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE cSByteCodeDesc);

	string GetPSOIndex(shared_ptr<GEOMETRY> geometry);
	string GetShadowMapPSOIndex(shared_ptr<GEOMETRY> geometry);
	string GetGBufferPSOIndex(shared_ptr<GEOMETRY> geometry);

//����
	void Draw(int drawIndex);
	void DrawGeometries(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawNum, int geoNum);
	void DrawShadowMaps(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawNum, int geoNum);
	void DrawGBuffer(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawIndex, int geoNum);

	void DrawCompleteDeal();

//��������
	//����Ĭ��CSU��������
	ComPtr<ID3D12DescriptorHeap> CreateDefalutCSUDescHeap(int drawIndex);

	//����Ĭ�ϲ�������������
	ComPtr<ID3D12DescriptorHeap> CreateDefalutSamplerHeap(int drawIndex);

	//����������������
	ComPtr<ID3D12DescriptorHeap> CreateLaterProcessDescHeap(int drawIndex);

//�����ڹ���
public:
	virtual void WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

private:
	//�ӽǱ仯
	void ChangeAngle();

	//���ڴ�С�任
	void ResizeWnd();

	//�����ƶ�
	void MoveCamera();

//����
	//�����������
	void CreateGeometry(wstring name, GEOMETRY_TYPE type, MESH_TYPE meshType, wstring materialName, vector<GEOMETRY_FRAME_DATA> geoFrameData = vector<GEOMETRY_FRAME_DATA>());

	//Ϊ���帳����
	void SetGeometryMesh(shared_ptr<GEOMETRY> geoemtry, MESH_TYPE meshType);

	//�������嶥�㡢����������
	void InsertGeometry(wstring name, shared_ptr<GEOMETRY> geometry);
	void InsertBlendGeometry(wstring name, shared_ptr<GEOMETRY> geometry);

	//����common��������
	shared_ptr<GEOMETRY> CreateCommon(wstring name, wstring materialName, vector<GEOMETRY_FRAME_DATA> geoFrameData);


//����
public:
	virtual void Run();

};

