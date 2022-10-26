#pragma once

#include"D3DAPP.h"

D3DAPP::D3DAPP(HWND Hwnd, UINT Width, UINT Height) : D3DBASE(Hwnd, Width, Height), NewWidth(Width), NewHeight(Height)
{
	//初始化最大线程数量
	RealThreadNum = thread::hardware_concurrency() ? thread::hardware_concurrency() : 4;

	//初始化帧队列同步事件
	DrawCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

#ifndef NDEBUG
	cout << "最大并行线程数量：" << RealThreadNum << endl;
#endif

	PointLightViewMat = CalcuProjMat(0.75, ShadowWidht, ShadowHeight);
	XMFLOAT4X4 TMatData = { 0.5f,0.f,0.f,0,0,-0.5f,0,0,0,0,1.f,0,0.5f,0.5f,0,1.f };
	TMat = XMLoadFloat4x4(&TMatData);

	InitialRootSigList();
	InitialIADesc();
	InitialShaders();
	InitialTextureList();
	InitialSamplerDescList();
	InitialPSOList();
	

	InitialDrawResource();

	//启动相机视角改变线程
	thread th1(&D3DAPP::ChangeAngle, this);
	th1.detach();

	//启动帧队列同步进程
	thread th2(&D3DAPP::DrawCompleteDeal, this);
	th2.detach();

	Sleep(100);

	wstring Texture1 = L"Texture1.dds";
	wstring Texture2 = L"Texture2.dds";

	PBR_ITEM MetalPBR({ 0.8,0.8,0.8 }, 0.3f, 0.9f);

	CreateGeometry(L"立方体", common, cube0, Texture1);
	GeometryList.find(L"立方体")->second->PushInstance({ 0, 0,0 }, { 0,0,0 }, { 1,1,1 }, MetalPBR);

	CreateGeometry(L"地板", common, flat0, Texture2);
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			GeometryList.find(L"地板")->second->PushInstance({ float(-2 + 2*i), -3, float(-2 + 2*j) }, { 0,0,0 }, { 1,1,1 }, MetalPBR);
		}
	}
	GeometryList.find(L"地板")->second->PushInstance({ -3,0,0 }, { 0,0,-XM_PI / 2 }, { 3,3,3 }, MetalPBR);
	GeometryList.find(L"地板")->second->PushInstance({ 0,0,-3 }, { XM_PI / 2,0,0 }, { 3,3,3 }, MetalPBR);
}

void D3DAPP::InitialTextureList()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;                       //SRV的描述

	//添加"Texture1.dds"
	{
		D3D12_TEX2D_SRV TexSrv;
		TexSrv.MipLevels = -1;                                         //mipmap层级，-1表示所有
		TexSrv.MostDetailedMip = 0;                                    //细节最高的层级
		TexSrv.ResourceMinLODClamp = 0.0;                              //可访问的最小层级，0.0表示全部可访问
		TexSrv.PlaneSlice = 0;

		SRVDesc.Format = DXGI_FORMAT_BC3_UNORM;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;         //纹理维度：2D
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //不对纹理向量分量重新排序
		SRVDesc.Texture2D = TexSrv;
	}
	AddTexture(L"Texture1.dds",SRVDesc);

	//添加"Texture2.dds"
	{
		D3D12_TEX2D_SRV TexSrv;
		TexSrv.MipLevels = -1;                                         //mipmap层级，-1表示所有
		TexSrv.MostDetailedMip = 0;                                    //细节最高的层级
		TexSrv.ResourceMinLODClamp = 0.0;                              //可访问的最小层级，0.0表示全部可访问
		TexSrv.PlaneSlice = 0;

		SRVDesc.Format = DXGI_FORMAT_BC3_UNORM;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;         //纹理维度：2D
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //不对纹理向量分量重新排序
		SRVDesc.Texture2D = TexSrv;
	}
	AddTexture(L"Texture2.dds", SRVDesc);

	//添加"Texture3.dds"
	{
		D3D12_TEX2D_SRV TexSrv;
		TexSrv.MipLevels = -1;                                         //mipmap层级，-1表示所有
		TexSrv.MostDetailedMip = 0;                                    //细节最高的层级
		TexSrv.ResourceMinLODClamp = 0.0;                              //可访问的最小层级，0.0表示全部可访问
		TexSrv.PlaneSlice = 0;

		SRVDesc.Format = DXGI_FORMAT_BC3_UNORM;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;         //纹理维度：2D
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //不对纹理向量分量重新排序
		SRVDesc.Texture2D = TexSrv;
	}
	AddTexture(L"Texture3.dds", SRVDesc);

}

void D3DAPP::AddTexture(wstring fileName, D3D12_SHADER_RESOURCE_VIEW_DESC sRVDesc)
{
	wcout.imbue(locale("", LC_CTYPE));
	if (TextureList.find(fileName) != TextureList.end())
	{
#ifndef NDEBUG
		wcout << L"纹理添加失败：名称“" << fileName << L"”已存在" << endl;
#endif
	}
	else
	{
		ComPtr<ID3D12Resource> Texture = UploadTexture(fileName);
		TextureList.insert({ fileName,{Texture,sRVDesc} });
#ifndef NDEBUG
		wcout << L"纹理“" << fileName << L"”添加成功" << endl;
#endif
	}
}

void D3DAPP::InitialSamplerDescList()
{
	D3D12_SAMPLER_DESC SamplerDesc = {};

	//默认采样器
	{
		SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;             //过滤方式：使用线性插值进行缩小、放大和 mip 级采样
		SamplerDesc.AddressU = SamplerDesc.AddressV =
			SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;       //在U、V、W（3D纹理）上的寻址模式：在uv整数连接处平铺处理
		SamplerDesc.MipLODBias = 0;                                       //纹理查询偏移，即按层级+MipLODBias进行采样
		SamplerDesc.MaxAnisotropy = 1;                                    //最大各项异性值，特殊情况才生效
		SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;        //如果源数据小于或等于目标数据，则比较通过
		//SamplerDesc.BorderColor                                         //如果UVW指定D3D12_TEXTURE_ADDRESS_MODE_BORDER，则用这个
		SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                           //最大mipmap层级
		SamplerDesc.MinLOD = 0;                                           //最小mipmap层级
	}
	SamplerDescList.insert({ "DEFSampler",SamplerDesc });

	//ShadowMap PCF
	{
		SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		SamplerDesc.AddressU = SamplerDesc.AddressV =
			SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;       //在U、V、W（3D纹理）上的寻址模式：0-1范围外纹理坐标设置为指定边框颜色
		SamplerDesc.MipLODBias = 0;                                       //纹理查询偏移，即按层级+MipLODBias进行采样
		SamplerDesc.MaxAnisotropy = 1;                                    //最大各项异性值，特殊情况才生效
		SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;        //指定将采样数据与现有采样数据进行比较的函数，用于实现阴影贴图等效果
		//SamplerDesc.BorderColor                                         //如果UVW指定D3D12_TEXTURE_ADDRESS_MODE_BORDER，则用这个
		SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                           //最大mipmap层级
		SamplerDesc.MinLOD = 0;                                           //最小mipmap层级
	}
	SamplerDescList.insert({ "ShadowSampler",SamplerDesc });

	{
		SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;             //过滤方式：使用线性插值进行缩小、放大和 mip 级采样
		SamplerDesc.AddressU = SamplerDesc.AddressV =
			SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;       //在U、V、W（3D纹理）上的寻址模式：在uv整数连接处平铺处理
		SamplerDesc.MipLODBias = 0;                                       //纹理查询偏移，即按层级+MipLODBias进行采样
		SamplerDesc.MaxAnisotropy = 1;                                    //最大各项异性值，特殊情况才生效
		SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;        //如果源数据小于或等于目标数据，则比较通过
		//SamplerDesc.BorderColor                                         //如果UVW指定D3D12_TEXTURE_ADDRESS_MODE_BORDER，则用这个
		SamplerDesc.MaxLOD = 0;                                            //最大mipmap层级
		SamplerDesc.MinLOD = 0;                                           //最小mipmap层级
	}
	SamplerDescList.insert({ "SSAOSampler",SamplerDesc });
}

//资源管理部分-----------------------------------------------------------------------------------------------

void D3DAPP::InitialDrawResource()
{
//AO Blur权重资源Gaussian
	//计算高斯权重
	vector<XMFLOAT4> GaussWeight;
	float sum = 0;
	for (int i = -3; i <= 3; i++)
	{
		XMFLOAT4 Weight;
		Weight.x = Gaussian(i);
		GaussWeight.push_back(Weight);
		sum += Weight.x;
	}
	for (int i = 0; i < 7; i++)
	{
		GaussWeight[i].x /= sum;
		//cout << GaussWeight[i] << endl;
	}

	//创建常量缓冲区
	ComPtr<ID3D12Resource> GaussWeightCB;
	auto GaussWeightCBVDesc = CreateConstantBuffer(GaussWeightCB, sizeof(XMFLOAT4) * GaussWeight.size());
	UpdateUB(GaussWeight.data(), sizeof(XMFLOAT4) * GaussWeight.size(), GaussWeightCB);

//初始化三个DrawResource
	for (int i = 0; i < 3; i++)
	{
		auto DrawResource = make_shared<DRAW_RESOURCE>();

	//创建一般帧资源常量缓冲区
		DrawResource->FrameBufferViewDesc = CreateConstantBuffer(DrawResource->FrameBuffer, sizeof(FRAME_DATA));

	//创建环境光和平行光帧资源常量缓冲区
		DrawResource->AmbParaLightBufferViewDesc = CreateConstantBuffer(DrawResource->AmbParaLightBuffer, sizeof(LIGHT_FRAME_DATA));

	//创建点光源SR资源及其描述符的描述
		//创建点光源缓冲区
		DrawResource->PointLightBuffer = CreateUploadBuffer(sizeof(POINT_LIGHT_FRAME_DATA) * GetPointLightNum());
		
		//点光源SRV描述
		D3D12_BUFFER_SRV BS;
		BS.FirstElement = 0;
		BS.NumElements = GetPointLightNum();
		BS.StructureByteStride = sizeof(POINT_LIGHT_FRAME_DATA);
		BS.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		DrawResource->PointLightSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		DrawResource->PointLightSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		DrawResource->PointLightSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //不对纹理向量分量重新排序
		DrawResource->PointLightSRVDesc.Buffer = BS;
		
	//初始化阴影贴图相关
		//创建阴影贴图资源和DSV
		DrawResource->ShadowMapBufferArray = InitialShadowMap(DrawResource->ShadowItemList);

		//创建阴影贴图资源的SRV描述
		DrawResource->ShadowMapSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		DrawResource->ShadowMapSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		DrawResource->ShadowMapSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		DrawResource->ShadowMapSRVDesc.Texture2DArray.MostDetailedMip = 0;
		DrawResource->ShadowMapSRVDesc.Texture2DArray.MipLevels = 1;
		DrawResource->ShadowMapSRVDesc.Texture2DArray.FirstArraySlice = 0;
		DrawResource->ShadowMapSRVDesc.Texture2DArray.ArraySize = GetPointLightNum();
		DrawResource->ShadowMapSRVDesc.Texture2DArray.PlaneSlice = 0;
		DrawResource->ShadowMapSRVDesc.Texture2DArray.ResourceMinLODClamp = 0.f;

		//创建光源VPT矩阵资源
		DrawResource->PointLightVPTListBuffer = CreateUploadBuffer(sizeof(XMFLOAT4X4) * GetPointLightNum());

		//点光源VPT矩阵资源SRV描述
		D3D12_BUFFER_SRV BS2;
		BS2.FirstElement = 0;
		BS2.NumElements = GetPointLightNum();
		BS2.StructureByteStride = sizeof(XMFLOAT4X4);
		BS2.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		DrawResource->PointLightVPTListSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		DrawResource->PointLightVPTListSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		DrawResource->PointLightVPTListSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //不对纹理向量分量重新排序
		DrawResource->PointLightVPTListSRVDesc.Buffer = BS2;

	//SSAO相关
		//创建HBAO计算资源，为默认堆常量缓冲区
		DrawResource->HBAOResourceCBViewDesc = CreateConstantBuffer(DrawResource->HBAOResource, sizeof(SSAO_RESOURCE)/*, D3D12_HEAP_TYPE_DEFAULT*/);

		//获得采样向量
		SSAO_RESOURCE SSAOResource;
		{
			// 8个立方体角点向量
			SSAOResource.NormalList[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
			SSAOResource.NormalList[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

			SSAOResource.NormalList[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
			SSAOResource.NormalList[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

			SSAOResource.NormalList[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
			SSAOResource.NormalList[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

			SSAOResource.NormalList[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
			SSAOResource.NormalList[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

			// 6个面中心点向量
			SSAOResource.NormalList[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
			SSAOResource.NormalList[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

			SSAOResource.NormalList[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
			SSAOResource.NormalList[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

			SSAOResource.NormalList[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
			SSAOResource.NormalList[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);
		}
		
		//UpdateDB(DrawResource->HBAOResource, &SSAOResource, sizeof(SSAOResource), D3D12_RESOURCE_STATE_GENERIC_READ);
		UpdateUB(&SSAOResource, sizeof(SSAOResource),DrawResource->HBAOResource);

	//AO图模糊
		DrawResource->AOBlurWeightCB = GaussWeightCB;

		DrawResource->AOBlurCSUHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 10);

		//创建描述符堆
		CD3DX12_CPU_DESCRIPTOR_HANDLE AOBlurHeapHandle(DrawResource->AOBlurCSUHeap->GetCPUDescriptorHandleForHeapStart());

		//创建高斯权重CBV
		Device->CreateConstantBufferView(&GaussWeightCBVDesc, AOBlurHeapHandle);

		//创建模糊运算资源CBV
		AOBlurHeapHandle.Offset(1, CSUSize);
		auto CBVDesc = CreateConstantBuffer(DrawResource->AOBlurResource, sizeof(AO_BUFFER_BLUR_DATA));
		Device->CreateConstantBufferView(&CBVDesc, AOBlurHeapHandle);

	//初始化实际渲染目标缓冲区和GBuffer
		//创建描述符堆
		DrawResource->RTVHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 8);
		DrawResource->GBufferCSUHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 20);

		//创建延迟渲染帧资源常量缓冲区
		auto DeferredRenderCBVDesc = CreateConstantBuffer(DrawResource->DeferredRenderCB, sizeof(DEFERRED_RENDER_DATA));

		//初始化延迟渲染描述符堆
		{
			//描述符堆句柄
			CD3DX12_CPU_DESCRIPTOR_HANDLE HeapHandle(DrawResource->GBufferCSUHeap->GetCPUDescriptorHandleForHeapStart());

			//描述符1：全局帧资源缓冲区描述符
			auto HeapHandle1 = HeapHandle;
			HeapHandle1.Offset(1, CSUSize);
			Device->CreateConstantBufferView(&DrawResource->FrameBufferViewDesc, HeapHandle1);

			//描述符2：
			auto HeapHandle2 = HeapHandle;
			HeapHandle2.Offset(2, CSUSize);
			Device->CreateConstantBufferView(&DrawResource->AmbParaLightBufferViewDesc, HeapHandle2);

			//描述符4：
			auto HeapHandle4 = HeapHandle;
			HeapHandle4.Offset(3, CSUSize);
			Device->CreateConstantBufferView(&DrawResource->HBAOResourceCBViewDesc, HeapHandle4);

			//描述符5：
			auto HeapHandle5 = HeapHandle;
			HeapHandle5.Offset(5, CSUSize);
			Device->CreateShaderResourceView(DrawResource->PointLightBuffer.Get(), &DrawResource->PointLightSRVDesc, HeapHandle5);

			//描述符8：
			auto HeapHandle8 = HeapHandle;
			HeapHandle8.Offset(8, CSUSize);
			Device->CreateShaderResourceView(DrawResource->ShadowMapBufferArray.Get(), &DrawResource->ShadowMapSRVDesc, HeapHandle8);

			//描述符9：
			auto HeapHandle9 = HeapHandle;
			HeapHandle9.Offset(9, CSUSize);
			Device->CreateShaderResourceView(DrawResource->PointLightVPTListBuffer.Get(), &DrawResource->PointLightVPTListSRVDesc, HeapHandle9);

			//描述符10：
			auto HeapHandle10 = HeapHandle;
			HeapHandle10.Offset(10, CSUSize);
			Device->CreateConstantBufferView(&DeferredRenderCBVDesc, HeapHandle10);
		}

		RecreateRTs(DrawResource);

		DrawResource->DeferredRenderSamplerDescHeap = CreateDefalutSamplerHeap(i);

		DrawResource->PointVPTMatList = vector<XMFLOAT4X4>(GetPointLightNum());


	//将绘制资源加入列表
		DrawResourceList.push_back(DrawResource);
	}
}

ComPtr<ID3D12Resource> D3DAPP::InitialShadowMap(vector<SHADOW_ITEM>& shadowItemList)
{
	//点光源数量
	int Num = GetPointLightNum();

//创建资源
	//描述dsbuffer大资源数组
	D3D12_RESOURCE_DESC ShadowMapDesc;
	ShadowMapDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ShadowMapDesc.Alignment = 0;
	ShadowMapDesc.Width = ShadowWidht;
	ShadowMapDesc.Height = ShadowHeight;
	ShadowMapDesc.DepthOrArraySize = Num;
	ShadowMapDesc.MipLevels = 1;
	ShadowMapDesc.Format = DSFormat;
	ShadowMapDesc.SampleDesc = { 1,0 };
	ShadowMapDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ShadowMapDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//描述资源清除方式
	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ClearValue.DepthStencil.Depth = 1.f;
	ClearValue.DepthStencil.Stencil = 0;

	//创建资源
	ComPtr<ID3D12Resource> ShadowMapResource;
	auto HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(Device->CreateCommittedResource(&HeapProperties,
		D3D12_HEAP_FLAG_NONE, &ShadowMapDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		&ClearValue, IID_PPV_ARGS(ShadowMapResource.GetAddressOf())));
	
//创建描述符和VP矩阵常量缓冲区
	for (int i = 0; i < Num; i++)
	{
		//描述具体dsbuffer在资源中位置
		D3D12_DEPTH_STENCIL_VIEW_DESC DSViewDesc;
		DSViewDesc.Format = DSFormat;
		DSViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		DSViewDesc.Flags = D3D12_DSV_FLAG_NONE;
		DSViewDesc.Texture2DArray.MipSlice = 0;
		DSViewDesc.Texture2DArray.FirstArraySlice = i;
		DSViewDesc.Texture2DArray.ArraySize = 1;

		//创建描述符和描述符堆
		ComPtr<ID3D12DescriptorHeap> ShadowMapDescHeap;
		ShadowMapDescHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
		Device->CreateDepthStencilView(ShadowMapResource.Get(), &DSViewDesc, ShadowMapDescHeap->GetCPUDescriptorHandleForHeapStart());

		//将描述符加入列表
		shadowItemList.push_back(SHADOW_ITEM());
		shadowItemList[i].DescHeap = ShadowMapDescHeap;

		//创建VP常量缓冲区
		CreateConstantBuffer(shadowItemList[i].VPMatBuffer, sizeof(XMFLOAT4X4));
	}

	return ShadowMapResource;
}

void D3DAPP::RecreateRTs(shared_ptr<DRAW_RESOURCE> drawResource)
{
//释放原资源
	drawResource->RTBList.clear();
	//for (auto iter = rTBList.begin(); iter != rTBList.end(); iter++)
	//{
	//	iter->Reset();
	//}

//获得描述符堆句柄
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(drawResource->RTVHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE CSUHeapHanlde(drawResource->GBufferCSUHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE AOBlurCSUHeapHandle(drawResource->AOBlurCSUHeap->GetCPUDescriptorHandleForHeapStart());
	CSUHeapHanlde.Offset(15, CSUSize);

//所有资源都是默认堆
	CD3DX12_HEAP_PROPERTIES RTBProperties(D3D12_HEAP_TYPE_DEFAULT);

//创建新RealRT
	D3D12_RESOURCE_DESC RTDesc;
	RTDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	RTDesc.Alignment = 0;
	RTDesc.Width = ClientWidth;
	RTDesc.Height = ClientHeight;
	RTDesc.DepthOrArraySize = 1;
	RTDesc.MipLevels = 1;
	RTDesc.Format = RTFormat;
	RTDesc.SampleDesc = { 1,0 };
	RTDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	RTDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;       //允许为资源创建RTV和UAV

	D3D12_CLEAR_VALUE RealRTClearValue;
	RealRTClearValue.Format = RTFormat;
	float ClearColor[4] = { 0.690196097f, 0.768627524f, 0.870588303f, 1.000000000f };
	RealRTClearValue.Color[0] = ClearColor[0];
	RealRTClearValue.Color[1] = ClearColor[1];
	RealRTClearValue.Color[2] = ClearColor[2];
	RealRTClearValue.Color[3] = ClearColor[3];

	ComPtr<ID3D12Resource> RealRTB;
	ThrowIfFailed(Device->CreateCommittedResource(&RTBProperties, D3D12_HEAP_FLAG_NONE, &RTDesc, D3D12_RESOURCE_STATE_COMMON, &RealRTClearValue, IID_PPV_ARGS(RealRTB.GetAddressOf())));
	RealRTB->SetName(L"RealRTB");
	drawResource->RTBList.push_back(RealRTB);

    //创建描述符
	Device->CreateRenderTargetView(drawResource->RTBList[0].Get(),nullptr,RTVHeapHandle);

//GBuffer资源清除值
	D3D12_CLEAR_VALUE GBufferClearValue;
	GBufferClearValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	GBufferClearValue.Color[0] = 0;
	GBufferClearValue.Color[1] = 0;
	GBufferClearValue.Color[2] = 0;
	GBufferClearValue.Color[3] = 0;

//创建第一个GBuffer，用来Albedo、粗糙度Roughness
	//描述资源并创建
	D3D12_RESOURCE_DESC GBuffer0Desc;
	GBuffer0Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	GBuffer0Desc.Alignment = 0;
	GBuffer0Desc.Width = ClientWidth;
	GBuffer0Desc.Height = ClientHeight;
	GBuffer0Desc.DepthOrArraySize = 1;
	GBuffer0Desc.MipLevels = 1;
	GBuffer0Desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	GBuffer0Desc.SampleDesc = { 1,0 };
	GBuffer0Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	GBuffer0Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;       //允许为资源创建RTV和UAV

	ComPtr<ID3D12Resource> GBuffer0;
	ThrowIfFailed(Device->CreateCommittedResource(&RTBProperties, D3D12_HEAP_FLAG_NONE, &GBuffer0Desc, D3D12_RESOURCE_STATE_COMMON, &GBufferClearValue, IID_PPV_ARGS(GBuffer0.GetAddressOf())));
	GBuffer0->SetName(L"GBuffer0");
	drawResource->RTBList.push_back(GBuffer0);

	//创建RTV
	RTVHeapHandle.Offset(1, RTVSize);
	Device->CreateRenderTargetView(drawResource->RTBList[1].Get(), nullptr, RTVHeapHandle);

	drawResource->GBufferRTVHeapHandle = RTVHeapHandle;

	//创建资源的SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC GBuffer0SRVDesc;
	GBuffer0SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	GBuffer0SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	GBuffer0SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	GBuffer0SRVDesc.Texture2D.MostDetailedMip = 0;
	GBuffer0SRVDesc.Texture2D.MipLevels = 1;
	GBuffer0SRVDesc.Texture2D.PlaneSlice = 0;
	GBuffer0SRVDesc.Texture2D.ResourceMinLODClamp = 0;

	Device->CreateShaderResourceView(drawResource->RTBList[1].Get(), &GBuffer0SRVDesc, CSUHeapHanlde);
	CSUHeapHanlde.Offset(1, CSUSize);

//创建第二个GBuffer，用来放法向量Normal、金属度Metallicity
	//描述资源并创建
	D3D12_RESOURCE_DESC GBuffer1Desc;
	GBuffer1Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	GBuffer1Desc.Alignment = 0;
	GBuffer1Desc.Width = ClientWidth;
	GBuffer1Desc.Height = ClientHeight;
	GBuffer1Desc.DepthOrArraySize = 1;
	GBuffer1Desc.MipLevels = 1;
	GBuffer1Desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	GBuffer1Desc.SampleDesc = { 1,0 };
	GBuffer1Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	GBuffer1Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;       //允许为资源创建RTV和UAV

	ComPtr<ID3D12Resource> GBuffer1;
	ThrowIfFailed(Device->CreateCommittedResource(&RTBProperties, D3D12_HEAP_FLAG_NONE, &GBuffer1Desc, D3D12_RESOURCE_STATE_COMMON, &GBufferClearValue, IID_PPV_ARGS(GBuffer1.GetAddressOf())));
	GBuffer1->SetName(L"GBuffer1");
	drawResource->RTBList.push_back(GBuffer1);

	//创建RTV
	RTVHeapHandle.Offset(1, RTVSize);
	Device->CreateRenderTargetView(drawResource->RTBList[2].Get(), nullptr, RTVHeapHandle);

	//创建资源的SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC GBuffer1SRVDesc;
	GBuffer1SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	GBuffer1SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	GBuffer1SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	GBuffer1SRVDesc.Texture2D.MostDetailedMip = 0;
	GBuffer1SRVDesc.Texture2D.MipLevels = 1;
	GBuffer1SRVDesc.Texture2D.PlaneSlice = 0;
	GBuffer1SRVDesc.Texture2D.ResourceMinLODClamp = 0;

	Device->CreateShaderResourceView(drawResource->RTBList[2].Get(), &GBuffer1SRVDesc, CSUHeapHanlde);
	CSUHeapHanlde.Offset(1, CSUSize);

//创建第三个GBuffer，用来放菲涅尔系数F0
	//描述资源并创建
	D3D12_RESOURCE_DESC GBuffer2Desc;
	GBuffer2Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	GBuffer2Desc.Alignment = 0;
	GBuffer2Desc.Width = ClientWidth;
	GBuffer2Desc.Height = ClientHeight;
	GBuffer2Desc.DepthOrArraySize = 1;
	GBuffer2Desc.MipLevels = 1;
	GBuffer2Desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	GBuffer2Desc.SampleDesc = { 1,0 };
	GBuffer2Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	GBuffer2Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;       //允许为资源创建RTV和UAV

	ComPtr<ID3D12Resource> GBuffer2;
	ThrowIfFailed(Device->CreateCommittedResource(&RTBProperties, D3D12_HEAP_FLAG_NONE, &GBuffer2Desc, D3D12_RESOURCE_STATE_COMMON, &GBufferClearValue, IID_PPV_ARGS(GBuffer2.GetAddressOf())));
	GBuffer2->SetName(L"GBuffer2");
	drawResource->RTBList.push_back(GBuffer2);

	//创建RTV
	RTVHeapHandle.Offset(1, RTVSize);
	Device->CreateRenderTargetView(drawResource->RTBList[3].Get(), nullptr, RTVHeapHandle);

	//创建资源的SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC GBuffer2SRVDesc;
	GBuffer2SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	GBuffer2SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	GBuffer2SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	GBuffer2SRVDesc.Texture2D.MostDetailedMip = 0;
	GBuffer2SRVDesc.Texture2D.MipLevels = 1;
	GBuffer2SRVDesc.Texture2D.PlaneSlice = 0;
	GBuffer2SRVDesc.Texture2D.ResourceMinLODClamp = 0;

	Device->CreateShaderResourceView(drawResource->RTBList[3].Get(), &GBuffer2SRVDesc, CSUHeapHanlde);
	CSUHeapHanlde.Offset(1, CSUSize);

//创建第四个GBuffer，即深度模板缓冲区的SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC GBuffer3SRVDesc;
	GBuffer3SRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	GBuffer3SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	GBuffer3SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	GBuffer3SRVDesc.Texture2D.MostDetailedMip = 0;
	GBuffer3SRVDesc.Texture2D.MipLevels = 1;
	GBuffer3SRVDesc.Texture2D.PlaneSlice = 0;
	GBuffer3SRVDesc.Texture2D.ResourceMinLODClamp = 0;

	Device->CreateShaderResourceView(DSBuffer.Get(), &GBuffer3SRVDesc, CSUHeapHanlde);
	CSUHeapHanlde.Offset(1, CSUSize);

//创建第五个GBuffer，为A0
	D3D12_CLEAR_VALUE AOGBufferClearValue;
	AOGBufferClearValue.Format = DXGI_FORMAT_R32G32_FLOAT;
	AOGBufferClearValue.Color[0] = 0;
	AOGBufferClearValue.Color[1] = 0;
	AOGBufferClearValue.Color[2] = 0;
	AOGBufferClearValue.Color[3] = 0;
	
	//描述资源并创建
	D3D12_RESOURCE_DESC GBuffer4Desc;
	GBuffer4Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	GBuffer4Desc.Alignment = 0;
	GBuffer4Desc.Width = ClientWidth;
	GBuffer4Desc.Height = ClientHeight;
	GBuffer4Desc.DepthOrArraySize = 1;
	GBuffer4Desc.MipLevels = 1;
	GBuffer4Desc.Format = DXGI_FORMAT_R32G32_FLOAT;
	GBuffer4Desc.SampleDesc = { 1,0 };
	GBuffer4Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	GBuffer4Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;       //允许为资源创建RTV和UAV

	ComPtr<ID3D12Resource> GBuffer4;
	ThrowIfFailed(Device->CreateCommittedResource(&RTBProperties, D3D12_HEAP_FLAG_NONE, &GBuffer4Desc, D3D12_RESOURCE_STATE_COMMON, &AOGBufferClearValue, IID_PPV_ARGS(GBuffer4.GetAddressOf())));
	GBuffer4->SetName(L"GBuffer4");
	drawResource->RTBList.push_back(GBuffer4);

	//创建RTV
	RTVHeapHandle.Offset(1, RTVSize);
	Device->CreateRenderTargetView(drawResource->RTBList[4].Get(), nullptr, RTVHeapHandle);
	drawResource->SSAORTVHeapHandle = RTVHeapHandle;

	//创建资源的SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC GBuffer4SRVDesc;
	GBuffer4SRVDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
	GBuffer4SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	GBuffer4SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	GBuffer4SRVDesc.Texture2D.MostDetailedMip = 0;
	GBuffer4SRVDesc.Texture2D.MipLevels = 1;
	GBuffer4SRVDesc.Texture2D.PlaneSlice = 0;
	GBuffer4SRVDesc.Texture2D.ResourceMinLODClamp = 0;

	Device->CreateShaderResourceView(drawResource->RTBList[4].Get(), &GBuffer4SRVDesc, CSUHeapHanlde);

	//创建UAV
	auto AOBlurCSUHeapHandle9 = AOBlurCSUHeapHandle;
	AOBlurCSUHeapHandle9.Offset(9, CSUSize);
	
	D3D12_UNORDERED_ACCESS_VIEW_DESC AOBufferUAV;
	AOBufferUAV.Format = DXGI_FORMAT_R32G32_FLOAT;
	AOBufferUAV.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	AOBufferUAV.Texture2D.MipSlice = 0;
	AOBufferUAV.Texture2D.PlaneSlice = 0;

	Device->CreateUnorderedAccessView(drawResource->RTBList[4].Get(), nullptr, &AOBufferUAV, AOBlurCSUHeapHandle9);
	
}

void D3DAPP::UpdateDrawResource(int index)
{
	//相机位置
	MoveCamera();

	FRAME_DATA FrameData;

//更新全局帧资源
	//更新投影变换矩阵
	auto ProjMat = CalcuProjMat(AngleSize, ClientWidth, ClientHeight);
	
	//更新相机位置和摄像机空间变换矩阵
	auto ViewMat = CameraFrameSource(FrameData.CameraPosition);

	//更新VP矩阵
	FrameData.VPMat = LoadMat(ViewMat * ProjMat);
	DrawResourceList[index]->VPMat = FrameData.VPMat;

	//更新时间
	auto Time = GetRunTime();
	DrawResourceList[index]->CurrentTime = Time;
	FrameData.Time = Time;

	UpdateUB(&FrameData, sizeof(FRAME_DATA), DrawResourceList[index]->FrameBuffer);

//更新光照帧资源
	//更新环境光和平行光
	LIGHT_FRAME_DATA LightFrameData = GetLightResource();
	UpdateUB(&LightFrameData, sizeof(LIGHT_FRAME_DATA), DrawResourceList[index]->AmbParaLightBuffer);

	//更新点光源
	UpdatePointLightResource(DrawResourceList[index]->PointLightBuffer, Time);

//更新阴影相关
	//auto PointLightRotateMat = XMMatrixRotationY(Time);
	for (int i = 0; i < GetPointLightNum(); i++)
	{
		//计算点光源当前置位
		auto PointLightPos = GetPointLightPosition(i);
		auto LightPositionMat = XMLoadFloat3(&PointLightPos);
		//LightPositionMat = XMVector3TransformCoord(LightPositionMat, PointLightRotateMat);

		//计算观察——投影变换矩阵
		auto ViewMat = XMMatrixLookAtLH(LightPositionMat, XMVectorSet(0, 0, 0, 1), XMVectorSet(0, 1, 0, 0));

		DrawResourceList[index]->ShadowItemList[i].PVMat = ViewMat * PointLightViewMat;
	}

//更新延迟渲染绘制数据
	DEFERRED_RENDER_DATA DeferredRenderData;
	DeferredRenderData.InverseViewMat = LoadMat(XMMatrixInverse(nullptr, ViewMat));
	DeferredRenderData.ClientHeight = ClientHeight;
	DeferredRenderData.ClientWidth = ClientWidth;
	DeferredRenderData.A = float(1000) / float(1000 - 1);
	DeferredRenderData.B = -1 * DeferredRenderData.A;
	DeferredRenderData.TanHalfFov = tan(AngleSize * XM_PI / 2);

	UpdateUB(&DeferredRenderData, sizeof(DEFERRED_RENDER_DATA), DrawResourceList[index]->DeferredRenderCB);

//更新AO模糊计算资源
	AO_BUFFER_BLUR_DATA AOBlurData = { ClientWidth,ClientHeight };
	UpdateUB(&AOBlurData, sizeof(AO_BUFFER_BLUR_DATA), DrawResourceList[index]->AOBlurResource);
}

//根签名部分-----------------------------------------------------------------------------------------------

void D3DAPP::InitialRootSigList()
{
	//创建默认根签名并将其放入列表中
	RootSigList.insert({ "DEFRootSig",CreateDefaultRootSig() });
	RootSigList.insert({ "ShadowMapRootSig", CreateShadowMapRootSig() });
	RootSigList.insert({ "DeferredRenderRootSig",CreateDeferredRenderRootSig() });
	RootSigList.insert({ "SSAORootSig",CreateSSAORootSig() });
	RootSigList.insert({ "AOBlur",CreateAOBlurSig() });

#ifndef NDEBUG
	cout << "根签名列表初始化完成" << endl;
#endif
}

ComPtr<ID3D12RootSignature> D3DAPP::CreateDefaultRootSig()
{
//第一个根参数
	//创建CBV描述符范围
	D3D12_DESCRIPTOR_RANGE CSUDescRange[3];

	//CBV描述符范围
	CSUDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //类型为CBV
	CSUDescRange[0].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[0].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[0].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[0].OffsetInDescriptorsFromTableStart = 0;            //在描述符表中的偏移量

	//SRV描述符范围
	CSUDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //类型为SRV
	CSUDescRange[1].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[1].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[1].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[1].OffsetInDescriptorsFromTableStart = 5;            //在描述符表中的偏移量

	//UAV描述符范围
	CSUDescRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;       //类型为UAV
	CSUDescRange[2].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[2].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[2].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[2].OffsetInDescriptorsFromTableStart = 10;            //在描述符表中的偏移量

	//创建描述符表
	D3D12_ROOT_DESCRIPTOR_TABLE CSURootTable;
	CSURootTable.NumDescriptorRanges = 3;                            //描述符范围数量
	CSURootTable.pDescriptorRanges = CSUDescRange;                      //描述符范围数组

//第二个根参数
	D3D12_DESCRIPTOR_RANGE SamplerDescRange[1];

	SamplerDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	SamplerDescRange[0].NumDescriptors = 5;
	SamplerDescRange[0].OffsetInDescriptorsFromTableStart = 0;
	SamplerDescRange[0].BaseShaderRegister = 0;
	SamplerDescRange[0].RegisterSpace = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE SamplerRootTable;
	SamplerRootTable.NumDescriptorRanges = 1;
	SamplerRootTable.pDescriptorRanges = SamplerDescRange;

//创建根参数
	D3D12_ROOT_PARAMETER RootParameters[2];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     //根参数类型
	RootParameters[0].DescriptorTable = CSURootTable;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[1].DescriptorTable = SamplerRootTable;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

//根签名
	//描述根签名
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(                                   //描述根签名
		2,                                                                     //根参数数量：2个
		RootParameters,                                                       //根参数数组
		0,                                                                     //静态采样器数量
		nullptr,                                                               //与静态采样器有关
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);         //程序选择输入汇编器（需定义一组顶点缓冲区绑定的输入布局）

	return CreateRootSig(RootSigDesc);
}

ComPtr<ID3D12RootSignature> D3DAPP::CreateShadowMapRootSig()
{
//第一个根参数
	//创建CBV描述符范围
	D3D12_DESCRIPTOR_RANGE CSUDescRange[3];

	//CBV描述符范围
	CSUDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //类型为CBV
	CSUDescRange[0].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[0].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[0].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[0].OffsetInDescriptorsFromTableStart = 0;            //在描述符表中的偏移量

	//SRV描述符范围
	CSUDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //类型为SRV
	CSUDescRange[1].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[1].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[1].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[1].OffsetInDescriptorsFromTableStart = 5;            //在描述符表中的偏移量

	//UAV描述符范围
	CSUDescRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;       //类型为UAV
	CSUDescRange[2].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[2].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[2].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[2].OffsetInDescriptorsFromTableStart = 10;            //在描述符表中的偏移量

	//创建描述符表
	D3D12_ROOT_DESCRIPTOR_TABLE CSURootTable;
	CSURootTable.NumDescriptorRanges = 3;                            //描述符范围数量
	CSURootTable.pDescriptorRanges = CSUDescRange;                      //描述符范围数组

//第二个根参数
	//创建根描述符
	D3D12_ROOT_DESCRIPTOR RootDesc;
	RootDesc.RegisterSpace = 1;
	RootDesc.ShaderRegister = 0;

//创建根参数
	D3D12_ROOT_PARAMETER RootParameters[2];

	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     //根参数类型
	RootParameters[0].DescriptorTable = CSURootTable;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	RootParameters[1].Descriptor = RootDesc;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


//根签名
	//描述根签名
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(                                   //描述根签名
		2,                                                                     //根参数数量：2个
		RootParameters,                                                       //根参数数组
		0,                                                                     //静态采样器数量
		nullptr,                                                               //与静态采样器有关
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);         //程序选择输入汇编器（需定义一组顶点缓冲区绑定的输入布局）

	return CreateRootSig(RootSigDesc);
}

ComPtr<ID3D12RootSignature> D3DAPP::CreateSSAORootSig()
{
	//第一个根参数
	//创建CBV描述符范围
	D3D12_DESCRIPTOR_RANGE CSUDescRange[4];

	CSUDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //类型为CBV
	CSUDescRange[0].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[0].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[0].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[0].OffsetInDescriptorsFromTableStart = 0;            //在描述符表中的偏移量

	CSUDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //类型为SRV
	CSUDescRange[1].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[1].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[1].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[1].OffsetInDescriptorsFromTableStart = 5;            //在描述符表中的偏移量

	CSUDescRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //类型为CBV
	CSUDescRange[2].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[2].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[2].RegisterSpace = 2;                                //着色器空间
	CSUDescRange[2].OffsetInDescriptorsFromTableStart = 10;            //在描述符表中的偏移量

	CSUDescRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //类型为SRV
	CSUDescRange[3].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[3].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[3].RegisterSpace = 1;                                //着色器空间
	CSUDescRange[3].OffsetInDescriptorsFromTableStart = 15;            //在描述符表中的偏移量

	//创建描述符表
	D3D12_ROOT_DESCRIPTOR_TABLE CSURootTable;
	CSURootTable.NumDescriptorRanges = 4;                            //描述符范围数量
	CSURootTable.pDescriptorRanges = CSUDescRange;                      //描述符范围数组

	//第二个根参数
	D3D12_DESCRIPTOR_RANGE SamplerDescRange[1];

	SamplerDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	SamplerDescRange[0].NumDescriptors = 5;
	SamplerDescRange[0].OffsetInDescriptorsFromTableStart = 0;
	SamplerDescRange[0].BaseShaderRegister = 0;
	SamplerDescRange[0].RegisterSpace = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE SamplerRootTable;
	SamplerRootTable.NumDescriptorRanges = 1;
	SamplerRootTable.pDescriptorRanges = SamplerDescRange;

	//创建根参数
	D3D12_ROOT_PARAMETER RootParameters[2];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     //根参数类型
	RootParameters[0].DescriptorTable = CSURootTable;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[1].DescriptorTable = SamplerRootTable;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//根签名
		//描述根签名
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(                                   //描述根签名
		2,                                                                     //根参数数量：2个
		RootParameters,                                                       //根参数数组
		0,                                                                     //静态采样器数量
		nullptr,                                                               //与静态采样器有关
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);         //程序选择输入汇编器（需定义一组顶点缓冲区绑定的输入布局）

	return CreateRootSig(RootSigDesc);
}

ComPtr<ID3D12RootSignature>  D3DAPP::CreateDeferredRenderRootSig()
{
//第一个根参数
	//创建CBV描述符范围
	D3D12_DESCRIPTOR_RANGE CSUDescRange[4];

	CSUDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //类型为CBV
	CSUDescRange[0].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[0].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[0].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[0].OffsetInDescriptorsFromTableStart = 0;            //在描述符表中的偏移量

	CSUDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //类型为SRV
	CSUDescRange[1].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[1].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[1].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[1].OffsetInDescriptorsFromTableStart = 5;            //在描述符表中的偏移量

	CSUDescRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //类型为CBV
	CSUDescRange[2].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[2].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[2].RegisterSpace = 2;                                //着色器空间
	CSUDescRange[2].OffsetInDescriptorsFromTableStart = 10;            //在描述符表中的偏移量

	CSUDescRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //类型为SRV
	CSUDescRange[3].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[3].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[3].RegisterSpace = 1;                                //着色器空间
	CSUDescRange[3].OffsetInDescriptorsFromTableStart = 15;            //在描述符表中的偏移量

	//创建描述符表
	D3D12_ROOT_DESCRIPTOR_TABLE CSURootTable;
	CSURootTable.NumDescriptorRanges = 4;                            //描述符范围数量
	CSURootTable.pDescriptorRanges = CSUDescRange;                      //描述符范围数组

	//第二个根参数
	D3D12_DESCRIPTOR_RANGE SamplerDescRange[1];

	SamplerDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	SamplerDescRange[0].NumDescriptors = 5;
	SamplerDescRange[0].OffsetInDescriptorsFromTableStart = 0;
	SamplerDescRange[0].BaseShaderRegister = 0;
	SamplerDescRange[0].RegisterSpace = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE SamplerRootTable;
	SamplerRootTable.NumDescriptorRanges = 1;
	SamplerRootTable.pDescriptorRanges = SamplerDescRange;

	//创建根参数
	D3D12_ROOT_PARAMETER RootParameters[2];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     //根参数类型
	RootParameters[0].DescriptorTable = CSURootTable;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[1].DescriptorTable = SamplerRootTable;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

//根签名
	//描述根签名
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(                                   //描述根签名
		2,                                                                     //根参数数量：2个
		RootParameters,                                                       //根参数数组
		0,                                                                     //静态采样器数量
		nullptr,                                                               //与静态采样器有关
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);         //程序选择输入汇编器（需定义一组顶点缓冲区绑定的输入布局）

	return CreateRootSig(RootSigDesc);
}

ComPtr<ID3D12RootSignature>  D3DAPP::CreateAOBlurSig()
{
//第一个根参数
	D3D12_DESCRIPTOR_RANGE CSUDescRange[2];

	CSUDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //类型为CBV
	CSUDescRange[0].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[0].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[0].RegisterSpace = 3;                                //着色器空间
	CSUDescRange[0].OffsetInDescriptorsFromTableStart = 0;            //在描述符表中的偏移量

	CSUDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;      //类型为UAV
	CSUDescRange[1].NumDescriptors = 5;                               //范围内描述符数量
	CSUDescRange[1].BaseShaderRegister = 0;                           //基准着色器寄存器
	CSUDescRange[1].RegisterSpace = 0;                                //着色器空间
	CSUDescRange[1].OffsetInDescriptorsFromTableStart = 5;            //在描述符表中的偏移量

	//创建描述符表
	D3D12_ROOT_DESCRIPTOR_TABLE CSURootTable;
	CSURootTable.NumDescriptorRanges = 2;                            //描述符范围数量
	CSURootTable.pDescriptorRanges = CSUDescRange;                      //描述符范围数组

//创建根参数
	D3D12_ROOT_PARAMETER RootParameters[1];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     //根参数类型
	RootParameters[0].DescriptorTable = CSURootTable;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

//根签名
	//描述根签名
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(                                   //描述根签名
		1,                                                                     //根参数数量：2个
		RootParameters,                                                       //根参数数组
		0,                                                                     //静态采样器数量
		nullptr,                                                               //与静态采样器有关
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);         //程序选择输入汇编器（需定义一组顶点缓冲区绑定的输入布局）

	return CreateRootSig(RootSigDesc);
}

//着色器部分-----------------------------------------------------------------------------------------------

void D3DAPP::InitialShaders()
{
	AddShader(VSList, "VS_Common", "vs_5_1");
	AddShader(PSList, "PS_Common", "ps_5_1");

	AddShader(VSList, "VS_CommonShadowMap", "vs_5_1");

	AddShader(PSList, "PS_GBuffer", "ps_5_1");

	AddShader(PSList, "PS_HBAO", "ps_5_1");

	AddShader(VSList, "VS_DeferredRender", "vs_5_1");
	AddShader(PSList, "PS_DeferredShading", "ps_5_1");

	AddShader(CSList, "CS_AOBufferBlurRow", "cs_5_1");
	AddShader(CSList, "CS_AOBufferBlurCol", "cs_5_1");

#ifndef NDEBUG
	cout << "着色器列表初始化完成" << endl;
#endif
}

void D3DAPP::AddShader(unordered_map<string, SHADER>& List, string Name, string Version, wstring File)
{
	ComPtr<ID3DBlob> ShaderCode;
	D3D12_SHADER_BYTECODE Desc = CompileShader(ShaderCode, (wchar_t*)File.data(), (char*)Name.data(), Version.data());

	auto result = List.insert({ Name,{ShaderCode,Desc} });

#ifdef NDEBUG
	if (!result.second)
	{
		cout << "着色器：" << Name << "未添加成功" << endl;
	}
#endif // NDEBUG

}

//IA部分-----------------------------------------------------------------------------------------------

void D3DAPP::InitialIADesc()
{
//输入布局描述
	//顶点、颜色
	vector<D3D12_INPUT_ELEMENT_DESC> P_C =
	{
		//描述顶点结构体第一个元素：顶点位置信息
		{"POSITION",                                                         //语义：顶点坐标
		0,                                                                   //附加到语义上的索引以区多个同类语义，单个为0
		DXGI_FORMAT_R32G32B32_FLOAT,                                         //顶点元素类型：3D 32位浮点向量
		0,                                                                   //指定传递元素所用的输入索引槽
		0,                                                                   //特定输入槽中，顶点结构体首地址道元素起始地址的偏移量
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,                          //标志单个输入槽的数据类型，非实例化
		0},                                                                  //实例数，非实例化设为0
		//描述顶点结构体第二个元素：顶点颜色信息
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	//顶点、法线、纹理坐标
	vector<D3D12_INPUT_ELEMENT_DESC> P_N_T =
	{
		//顶点信息
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//法线信息
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//顶点对应纹理坐标
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	//顶点、法线、纹理坐标、颜色
	vector<D3D12_INPUT_ELEMENT_DESC> P_N_T_C =
	{
		//顶点
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//法线
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//纹理坐标
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//颜色
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	AddIADesc("P_C", P_C);
	AddIADesc("P_N_T", P_N_T);
	AddIADesc("Common", P_N_T_C);

#ifndef NDEBUG
	cout << "顶点输入布局描述列表初始化完成" << endl;
#endif
}

void D3DAPP::AddIADesc(string Name, vector<D3D12_INPUT_ELEMENT_DESC>& ElementDesc)
{
	IADESC IADesc;
	IADesc.ElementDesc = move(ElementDesc);
	IADesc.LayoutDesc = { IADesc.ElementDesc.data(),(UINT)IADesc.ElementDesc.size() };

	auto result = IADescList.insert({ Name,move(IADesc) });

#ifdef NDEBUG
	if (!result.second)
	{
		cout << "输入布局描述：" << Name << "未添加成功" << endl;
	}
#endif // NDEBUG

}

//PSO部分-----------------------------------------------------------------------------------------------

void D3DAPP::InitialPSOList()
{
	PSOList.insert({ "PSOCommon" ,CreateBasicPSO(RootSigList["DEFRootSig"], VSList["VS_Common"].Desc, PSList["PS_Common"].Desc, IADescList["Common"].LayoutDesc) });
	PSOList.insert({ "PSOCommonShadowMap" ,CreateShadowMapPSO(RootSigList["ShadowMapRootSig"], VSList["VS_CommonShadowMap"].Desc, IADescList["Common"].LayoutDesc) });

	vector<DXGI_FORMAT> RTFormatList = { DXGI_FORMAT_R32G32B32A32_FLOAT ,DXGI_FORMAT_R32G32B32A32_FLOAT ,DXGI_FORMAT_R32G32B32A32_FLOAT };
	PSOList.insert({ "PSOCommonGBuffer",CreateGBufferPSO(RootSigList["DEFRootSig"],VSList["VS_Common"].Desc, PSList["PS_GBuffer"].Desc, IADescList["Common"].LayoutDesc, move(RTFormatList)) });

	PSOList.insert({"PSOSSAO", CreateSSAOPSO(RootSigList["SSAORootSig"], VSList["VS_DeferredRender"].Desc, PSList["PS_HBAO"].Desc, IADescList["Common"].LayoutDesc)});

	PSOList.insert({ "PSODeferredRender" ,CreateDeferredRenderPSO(RootSigList["DeferredRenderRootSig"], VSList["VS_DeferredRender"].Desc, PSList["PS_DeferredShading"].Desc, IADescList["Common"].LayoutDesc) });
	
	PSOList.insert({ "PSOAOBufferBlurRow",CreateComputePSO(RootSigList["AOBlur"],CSList["CS_AOBufferBlurRow"].Desc) });
	PSOList.insert({ "PSOAOBufferBlurCol",CreateComputePSO(RootSigList["AOBlur"],CSList["CS_AOBufferBlurCol"].Desc) });

#ifndef NDEBUG
	cout << "PSO列表初始化完成" << endl;
	cout << "PSO数量：" << PSOList.size() << endl;
#endif
}

ComPtr<ID3D12PipelineState> D3DAPP::CreateShadowMapPSO(ComPtr<ID3D12RootSignature> RootSignature,
	D3D12_SHADER_BYTECODE VSByteCodeDesc,D3D12_INPUT_LAYOUT_DESC IAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
	ComPtr<ID3D12PipelineState> PSO;

//描述PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

	D3D12_RASTERIZER_DESC RasterState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RasterState.DepthBias = 10000;
	RasterState.DepthBiasClamp = 0.f;
	RasterState.SlopeScaledDepthBias = 1.f;
	//RasterState.CullMode = D3D12_CULL_MODE_NONE;

	PSODesc.pRootSignature = RootSignature.Get();
	PSODesc.VS = VSByteCodeDesc;
	PSODesc.RasterizerState = RasterState;
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PSODesc.SampleMask = UINT_MAX;
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	PSODesc.PrimitiveTopologyType = TopologyType;
	PSODesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	PSODesc.NumRenderTargets = 0;                                              //无渲染目标
	PSODesc.SampleDesc.Count = 1;
	PSODesc.SampleDesc.Quality = 0;
	PSODesc.DSVFormat = DSFormat;
	PSODesc.InputLayout = IAInputDesc;

//创建PSO
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	return PSO;
}

ComPtr<ID3D12PipelineState> D3DAPP::CreateGBufferPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
	D3D12_INPUT_LAYOUT_DESC iAInputDesc, vector<DXGI_FORMAT> rTFormatList, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
	ComPtr<ID3D12PipelineState> PSO;

	//描述PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

	PSODesc.pRootSignature = rootSignature.Get();                              //绑定根签名
	PSODesc.VS = vSByteCodeDesc;                                               //绑定顶点着色器
	PSODesc.PS = pSByteCodeDesc;                                               //绑定像素着色器
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);          //使用默认光栅化
	//PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //线框模式
	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //绘制全部三角形
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                    //混合状态：默认
	PSODesc.SampleMask = UINT_MAX;                                             //多重采样相关，对所有采样点都进行采样
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);     //深度、模板状态：默认
	PSODesc.PrimitiveTopologyType = topologyType;                              //图元拓扑类型：三角形
	for (int i = 0; i < rTFormatList.size() && i < 8; i++)                     //RT格式
	{
		PSODesc.RTVFormats[i] = rTFormatList[i];
	}
	PSODesc.NumRenderTargets = rTFormatList.size() < 8 ? rTFormatList.size() : 8;   //渲染目标格式数组元素数量
	PSODesc.SampleDesc.Count = 1;                                              //每个像素多重采样数
	PSODesc.SampleDesc.Quality = 0;                                            //多重采样质量
	PSODesc.DSVFormat = DSFormat;                                              //深度模板缓冲区格式
	PSODesc.InputLayout = iAInputDesc;                                         //输入布局描述

	//创建PSO
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	return PSO;
}

ComPtr<ID3D12PipelineState> D3DAPP::CreateSSAOPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
	D3D12_INPUT_LAYOUT_DESC iAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
	ComPtr<ID3D12PipelineState> PSO;

	//描述PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

	PSODesc.pRootSignature = rootSignature.Get();                              //绑定根签名
	PSODesc.VS = vSByteCodeDesc;                                               //绑定顶点着色器
	PSODesc.PS = pSByteCodeDesc;                                               //绑定像素着色器
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);          //使用默认光栅化
	//PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //线框模式
	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //绘制全部三角形
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                    //混合状态：默认
	PSODesc.SampleMask = UINT_MAX;                                             //多重采样相关，对所有采样点都进行采样
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);     //深度、模板状态：默认
	PSODesc.PrimitiveTopologyType = topologyType;                              //图元拓扑类型：三角形
	PSODesc.RTVFormats[0] = DXGI_FORMAT_R32G32_FLOAT;
	PSODesc.NumRenderTargets = 1;                                              //渲染目标格式数组元素数量
	PSODesc.SampleDesc.Count = 1;                                              //每个像素多重采样数
	PSODesc.SampleDesc.Quality = 0;                                            //多重采样质量
	PSODesc.DSVFormat = DXGI_FORMAT_UNKNOWN;                                   //深度模板缓冲区格式
	PSODesc.InputLayout = iAInputDesc;                                         //输入布局描述

	//创建PSO
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	return PSO;
}

ComPtr<ID3D12PipelineState> D3DAPP::CreateDeferredRenderPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
	D3D12_INPUT_LAYOUT_DESC iAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
	ComPtr<ID3D12PipelineState> PSO;

	//描述PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

	PSODesc.pRootSignature = rootSignature.Get();                              //绑定根签名
	PSODesc.VS = vSByteCodeDesc;                                               //绑定顶点着色器
	PSODesc.PS = pSByteCodeDesc;                                               //绑定像素着色器
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);          //使用默认光栅化
	//PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //线框模式
	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //绘制全部三角形
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                    //混合状态：默认
	PSODesc.SampleMask = UINT_MAX;                                             //多重采样相关，对所有采样点都进行采样
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);     //深度、模板状态：默认
	PSODesc.PrimitiveTopologyType = topologyType;                              //图元拓扑类型：三角形
	PSODesc.RTVFormats[0] = RTFormat;
	PSODesc.NumRenderTargets = 1;                                              //渲染目标格式数组元素数量
	PSODesc.SampleDesc.Count = 1;                                              //每个像素多重采样数
	PSODesc.SampleDesc.Quality = 0;                                            //多重采样质量
	PSODesc.DSVFormat = DXGI_FORMAT_UNKNOWN;                                   //深度模板缓冲区格式
	PSODesc.InputLayout = iAInputDesc;                                         //输入布局描述

	//创建PSO
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));
	
	return PSO;
}

ComPtr<ID3D12PipelineState> D3DAPP::CreateComputePSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE cSByteCodeDesc)
{
	ComPtr<ID3D12PipelineState> PSO;

	//描述PSO
	D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc;
	PSODesc.pRootSignature = rootSignature.Get();
	PSODesc.CS = cSByteCodeDesc;
	PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	PSODesc.CachedPSO = {};
	PSODesc.NodeMask = GPUNodeMask;

	//创建PSO
	ThrowIfFailed(Device->CreateComputePipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	return PSO;
}

//绘制部分-----------------------------------------------------------------------------------------------

void D3DAPP::Draw(int drawIndex)
{
//清除之前的临时资源
	thread ReleaseDrawResource(&DRAW_RESOURCE::ReleaseTempResource,DrawResourceList[drawIndex]);

//绘制准备工作
	ComPtr<ID3D12CommandAllocator> PreCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> PreCmdList = CreateCmdList(PreCmdAllocator,L"PreCmd");

	//DSB初始化
	D3D12_CPU_DESCRIPTOR_HANDLE DSVHeapHandle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
	PreCmdList->ClearDepthStencilView(DSVHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,                     //标志，表示要清除的数据类型（深度/模板缓冲区）：深度缓冲区和模板缓冲区都清除
		1.f, 0,                                                                //深度/模板清除值
		0, nullptr);                                                           //同上
	
    //初始化实际渲染目标缓冲区
	CD3DX12_CPU_DESCRIPTOR_HANDLE RealRTVHeapHandle(DrawResourceList[drawIndex]->RTVHeap->GetCPUDescriptorHandleForHeapStart(), 0, RTVSize);

	CD3DX12_RESOURCE_BARRIER RealRTBufferBarrierRender = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->RTBList[0].Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
	PreCmdList->ResourceBarrier(1, &RealRTBufferBarrierRender);

	PreCmdList->ClearRenderTargetView(RealRTVHeapHandle,Colors::LightSteelBlue,0,nullptr);

	//执行
	ExeCmd(PreCmdList, MainCmdQueue);
	
	//确定实际渲染目标和深度缓冲区
	DrawResourceList[drawIndex]->RTVHeapHandle = RealRTVHeapHandle;
	DrawResourceList[drawIndex]->DSVHeapHandle = DSVHeapHandle;
	
	//结束准备工作
	ReleaseDrawResource.join();

	//防止命令分配器被提前析构
	DrawResourceList[drawIndex]->TempAllocatorList.push_back(PreCmdAllocator);
	
//绘制阴影贴图------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
	int Num1 = GeometryList.size() / RealThreadNum;                           //每个线程最小绘制数量
	int Num2 = GeometryList.size() % RealThreadNum;                           //多绘制1个物体的线程数量

	unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter = GeometryList.begin();
	
	//依次计算每个点光源的阴影贴图
	for (int PointIndex = 0; PointIndex < GetPointLightNum(); PointIndex++)
	{
		iter = GeometryList.begin();
	//预备工作
		ComPtr<ID3D12CommandAllocator> ShadowPreCmdAllocator;
		ComPtr<ID3D12GraphicsCommandList> ShadowPreCmdList = CreateCmdList(ShadowPreCmdAllocator, L"ShadowPreCmd");
		
		//获得DS描述符句柄
		DrawResourceList[drawIndex]->ShadowMapDSHandle = DrawResourceList[drawIndex]->ShadowItemList[PointIndex].DescHeap->GetCPUDescriptorHandleForHeapStart();

		//更改阴影贴图资源状态
		CD3DX12_RESOURCE_BARRIER ShadowMapPreBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->ShadowMapBufferArray.Get(), 
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		ShadowPreCmdList->ResourceBarrier(1, &ShadowMapPreBarrier);

		//DSB初始化
		ShadowPreCmdList->ClearDepthStencilView(DrawResourceList[drawIndex]->ShadowMapDSHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

		//执行
		ExeCmd(ShadowPreCmdList, MainCmdQueue);

		DrawResourceList[drawIndex]->TempAllocatorList.push_back(ShadowPreCmdAllocator);

	//计算VP矩阵
		auto PointLightPos = GetPointLightPosition(PointIndex);
		auto LightPositionMat = XMLoadFloat3(&PointLightPos);                                                 //点光源位置

		//计算观察—投影变换矩阵
		auto ViewMat = XMMatrixLookAtLH(LightPositionMat, XMVectorSet(0, 0, 0, 1), XMVectorSet(0, 1, 0, 0));
		auto VPXMMat = ViewMat * PointLightViewMat;
		auto VPMat = LoadMat(VPXMMat);
		UpdateUB(&VPMat, sizeof(VPMat), DrawResourceList[drawIndex]->ShadowItemList[PointIndex].VPMatBuffer);

		//VPMat资源虚拟地址，用以绑定
		DrawResourceList[drawIndex]->PointVPMatBufferAddr = DrawResourceList[drawIndex]->ShadowItemList[PointIndex].VPMatBuffer->GetGPUVirtualAddress();
		
	//计算VPT矩阵
		DrawResourceList[drawIndex]->PointVPTMatList[PointIndex] = LoadMat(VPXMMat * TMat);
		
		//开始绘制
		for (int i = 0; i < RealThreadNum; i++)
		{
			if (iter == GeometryList.end())
			{
				//通知本线程所有命令提交完毕
				{
					lock_guard<mutex> lg(DrawResourceList[drawIndex]->DrawThreadNumMutex);
					DrawResourceList[drawIndex]->DrawThreadCompleteNum++;
				}
				DrawResourceList[drawIndex]->DrawCompleteCV.notify_all();

				continue;
			}
			if (i < Num2)
			{
				thread th(&D3DAPP::DrawShadowMaps, this, iter, drawIndex, Num1 + 1);
				th.detach();

				iter += (Num1 + 1);
			}
			else
			{
				thread th(&D3DAPP::DrawShadowMaps, this, iter, drawIndex, Num1);
				th.detach();

				iter += Num1;
			}
		}

		//等待所有绘制线程执行完毕
		{
			unique_lock<mutex> ul(DrawResourceList[drawIndex]->DrawThreadNumMutex);
			DrawResourceList[drawIndex]->DrawCompleteCV.wait(ul, [this, drawIndex] {return DrawResourceList[drawIndex]->DrawThreadCompleteNum == RealThreadNum; });
			DrawResourceList[drawIndex]->DrawThreadCompleteNum = 0;
		}

	//完成工作
		ComPtr<ID3D12CommandAllocator> ShadowMapPostCmdAllocator;
		ComPtr<ID3D12GraphicsCommandList> ShadowMapPostCmdList = CreateCmdList(ShadowMapPostCmdAllocator, L"ShadowMapPostCmd");

		//更改阴影贴图资源状态
		CD3DX12_RESOURCE_BARRIER ShadowMapBackBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			DrawResourceList[drawIndex]->ShadowMapBufferArray.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ/*, PointIndex*/);
		ShadowMapPostCmdList->ResourceBarrier(1, &ShadowMapBackBarrier);

		ExeCmd(ShadowMapPostCmdList, MainCmdQueue);

		DrawResourceList[drawIndex]->TempAllocatorList.push_back(ShadowMapPostCmdAllocator);
	}

	//更新VPT矩阵
	UpdateUB(DrawResourceList[drawIndex]->PointVPTMatList.data(),
		DrawResourceList[drawIndex]->PointVPTMatList.size() * sizeof(XMFLOAT4X4),
		DrawResourceList[drawIndex]->PointLightVPTListBuffer);

//GBuffer绘制------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	Num1 = GeometryList.size() / RealThreadNum;                           //每个线程最小绘制数量
	Num2 = GeometryList.size() % RealThreadNum;                           //多绘制1个物体的线程数量

	iter = GeometryList.begin();

	ComPtr<ID3D12CommandAllocator> GBufferPreCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> GBufferPreCmdList = CreateCmdList(GBufferPreCmdAllocator, L"GBufferPreCmd");

	//更改GBuffer屏障状态到D3D12_RESOURCE_STATE_RENDER_TARGET
	int GBufferNum = DrawResourceList[drawIndex]->RTBList.size() - 2;        //GBuffer数量
	vector<CD3DX12_RESOURCE_BARRIER> GBufferBarrierListRT;                     //GBuffer屏障数组
	for (int i = 0; i < GBufferNum; i++)
	{
		CD3DX12_RESOURCE_BARRIER GBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			DrawResourceList[drawIndex]->RTBList[i + 1].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
		GBufferBarrierListRT.push_back(GBufferBarrier);
	}
	GBufferPreCmdList->ResourceBarrier(GBufferNum, GBufferBarrierListRT.data());

	//清理GBuffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE GBufferRTVHandle(DrawResourceList[drawIndex]->GBufferRTVHeapHandle);
	float GBufferClearValue[4] = { 0,0,0,0 };
	for (int i = 0; i < GBufferNum; i++)
	{
		GBufferPreCmdList->ClearRenderTargetView(GBufferRTVHandle, GBufferClearValue, 0, nullptr);
		GBufferRTVHandle.Offset(1, RTVSize);
	}

	ExeCmd(GBufferPreCmdList, MainCmdQueue);

	DrawResourceList[drawIndex]->TempAllocatorList.push_back(GBufferPreCmdAllocator);

	//绘制GBuffer
	for (int i = 0; i < RealThreadNum; i++)
	{
		if (iter == GeometryList.end())
		{
			//通知本线程所有命令提交完毕
			{
				lock_guard<mutex> lg(DrawResourceList[drawIndex]->DrawThreadNumMutex);
				DrawResourceList[drawIndex]->DrawThreadCompleteNum++;
			}
			DrawResourceList[drawIndex]->DrawCompleteCV.notify_all();

			continue;
		}
		if (i < Num2)
		{
			thread th(&D3DAPP::DrawGBuffer, this, iter, drawIndex, Num1 + 1);
			th.detach();

			iter += (Num1 + 1);
		}
		else
		{
			thread th(&D3DAPP::DrawGBuffer, this, iter, drawIndex, Num1);
			th.detach();

			iter += Num1;
		}
	}

	//等待所有绘制线程执行完毕
	{
		unique_lock<mutex> ul(DrawResourceList[drawIndex]->DrawThreadNumMutex);
		DrawResourceList[drawIndex]->DrawCompleteCV.wait(ul, [this, drawIndex] {return DrawResourceList[drawIndex]->DrawThreadCompleteNum == RealThreadNum; });
		DrawResourceList[drawIndex]->DrawThreadCompleteNum = 0;
	}

	//更改GBuffer屏障状态到D3D12_RESOURCE_STATE_COMMON
	ComPtr<ID3D12CommandAllocator> GBufferPostCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> GBufferPostCmdList = CreateCmdList(GBufferPostCmdAllocator, L"GBufferPostCmd");

	vector<CD3DX12_RESOURCE_BARRIER> GBufferBarrierListCommon;
	for (int i = 0; i < GBufferNum; i++)
	{
		CD3DX12_RESOURCE_BARRIER GBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			DrawResourceList[drawIndex]->RTBList[i + 1].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
		GBufferBarrierListCommon.push_back(GBufferBarrier);
	}

	GBufferPostCmdList->ResourceBarrier(GBufferNum, GBufferBarrierListCommon.data());

	ExeCmd(GBufferPostCmdList, MainCmdQueue);

	DrawResourceList[drawIndex]->TempAllocatorList.push_back(GBufferPostCmdAllocator);

/*COMMON非混合物体绘制-------------------------------------------------------------------------------*/
	//Num1 = GeometryList.size() / RealThreadNum;                           //每个线程最小绘制数量
	//Num2 = GeometryList.size() % RealThreadNum;                           //多绘制1个物体的线程数量
	//
	//iter = GeometryList.begin();
	//
	//for (int i = 0; i < RealThreadNum; i++)
	//{
	//	if (iter == GeometryList.end())
	//	{
	//		//通知本线程所有命令提交完毕
	//		{
	//			lock_guard<mutex> lg(DrawResourceList[drawIndex]->DrawThreadNumMutex);
	//			DrawResourceList[drawIndex]->DrawThreadCompleteNum++;
	//		}
	//		DrawResourceList[drawIndex]->DrawCompleteCV.notify_all();
	//
	//		continue;
	//	}
	//	if (i < Num2)
	//	{
	//		thread th(&D3DAPP::DrawGeometries, this, iter, drawIndex, Num1 + 1);
	//		th.detach();
	//		
	//		iter += (Num1 + 1);
	//	}
	//	else
	//	{
	//		thread th(&D3DAPP::DrawGeometries, this, iter, drawIndex, Num1);
	//		th.detach();
	//
	//		iter += Num1;
	//	}
	//}
	//
	////等待所有绘制线程执行完毕
	//{
	//	unique_lock<mutex> ul(DrawResourceList[drawIndex]->DrawThreadNumMutex);
	//	DrawResourceList[drawIndex]->DrawCompleteCV.wait(ul, [this, drawIndex] {return DrawResourceList[drawIndex]->DrawThreadCompleteNum == RealThreadNum; });
	//	DrawResourceList[drawIndex]->DrawThreadCompleteNum = 0;
	//}

//SSAO(HBAO)
	ComPtr<ID3D12CommandAllocator> SSAOCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> SSAOCmdList = CreateCmdList(SSAOCmdAllocator, L"SSAOCmd");

	//更改SSAOBuffer屏障状态到D3D12_RESOURCE_STATE_RENDER_TARGET
	CD3DX12_RESOURCE_BARRIER SSAOBufferBarrierRT = CD3DX12_RESOURCE_BARRIER::Transition(
		DrawResourceList[drawIndex]->RTBList[4].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
	SSAOCmdList->ResourceBarrier(1, &SSAOBufferBarrierRT);

	//清理SSAOBuffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE SSAORTVHandle(DrawResourceList[drawIndex]->SSAORTVHeapHandle);
	float SSAOBufferClearValue[4] = { 0,0,0,0 };
	SSAOCmdList->ClearRenderTargetView(SSAORTVHandle, SSAOBufferClearValue, 0, nullptr);

	//
	SetViewPortAndSciRect(SSAOCmdList);
	SSAOCmdList->SetPipelineState(PSOList["PSOSSAO"].Get());
	SSAOCmdList->SetGraphicsRootSignature(RootSigList["SSAORootSig"].Get());
	SSAOCmdList->OMSetRenderTargets(1, &SSAORTVHandle, TRUE, nullptr);

	//绑定描述符堆
	ID3D12DescriptorHeap* SSAODescHeaps[] = { DrawResourceList[drawIndex]->GBufferCSUHeap.Get(),DrawResourceList[drawIndex]->DeferredRenderSamplerDescHeap.Get() };
	SSAOCmdList->SetDescriptorHeaps(2, SSAODescHeaps);

	//设置根签名对应资源
	SSAOCmdList->SetGraphicsRootDescriptorTable(0, DrawResourceList[drawIndex]->GBufferCSUHeap->GetGPUDescriptorHandleForHeapStart());
	SSAOCmdList->SetGraphicsRootDescriptorTable(1, DrawResourceList[drawIndex]->DeferredRenderSamplerDescHeap->GetGPUDescriptorHandleForHeapStart());

	//指定图元拓扑
	SSAOCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//绘制
	SSAOCmdList->DrawInstanced(3, 1, 0, 0);

	//更改SSAOBuffer屏障状态到D3D12_RESOURCE_STATE_COMMON
	CD3DX12_RESOURCE_BARRIER SSAOBufferBarrierCommon = CD3DX12_RESOURCE_BARRIER::Transition(
		DrawResourceList[drawIndex]->RTBList[4].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
	SSAOCmdList->ResourceBarrier(1, &SSAOBufferBarrierCommon);

	ExeCmd(SSAOCmdList, MainCmdQueue);

	DrawResourceList[drawIndex]->TempAllocatorList.push_back(SSAOCmdAllocator);

//对AO图进行模糊处理
	ComPtr<ID3D12CommandAllocator> AOBufferBlurCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> AOBufferBlurCmdList = CreateCmdList(AOBufferBlurCmdAllocator, L"AOBufferBlur");

	//行模糊
	AOBufferBlurCmdList->SetPipelineState(PSOList["PSOAOBufferBlurRow"].Get());
	AOBufferBlurCmdList->SetComputeRootSignature(RootSigList["AOBlur"].Get());
	ID3D12DescriptorHeap* AOBlurDescHeaps[] = { DrawResourceList[drawIndex]->AOBlurCSUHeap.Get() };
	AOBufferBlurCmdList->SetDescriptorHeaps(1, AOBlurDescHeaps);
	AOBufferBlurCmdList->SetComputeRootDescriptorTable(0, DrawResourceList[drawIndex]->AOBlurCSUHeap->GetGPUDescriptorHandleForHeapStart());
	AOBufferBlurCmdList->Dispatch(ClientHeight, 1, 1);                                   //开启线程

	//列模糊
	AOBufferBlurCmdList->SetPipelineState(PSOList["PSOAOBufferBlurCol"].Get());
	AOBufferBlurCmdList->SetComputeRootSignature(RootSigList["AOBlur"].Get());
	//AOBufferBlurCmdList->SetDescriptorHeaps(1, AOBlurDescHeaps);
	AOBufferBlurCmdList->SetComputeRootDescriptorTable(0, DrawResourceList[drawIndex]->AOBlurCSUHeap->GetGPUDescriptorHandleForHeapStart());
	AOBufferBlurCmdList->Dispatch(ClientWidth, 1, 1);                                   //开启线程

	ExeCmd(AOBufferBlurCmdList, MainCmdQueue);

	DrawResourceList[drawIndex]->TempAllocatorList.push_back(AOBufferBlurCmdAllocator);


//延迟渲染------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	ComPtr<ID3D12CommandAllocator> DeferredRenderCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> DeferredRenderCmdList = CreateCmdList(DeferredRenderCmdAllocator, L"DeferredRenderCmd");

	SetViewPortAndSciRect(DeferredRenderCmdList);
	DeferredRenderCmdList->SetPipelineState(PSOList["PSODeferredRender"].Get());
	DeferredRenderCmdList->SetGraphicsRootSignature(RootSigList["DeferredRenderRootSig"].Get());
	DeferredRenderCmdList->OMSetRenderTargets(1, &DrawResourceList[drawIndex]->RTVHeapHandle, TRUE, nullptr);

	//绑定描述符堆
	ID3D12DescriptorHeap* DeferredRenderDescHeaps[] = { DrawResourceList[drawIndex]->GBufferCSUHeap.Get(),DrawResourceList[drawIndex]->DeferredRenderSamplerDescHeap.Get() };
	DeferredRenderCmdList->SetDescriptorHeaps(2, DeferredRenderDescHeaps);

	//设置根签名对应资源
	DeferredRenderCmdList->SetGraphicsRootDescriptorTable(0, DrawResourceList[drawIndex]->GBufferCSUHeap->GetGPUDescriptorHandleForHeapStart());
	DeferredRenderCmdList->SetGraphicsRootDescriptorTable(1, DrawResourceList[drawIndex]->DeferredRenderSamplerDescHeap->GetGPUDescriptorHandleForHeapStart());

	//指定图元拓扑
	DeferredRenderCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//绘制
	DeferredRenderCmdList->DrawInstanced(3, 1, 0, 0);

	ExeCmd(DeferredRenderCmdList, MainCmdQueue);

	DrawResourceList[drawIndex]->TempAllocatorList.push_back(DeferredRenderCmdAllocator);

//后处理部分
	//tone mapping
	//ComPtr<ID3D12CommandAllocator> ToneMappingAllocator;
	//ComPtr<ID3D12GraphicsCommandList> ToneMappingCmdList = CreateCmdList(ToneMappingAllocator);
	//
	//ToneMappingCmdList->Dispatch(32, 1, 1);                               //启动线程

//结束过程------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	ComPtr<ID3D12CommandAllocator> EndCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> EndCmdList = CreateCmdList(EndCmdAllocator, L"EmdCmd");

	//MSAA（未开启，不用）
	{
		////实际渲染目标缓冲区转为解析资源状态
		//CD3DX12_RESOURCE_BARRIER RealBufferBarrierResolve = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->RTBList[0].Get(),
		//	D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
		//EndCmdList->ResourceBarrier(1, &RealBufferBarrierResolve);

		////后台缓冲区转为被解析态
		//CD3DX12_RESOURCE_BARRIER BackBufferBarrierResolveDest = CD3DX12_RESOURCE_BARRIER::Transition(SwapChainBuffer[BackBufferIndex].Get(),
		//	D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST);
		//EndCmdList->ResourceBarrier(1, &BackBufferBarrierResolveDest);

		////MSAA解析
		//EndCmdList->ResolveSubresource(SwapChainBuffer[BackBufferIndex].Get(), 0, DrawResourceList[drawIndex]->RTBList[0].Get(), 0, RTFormat);

		////实际渲染目标缓冲区转为common状态 D3D12_RESOURCE_STATE_COMMON
		//CD3DX12_RESOURCE_BARRIER RealBufferBarrierCommon = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->RTBList[0].Get(),
		//	D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_COMMON);
		//EndCmdList->ResourceBarrier(1, &RealBufferBarrierCommon);

		////获得描述符堆句柄
		////CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart(), BackBufferIndex, RTVSize);

		////后台缓冲区从被解析态转为呈现状态
		//CD3DX12_RESOURCE_BARRIER BackBufferBarrierPresent = CD3DX12_RESOURCE_BARRIER::Transition(SwapChainBuffer[BackBufferIndex].Get(),
		//	D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT);
	}

	//将实际渲染目标缓冲区复制到后台缓冲区
	{
	//状态转换
		//实际渲染目标缓冲区转为复制源状态
		CD3DX12_RESOURCE_BARRIER RealBufferBarrierCopySource = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->RTBList[0].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
		EndCmdList->ResourceBarrier(1, &RealBufferBarrierCopySource);

		//后台缓冲区转为复制目标态
		CD3DX12_RESOURCE_BARRIER BackBufferBarrierResolveDest = CD3DX12_RESOURCE_BARRIER::Transition(SwapChainBuffer[BackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
		EndCmdList->ResourceBarrier(1, &BackBufferBarrierResolveDest);

	//复制
		//描述复制源
		D3D12_TEXTURE_COPY_LOCATION CopySrc;
		CopySrc.pResource = DrawResourceList[drawIndex]->RTBList[0].Get();
		CopySrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		CopySrc.SubresourceIndex = 0;

		//描述复制目标
		D3D12_TEXTURE_COPY_LOCATION CopyDst;
		CopyDst.pResource = SwapChainBuffer[BackBufferIndex].Get();
		CopyDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		CopyDst.SubresourceIndex = 0;

		//复制
		EndCmdList->CopyTextureRegion(&CopyDst, 0, 0, 0, &CopySrc, NULL);

	//状态转换
		//实际渲染目标缓冲区转为common状态 D3D12_RESOURCE_STATE_COMMON
		CD3DX12_RESOURCE_BARRIER RealBufferBarrierCommon = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->RTBList[0].Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
		EndCmdList->ResourceBarrier(1, &RealBufferBarrierCommon);

		//后台缓冲区从被解析态转为呈现状态
		CD3DX12_RESOURCE_BARRIER BackBufferBarrierPresent = CD3DX12_RESOURCE_BARRIER::Transition(SwapChainBuffer[BackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

		EndCmdList->ResourceBarrier(1, &BackBufferBarrierPresent);
	}

	ExeCmd(EndCmdList, MainCmdQueue);

	//防止命令分配器被提前析构
	DrawResourceList[drawIndex]->TempAllocatorList.push_back(EndCmdAllocator);

	//呈现图像并交换前后台缓冲区
	SwapChain->Present(0, 0);

	BackBufferIndex = (BackBufferIndex + 1) % 2;

//绘制结束
	//通知绘制结束
	FluCmdQueue(MainCmdQueue, DrawCompleteEvent);
}

void D3DAPP::DrawGeometries(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawIndex, int geoNum)
{
	//依次绘制多个物体
	for (int i = 0; i < geoNum; i++)
	{
		shared_ptr<GEOMETRY> TempGeo = iter->second;
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->GeoListMutex);
			DrawResourceList[drawIndex]->TempGeoList.push_back(TempGeo);
		}

		//创建命令列表、分配器
		ComPtr<ID3D12CommandAllocator> Allocator;
		ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(Allocator, L"DrawGeometry");

		SetViewPortAndSciRect(CmdList);

		//绑定流水线状态
		CmdList->SetPipelineState(PSOList[GetPSOIndex(TempGeo)].Get());

		//绑定根签名
		CmdList->SetGraphicsRootSignature(RootSigList[GetRootSigIndex(TempGeo)].Get());

		//绘制并执行
		TempGeo->DrawGeometry(CmdList, DrawResourceList[drawIndex], drawIndex);
		ExeCmd(CmdList, MainCmdQueue);

		//防止命令分配器被提前析构
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->AllocatorMutex);
			DrawResourceList[drawIndex]->TempAllocatorList.push_back(Allocator);
		}
		
		iter++;
	}
	
	//通知本线程所有命令提交完毕
	{
		lock_guard<mutex> lg(DrawResourceList[drawIndex]->DrawThreadNumMutex);
		DrawResourceList[drawIndex]->DrawThreadCompleteNum++;
	}
	DrawResourceList[drawIndex]->DrawCompleteCV.notify_all();
}

void D3DAPP::DrawShadowMaps(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawIndex, int geoNum)
{
	//依次绘制多个物体

	for (int i = 0; i < geoNum; i++)
	{
		shared_ptr<GEOMETRY> TempGeo = iter->second;
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->GeoListMutex);
			DrawResourceList[drawIndex]->TempGeoList.push_back(TempGeo);
		}

		//创建命令列表、分配器
		ComPtr<ID3D12CommandAllocator> Allocator;
		ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(Allocator, L"DrawShadowMap");

		//设置视口
		D3D12_VIEWPORT ShadowMapViewPort;
		ShadowMapViewPort.TopLeftX = 0;                                                   //视口左上顶点在后台缓冲区的x坐标
		ShadowMapViewPort.TopLeftY = 0;                                                   //类上，y坐标
		ShadowMapViewPort.Width = static_cast<float>(ShadowWidht);                        //视口宽度
		ShadowMapViewPort.Height = static_cast<float>(ShadowHeight);                      //视口高度
		ShadowMapViewPort.MinDepth = 0.0f;                                                //深度值映射区间左端点值
		ShadowMapViewPort.MaxDepth = 1.0f;                                                //深度值映射区间的右端点值

		CmdList->RSSetViewports(1, &ShadowMapViewPort);

		//设置裁剪矩形
		D3D12_RECT ShadowMapSciRect;
		ShadowMapSciRect.left = 0;                                                        //裁剪矩形区左上角x坐标
		ShadowMapSciRect.top = 0;                                                         //左下角y坐标
		ShadowMapSciRect.right = ShadowWidht;                                             //右下角x坐标
		ShadowMapSciRect.bottom = ShadowHeight;                                           //右下角y坐标

		CmdList->RSSetScissorRects(1, &ShadowMapSciRect);

		//绑定流水线状态
		CmdList->SetPipelineState(PSOList[GetShadowMapPSOIndex(TempGeo)].Get());

		//绑定根签名
		CmdList->SetGraphicsRootSignature(RootSigList[GetShadowMapRootSigIndex(TempGeo)].Get());

		//绘制并执行
		TempGeo->DrawShadowMap(CmdList, DrawResourceList[drawIndex], drawIndex);
		ExeCmd(CmdList, MainCmdQueue);

		//防止命令分配器被提前析构
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->AllocatorMutex);
			DrawResourceList[drawIndex]->TempAllocatorList.push_back(Allocator);
		}

		iter++;
	}

	//通知本线程所有命令提交完毕
	{
		lock_guard<mutex> lg(DrawResourceList[drawIndex]->DrawThreadNumMutex);
		DrawResourceList[drawIndex]->DrawThreadCompleteNum++;
	}
	DrawResourceList[drawIndex]->DrawCompleteCV.notify_all();
}

void D3DAPP::DrawGBuffer(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawIndex, int geoNum)
{
	//依次绘制多个物体
	for (int i = 0; i < geoNum; i++)
	{
		shared_ptr<GEOMETRY> TempGeo = iter->second;
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->GeoListMutex);
			DrawResourceList[drawIndex]->TempGeoList.push_back(TempGeo);
		}

		//创建命令列表、分配器
		ComPtr<ID3D12CommandAllocator> Allocator;
		ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(Allocator, L"DrawGBuffer");

		SetViewPortAndSciRect(CmdList);

		//绑定流水线状态
		CmdList->SetPipelineState(PSOList[GetGBufferPSOIndex(TempGeo)].Get());

		//绑定根签名
		CmdList->SetGraphicsRootSignature(RootSigList[GetRootSigIndex(TempGeo)].Get());

		//绘制并执行
		TempGeo->DrawGBuffer(CmdList, DrawResourceList[drawIndex], drawIndex);
		ExeCmd(CmdList, MainCmdQueue);

		//防止命令分配器被提前析构
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->AllocatorMutex);
			DrawResourceList[drawIndex]->TempAllocatorList.push_back(Allocator);
		}

		iter++;
	}

	//通知本线程所有命令提交完毕
	{
		lock_guard<mutex> lg(DrawResourceList[drawIndex]->DrawThreadNumMutex);
		DrawResourceList[drawIndex]->DrawThreadCompleteNum++;
	}
	DrawResourceList[drawIndex]->DrawCompleteCV.notify_all();
}

string D3DAPP::GetPSOIndex(shared_ptr<GEOMETRY> geometry)
{
	GEOMETRY_TYPE GeoType = geometry->GetType();

	switch (GeoType)
	{
	case common:
	{
		return "PSOCommon";
	}
	}

}

string D3DAPP::GetShadowMapPSOIndex(shared_ptr<GEOMETRY> geometry)
{
	GEOMETRY_TYPE GeoType = geometry->GetType();

	switch (GeoType)
	{
	case common:
	{
		return "PSOCommonShadowMap";
	}
	}
}

string D3DAPP::GetGBufferPSOIndex(shared_ptr<GEOMETRY> geometry)
{
	GEOMETRY_TYPE GeoType = geometry->GetType();
	
	switch (GeoType)
	{
	case common:
	{
		return "PSOCommonGBuffer";
	}
	}
}

string D3DAPP::GetRootSigIndex(shared_ptr<GEOMETRY> geometry)
{
	GEOMETRY_TYPE GeoType = geometry->GetType();

	switch (GeoType)
	{
	case common:
	{
		return "DEFRootSig";
	}
	}
}

string D3DAPP::GetShadowMapRootSigIndex(shared_ptr<GEOMETRY> geometry)
{
	GEOMETRY_TYPE GeoType = geometry->GetType();

	switch (GeoType)
	{
	case common:
	{
		return "ShadowMapRootSig";
	}
	}
}

string D3DAPP::GetGBufferRootSigIndex(shared_ptr<GEOMETRY> geometry)
{
	GEOMETRY_TYPE GeoType = geometry->GetType();

	switch (GeoType)
	{
	case common:
	{
		return "DEFRootSig";
	}
	}
}

void D3DAPP::DrawCompleteDeal()
{
	while (1)
	{
		WaitForSingleObject(DrawCompleteEvent, INFINITE);
		{
			lock_guard<mutex> lg(CompleteDrawNumMutex);
			CompleteDrawNum++;
		}
		DrawCV.notify_one();
	}
}

void D3DAPP::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
//鼠标操作
	//左键按下
	case WM_LBUTTONDOWN:
	{
		//调整当前鼠标位置
		CurrentX = GET_X_LPARAM(lparam);
		CurrentY = GET_Y_LPARAM(lparam);

		//捕获鼠标（包括在窗口外）
		SetCapture(hwnd);

		break;
	}
	//左键松开
	case WM_LBUTTONUP:
	{
		ReleaseCapture();
		break;
	}
	//鼠标移动
	case WM_MOUSEMOVE:
	{
		if (wparam & WM_LBUTTONDOWN)
		{
			int x = GET_X_LPARAM(lparam);
			int y = GET_Y_LPARAM(lparam);

			//计算水平、数值方向移动
			int differx = x - CurrentX;
			int differy = y - CurrentY;

			CurrentX = x;
			CurrentY = y;

			{
				lock_guard<mutex> lg(AngleMoveMutex);
				HorAngleMove -= differx;
				VerAngleMove += differy;
				isChangeAngle = true;
			}

			AngleMoveCV.notify_all();

		}
		break;
	}
//键盘操作
	//按键
	case WM_KEYDOWN:
	{
		switch (wparam)
		{
		//按W
		case 0x57:
		{
			if (WPress == false)
			{
				lock_guard<mutex> lg(ForBackMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&ForwardStartTime);
				WPress = true;
			}
			break;
		}
		//按S
		case 0x53:
		{
			if (SPress == false)
			{
				lock_guard<mutex> lg(ForBackMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&BackwardStartTime);
				SPress = true;
			}
			break;
		}
		//按D
		case 0x44:
		{
			if (DPress == false)
			{
				lock_guard<mutex> lg(RightLeftMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&RightwardStartTime);
				DPress = true;
			}
			break;
		}
		//按A
		case 0x41:
		{
			if (APress == false)
			{
				lock_guard<mutex> lg(RightLeftMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&LeftwardStartTime);
				APress = true;
			}
			break;
		}
		//按E
		case 0x45:
		{
			if (EPress == false)
			{
				lock_guard<mutex> lg(UpDownMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&UpwardStartTime);
				EPress = true;
			}
			break;
		}
		//按Q
		case 0x51:
		{
			if (QPress == false)
			{
				lock_guard<mutex> lg(UpDownMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&DownwardStartTime);
				QPress = true;
			}
			break;
		}
		}
		break;
	}
	//松开键
	case WM_KEYUP:
	{
		switch (wparam)
		{
		//按W
		case 0x57:
		{
			if (WPress)
			{
				lock_guard<mutex> lg(ForBackMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&ForwardEndTime);
				WPress = false;
			}
			break;
		}
		//按S
		case 0x53:
		{
			if (SPress)
			{
				lock_guard<mutex> lg(ForBackMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&BackwardEndTime);
				SPress = false;
			}
			break;
		}
		//按D
		case 0x44:
		{
			if (DPress)
			{
				lock_guard<mutex> lg(RightLeftMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&RightwardEndTime);
				DPress = false;
			}
			break;
		}
		//按A
		case 0x41:
		{
			if (APress)
			{
				lock_guard<mutex> lg(RightLeftMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&LeftwardEndTime);
				APress = false;
			}
			break;
		}
		//按E
		case 0x45:
		{
			if (EPress)
			{
				lock_guard<mutex> lg(UpDownMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&UpwardEndTime);
				EPress = false;
			}
			break;
		}
		//按Q
		case 0x51:
		{
			if (QPress)
			{
				lock_guard<mutex> lg(UpDownMoveTimeMutex);
				QueryPerformanceCounter((LARGE_INTEGER*)&DownwardEndTime);
				QPress = false;
			}
			break;
		}
		}
		break;
	}
//其它
	//更变窗口大小
	case WM_SIZE:
	{
		//更新最新窗口大小
		{
			lock_guard<mutex> lg(WndResizeMutex);
			NewWidth = LOWORD(lparam);
			NewHeight = HIWORD(lparam);
			WndResize = true;
		}
		break;
	}
	}
}

ComPtr<ID3D12DescriptorHeap> D3DAPP::CreateDefalutCSUDescHeap(int drawIndex)
{
/*堆内容：0-4常量缓冲区，5-9着色器资源,10-14
* 0：变换矩阵帧资源缓冲区
* 1：全局帧资源缓冲区
* 5：点光源数组
* 6：单颜色贴图
* 7：实例数据
*/

//创建描述符堆
	int ViewNum = 15;
	ComPtr<ID3D12DescriptorHeap> DescHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, ViewNum);

	//描述符堆句柄
	CD3DX12_CPU_DESCRIPTOR_HANDLE HeapHandle(                             //获得RTV堆首句柄
		DescHeap->GetCPUDescriptorHandleForHeapStart());

//描述符0：


//描述符1：全局帧资源缓冲区描述符
	auto HeapHandle1 = HeapHandle;
	HeapHandle1.Offset(1, CSUSize);
	Device->CreateConstantBufferView(&DrawResourceList[drawIndex]->FrameBufferViewDesc, HeapHandle1);

//描述符2：
	auto HeapHandle2 = HeapHandle;
	HeapHandle2.Offset(2, CSUSize);
	Device->CreateConstantBufferView(&DrawResourceList[drawIndex]->AmbParaLightBufferViewDesc, HeapHandle2);

//描述符3：
	//auto HeapHandle3 = HeapHandle;
	//HeapHandle3.Offset(3, CSUSize);
	//Device->CreateConstantBufferView(, HeapHandle3);

////描述符4：
//	HeapHandle.Offset(1, CSUSize);

//描述符5：
	auto HeapHandle5 = HeapHandle;
	HeapHandle5.Offset(5, CSUSize);
	Device->CreateShaderResourceView(DrawResourceList[drawIndex]->PointLightBuffer.Get(), &DrawResourceList[drawIndex]->PointLightSRVDesc, HeapHandle5);

////描述符6：
//	HeapHandle.Offset(1, CSUSize);
//
////描述符7：
//	HeapHandle.Offset(1, CSUSize);
//
//描述符8：
	auto HeapHandle8 = HeapHandle;
	HeapHandle8.Offset(8, CSUSize);
	Device->CreateShaderResourceView(DrawResourceList[drawIndex]->ShadowMapBufferArray.Get(), &DrawResourceList[drawIndex]->ShadowMapSRVDesc, HeapHandle8);

//描述符9：
	auto HeapHandle9 = HeapHandle;
	HeapHandle9.Offset(9, CSUSize);
	Device->CreateShaderResourceView(DrawResourceList[drawIndex]->PointLightVPTListBuffer.Get(), &DrawResourceList[drawIndex]->PointLightVPTListSRVDesc, HeapHandle9);

//描述符10：

	return DescHeap;
}

ComPtr<ID3D12DescriptorHeap> D3DAPP::CreateDefalutSamplerHeap(int drawIndex)
{
/*堆内容:
*/
	int ViewNum = 15;
	ComPtr<ID3D12DescriptorHeap> DescHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, ViewNum);

	CD3DX12_CPU_DESCRIPTOR_HANDLE HeapHandle(                             //获得RTV堆首句柄
		DescHeap->GetCPUDescriptorHandleForHeapStart());

	//创建阴影贴图采样器
	auto HeapHandle1 = HeapHandle;
	HeapHandle1.Offset(1, CSUSize);
	auto& ShadowMapSamplerDesc = SamplerDescList["ShadowSampler"];
	Device->CreateSampler(&ShadowMapSamplerDesc, HeapHandle1);

	//创建SSAO的阴影贴图采样器
	auto HeapHandle2 = HeapHandle;
	HeapHandle2.Offset(2, CSUSize);
	auto& SSAOSamplerDesc = SamplerDescList["SSAOSampler"];
	Device->CreateSampler(&SSAOSamplerDesc, HeapHandle2);

	return DescHeap;
}

void D3DAPP::ChangeAngle()
{
	while(1)
	{
		int hor, ver;
		{
			unique_lock<mutex> ul(AngleMoveMutex);
			AngleMoveCV.wait(ul, [this] {return isChangeAngle; });

			hor = HorAngleMove;
			ver = VerAngleMove;

			HorAngleMove = 0;
			VerAngleMove = 0;

			isChangeAngle = false;
		}

		{
			lock_guard<mutex> lg(AngleSpeedMutex);
			SetAngle(AngleSpeed * hor, -AngleSpeed * ver);
		}
	}
}

void D3DAPP::ResizeWnd()
{
	int width, height;

	//获得新窗口大小
	{
		unique_lock<mutex> guard(WndResizeMutex);
		WndResizeCV.wait(guard, [this] {return WndResize; });
		width = NewWidth;
		height = NewHeight;
	}

	//改变窗口大小
	ClientWidth = width;
	ClientHeight = height;

	ResizeRTandDS();

	for (int i = 0; i < 3; i++)
	{
		RecreateRTs(DrawResourceList[i]);
	}
}

void D3DAPP::MoveCamera()
{
	__int64 CurrentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);

	float Speed;
	{
		lock_guard<mutex> lg(CameraSpeedMutex);
		Speed = CameraSpeed;
	}

	//计算前后移动距离
	future<float> ForwardDistance = async([this, CurrentTime, Speed] {
		__int64 ForwardMoveTime, BackwardMoveTime;
		{
			lock_guard<mutex> lg(ForBackMoveTimeMutex);

			if (WPress)
			{
				ForwardMoveTime = CurrentTime - ForwardStartTime;
				ForwardStartTime = CurrentTime;
			}
			else
			{
				ForwardMoveTime = ForwardEndTime - ForwardStartTime > 0 ? ForwardEndTime - ForwardStartTime : 0;
				ForwardStartTime = CurrentTime;
			}

			if (SPress)
			{
				BackwardMoveTime = CurrentTime - BackwardStartTime;
				BackwardStartTime = CurrentTime;
			}
			else
			{
				BackwardMoveTime = BackwardEndTime - BackwardStartTime > 0 ? BackwardEndTime - BackwardStartTime : 0;
				BackwardStartTime = CurrentTime;
			}
		}
		return Speed * (ForwardMoveTime - BackwardMoveTime) * TimingInterval;
		});

	//计算左右移动距离
	future<float> RightwardDistance = async([this, CurrentTime, Speed] {
		__int64 RightwardMoveTime, LeftwardMoveTime;
		{
			lock_guard<mutex> lg(RightLeftMoveTimeMutex);

			if (DPress)
			{
				RightwardMoveTime = CurrentTime - RightwardStartTime;
				RightwardStartTime = CurrentTime;
			}
			else
			{
				RightwardMoveTime = RightwardEndTime - RightwardStartTime > 0 ? RightwardEndTime - RightwardStartTime : 0;
				RightwardStartTime = CurrentTime;
			}

			if (APress)
			{
				LeftwardMoveTime = CurrentTime - LeftwardStartTime;
				LeftwardStartTime = CurrentTime;
			}
			else
			{
				LeftwardMoveTime = RightwardEndTime - RightwardStartTime > 0 ? RightwardEndTime - RightwardStartTime : 0;
				LeftwardStartTime = CurrentTime;
			}
		}
		return Speed * (RightwardMoveTime - LeftwardMoveTime) * TimingInterval;
		});

	//计算上下移动距离
	future<float> UpwardDistance = async([this, CurrentTime, Speed] {
		__int64 UpwardMoveTime, DownwardMoveTime;
		{
			lock_guard<mutex> lg(UpDownMoveTimeMutex);

			if (EPress)
			{
				UpwardMoveTime = CurrentTime - UpwardStartTime;
				UpwardStartTime = CurrentTime;
			}
			else
			{
				UpwardMoveTime = UpwardEndTime - UpwardStartTime > 0 ? UpwardEndTime - UpwardStartTime : 0;
				UpwardStartTime = CurrentTime;
			}

			if (QPress)
			{
				DownwardMoveTime = CurrentTime - DownwardStartTime;
				DownwardStartTime = CurrentTime;
			}
			else
			{
				DownwardMoveTime = DownwardEndTime - DownwardStartTime>0 ? DownwardEndTime - DownwardStartTime : 0;
				DownwardStartTime = CurrentTime;
			}
		}
		return Speed * (UpwardMoveTime - DownwardMoveTime) * TimingInterval;
		});

	//移动相机
	Move(ForwardDistance.get(), RightwardDistance.get(), UpwardDistance.get());
}

void D3DAPP::CreateGeometry(wstring name, GEOMETRY_TYPE type, MESH_TYPE meshType, wstring materialName, vector<GEOMETRY_FRAME_DATA> geoFrameData)
{
	if (GeometryList.find(name) != GeometryList.end())                  //检测物体名是否存在
	{
#ifndef NDEBUG
		wcout.imbue(locale("", LC_CTYPE));
		wcout << "新建物体失败：名字“ " << name << " ”已存在" << endl;
#endif
	}
	else
	{
		switch (type)
		{
		case common:
		{
			shared_ptr<GEOMETRY> Geometry(CreateCommon(name, materialName, geoFrameData));
			SetGeometryMesh(Geometry, meshType);
			InsertGeometry(name, Geometry);
			break;
		}
		default:
		{
			cout << "新建物体失败：类型不存在" << endl;
			break;
		}
		}
	}
}

void D3DAPP::SetGeometryMesh(shared_ptr<GEOMETRY> geometry, MESH_TYPE meshType)
{
	switch (meshType)
	{
	case cube0:
	{
		GeometryInitialCube0(geometry);
		break;
	}
	case flat0:
	{
		GeometryInitialFlat0(geometry);
		break;
	}
	default:
	{
		cout << "新建物体失败：网格类型不存在" << endl;
		break;
	}
	}
}

void D3DAPP::InsertGeometry(wstring name, shared_ptr<GEOMETRY> geometry)
{
	//创建顶点和索引缓冲区并完成描述符
	ComPtr<ID3D12Resource> VertexBuffer = CreateDefaultBuffer(geometry->GetVerticesSize(), geometry->GetVerticesAddr());
	ComPtr<ID3D12Resource> IndexBuffer = CreateDefaultBuffer(geometry->GetIndicesSize(), geometry->GetIndicesAddr());

	geometry->CreateVerticesResource(VertexBuffer);
	geometry->CreateIndicesResource(IndexBuffer);

	//插入物体
	GeometryList.insert({ name,geometry });

#ifndef NDEBUG
	wcout.imbue(locale("", LC_CTYPE));
	wcout << L"物体：“ " << name << L"”创建成功" << endl;
#endif
}

void D3DAPP::InsertBlendGeometry(wstring name, shared_ptr<GEOMETRY> geometry)
{
	//创建顶点和索引缓冲区并完成描述符
	ComPtr<ID3D12Resource> VertexBuffer = CreateDefaultBuffer(geometry->GetVerticesSize(), geometry->GetVerticesAddr());
	ComPtr<ID3D12Resource> IndexBuffer = CreateDefaultBuffer(geometry->GetIndicesSize(), geometry->GetIndicesAddr());

	geometry->CreateVerticesResource(VertexBuffer);
	geometry->CreateIndicesResource(IndexBuffer);

	//插入物体
	BlendGeometryList.insert({ name,geometry });

#ifndef NDEBUG
	wcout.imbue(locale("", LC_CTYPE));
	wcout << L"混合物体：“ " << name << L"”创建成功" << endl;
#endif
}

shared_ptr<GEOMETRY> D3DAPP::CreateCommon(wstring name, wstring materialName, vector<GEOMETRY_FRAME_DATA> geoFrameData)
{
	shared_ptr<COMMON> CommonGeometry = make_shared<COMMON>(name, geoFrameData);
	
	auto& TextureStruct = TextureList[materialName.data()];
	auto& SamplerDesc = SamplerDescList["DEFSampler"];

	//创建三个绘制资源
	GEOMETRY_DRAW_RESOURCE* ResourceList = CommonGeometry->GetDrawResource();
	for (int i = 0; i < 3; i++)
	{
		//获得纹理资源
		ResourceList[i].Texture = TextureStruct.Texture;

		//创建描述符堆
		ResourceList[i].CSUDescHeap = CreateDefalutCSUDescHeap(i);
		ResourceList[i].SamplerDescHeap = CreateDefalutSamplerHeap(i);

	//获得CSU描述符堆句柄
		CD3DX12_CPU_DESCRIPTOR_HANDLE CSUHeapHandle(ResourceList[i].CSUDescHeap->GetCPUDescriptorHandleForHeapStart());

		//创建纹理资源的描述符
		auto Handle6 = CSUHeapHandle;
		Handle6.Offset(6, CSUSize);
		Device->CreateShaderResourceView(ResourceList[i].Texture.Get(), &TextureStruct.SRVDesc, Handle6);

		//创建实例缓冲区
		auto Handle7 = CSUHeapHandle;
		Handle7.Offset(7, CSUSize);
		ResourceList[i].GeoFrameResource = CreateUploadBuffer(sizeof(GEOMETRY_FRAME_DATA) * CommonGeometry->GetMaxInstanceNum());

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDESC;

		D3D12_BUFFER_SRV BS;
		BS.FirstElement = 0;
		BS.NumElements = CommonGeometry->GetMaxInstanceNum();
		BS.StructureByteStride = sizeof(GEOMETRY_FRAME_DATA);
		BS.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		SRVDESC.Format = DXGI_FORMAT_UNKNOWN;
		SRVDESC.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SRVDESC.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //不对纹理向量分量重新排序
		SRVDESC.Buffer = BS;

		Device->CreateShaderResourceView(ResourceList[i].GeoFrameResource.Get(), &SRVDESC, Handle7);

	//创建采样器
		CD3DX12_CPU_DESCRIPTOR_HANDLE SamplerDescHandle(ResourceList[i].SamplerDescHeap->GetCPUDescriptorHandleForHeapStart());
		Device->CreateSampler(&SamplerDesc, SamplerDescHandle);
	}

	return CommonGeometry;
}
void D3DAPP::Run()
{
	StartTimer();

	int DrawIndex = 0;

	while (1)
	{
		//窗口大小是否改变
		if (WndResize)
		{
			unique_lock<mutex> ul(DrawMutex);
			DrawCV.wait(ul, [this] {return CompleteDrawNum == 3; });
			ResizeWnd();
			WndResize = false;
		}

		//等待帧队列空出
		unique_lock<mutex> ul(DrawMutex);
		DrawCV.wait(ul, [this] {return CompleteDrawNum > 0; });
		{
			lock_guard<mutex> lg(CompleteDrawNumMutex);
			CompleteDrawNum--;
		}

		//更新资源并绘制
		UpdateDrawResource(DrawIndex);
		Draw(DrawIndex);

		//帧计算
		AddFrames();
		CalcuFrames();

		int cur, min, max, avg;
		GetFrames(cur, min, max, avg);
		string f = "Current:" + to_string(cur) + " Min:" + to_string(min) + " Max:" + to_string(max) + " Avg:" + to_string(avg);
		SetWindowTextA(hwnd, f.data());

		DrawIndex = (DrawIndex + 1) % 3;
	}
}

void operator += (unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator& iter, UINT k)
{
	for (int i = 0; i < k; i++)
	{
		iter++;
	}
}


