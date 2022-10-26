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
//物体
	unordered_map<wstring, shared_ptr<GEOMETRY>> GeometryList;
	unordered_map<wstring, shared_ptr<GEOMETRY>> BlendGeometryList;

//阴影贴图
	int ShadowWidht = 4096;
	int ShadowHeight = 4096;

	bool c = true;

	//光源投影矩阵
	XMMATRIX PointLightViewMat;
	
	//T矩阵
	XMMATRIX TMat;

//纹理资源
	unordered_map<wstring, TEXTURE_STRUCT> TextureList;

//采样器描述
	unordered_map<string, D3D12_SAMPLER_DESC> SamplerDescList;

//根签名列表
	unordered_map<string, ComPtr<ID3D12RootSignature>> RootSigList;

//着色器列表
	struct SHADER
	{
		ComPtr<ID3DBlob> Code;
		D3D12_SHADER_BYTECODE Desc;
	};

	unordered_map<string, SHADER> VSList;                     //顶点着色器列表
	unordered_map<string, SHADER> HSList;                     //外壳着色器
	unordered_map<string, SHADER> DSList;                     //域着色器列表
	unordered_map<string, SHADER> PSList;                     //像素着色器列表
	unordered_map<string, SHADER> CSList;                     //计算着色器列表

//输入布局描述列表
	struct IADESC
	{
		vector<D3D12_INPUT_ELEMENT_DESC> ElementDesc;
		D3D12_INPUT_LAYOUT_DESC LayoutDesc;
	};
	unordered_map<string, IADESC> IADescList;

//PSO列表
	/*名字        根签名        着色器                   输入布局描述
	* PSO_0       DEFRootSig    VS_0  PS_0               P_C
	*/
	unordered_map<string, ComPtr<ID3D12PipelineState>> PSOList;

//帧资源
	vector<shared_ptr<DRAW_RESOURCE>> DrawResourceList;

//窗口处理相关
	//当前鼠标（按下）位置
	int CurrentX;
	int CurrentY;

	//鼠标移动距离
	int HorAngleMove = 0;
	int VerAngleMove = 0;
	bool isChangeAngle = false;
	mutex AngleMoveMutex;
	condition_variable AngleMoveCV;

	//视角移动速度
	float AngleSpeed = 0.001;
	mutex AngleSpeedMutex;

	//相机移动起止时间
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
	
	//移动速度
	float CameraSpeed = 10;
	mutex CameraSpeedMutex;
	
	//
	condition_variable CameraMoveCV;

	//窗口缩放
	int NewWidth;
	int NewHeight;
	float AngleSize = 0.25;                            //视角（*XM_PI）
	bool WndResize = false;
	mutex WndResizeMutex;
	condition_variable WndResizeCV;

//MSAA


//绘制相关
	mutex DrawMutex;
	condition_variable DrawCV;

	int CompleteDrawNum = 3;
	mutex CompleteDrawNumMutex;

	HANDLE DrawCompleteEvent;

//其它
	//实际最大并发线程数量
	int RealThreadNum;

public:
	D3DAPP(HWND Hwnd, UINT Width, UINT Height);

protected:
//资源
	//创建实际渲染目标缓冲区
	void RecreateRTs(shared_ptr<DRAW_RESOURCE> drawResource);

//纹理相关
	//初始化纹理资源
	void InitialTextureList();

	//添加纹理
	void AddTexture(wstring fileName, D3D12_SHADER_RESOURCE_VIEW_DESC sRVDesc);

//阴影贴图相关
	ComPtr<ID3D12Resource> InitialShadowMap(vector<SHADOW_ITEM>& shadowMapList);

//采样器
	//初始化采样器列表
	void InitialSamplerDescList();

//帧资源
	//初始化
	void InitialDrawResource();

	//更新相机数据
	void UpdateDrawResource(int index);

//根签名
	//初始化根签名列表
	void InitialRootSigList();

	//创建根签名
	ComPtr<ID3D12RootSignature> CreateDefaultRootSig();                        //创建默认根签名
	ComPtr<ID3D12RootSignature> CreateShadowMapRootSig();                      //创建阴影贴图根签名
	ComPtr<ID3D12RootSignature> CreateSSAORootSig();                           //创建SSAO(HBAO)根签名
	ComPtr<ID3D12RootSignature> CreateDeferredRenderRootSig();                      //创建延迟渲染根签名
	ComPtr<ID3D12RootSignature> CreateAOBlurSig();                            //创建AO模糊根签名

	//查找根签名
	string GetRootSigIndex(shared_ptr<GEOMETRY> geometry);
	string GetShadowMapRootSigIndex(shared_ptr<GEOMETRY> geometry);
	string GetGBufferRootSigIndex(shared_ptr<GEOMETRY> geometry);

//着色器
	//初始化着色器列表
	void InitialShaders();

	//添加着色器
	void AddShader(unordered_map<string, SHADER>& List, string Name, string Version, wstring File = L"Shaders.hlsl");

//输入布局描述
	void InitialIADesc();
	
	void AddIADesc(string Name, vector<D3D12_INPUT_ELEMENT_DESC>& ElementDesc);
	
//PSO
	//初始化PSO列表
	void InitialPSOList();

	//阴影相关PSO
	ComPtr<ID3D12PipelineState> CreateShadowMapPSO(ComPtr<ID3D12RootSignature> RootSignature, D3D12_SHADER_BYTECODE VSByteCodeDesc,
		D3D12_INPUT_LAYOUT_DESC IAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	//创建延迟渲染GBuffer流程PSO
	ComPtr<ID3D12PipelineState> CreateGBufferPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
		D3D12_INPUT_LAYOUT_DESC iAInputDesc, vector<DXGI_FORMAT> rTFormatList, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	//创建SSAO PSO
	ComPtr<ID3D12PipelineState> CreateSSAOPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
		D3D12_INPUT_LAYOUT_DESC iAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	//创建延迟渲染GBuffer流程PSO
	ComPtr<ID3D12PipelineState> CreateDeferredRenderPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
		D3D12_INPUT_LAYOUT_DESC iAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	//创建计算流水线
	ComPtr<ID3D12PipelineState> CreateComputePSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE cSByteCodeDesc);

	string GetPSOIndex(shared_ptr<GEOMETRY> geometry);
	string GetShadowMapPSOIndex(shared_ptr<GEOMETRY> geometry);
	string GetGBufferPSOIndex(shared_ptr<GEOMETRY> geometry);

//绘制
	void Draw(int drawIndex);
	void DrawGeometries(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawNum, int geoNum);
	void DrawShadowMaps(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawNum, int geoNum);
	void DrawGBuffer(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawIndex, int geoNum);

	void DrawCompleteDeal();

//描述符堆
	//创建默认CSU描述符堆
	ComPtr<ID3D12DescriptorHeap> CreateDefalutCSUDescHeap(int drawIndex);

	//创建默认采样器描述符堆
	ComPtr<ID3D12DescriptorHeap> CreateDefalutSamplerHeap(int drawIndex);

	//创建后处理描述符堆
	ComPtr<ID3D12DescriptorHeap> CreateLaterProcessDescHeap(int drawIndex);

//处理窗口过程
public:
	virtual void WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

private:
	//视角变化
	void ChangeAngle();

	//窗口大小变换
	void ResizeWnd();

	//处理移动
	void MoveCamera();

//物体
	//创建物体对象
	void CreateGeometry(wstring name, GEOMETRY_TYPE type, MESH_TYPE meshType, wstring materialName, vector<GEOMETRY_FRAME_DATA> geoFrameData = vector<GEOMETRY_FRAME_DATA>());

	//为物体赋网格
	void SetGeometryMesh(shared_ptr<GEOMETRY> geoemtry, MESH_TYPE meshType);

	//创建物体顶点、索引缓冲区
	void InsertGeometry(wstring name, shared_ptr<GEOMETRY> geometry);
	void InsertBlendGeometry(wstring name, shared_ptr<GEOMETRY> geometry);

	//创建common类型物体
	shared_ptr<GEOMETRY> CreateCommon(wstring name, wstring materialName, vector<GEOMETRY_FRAME_DATA> geoFrameData);


//运行
public:
	virtual void Run();

};

