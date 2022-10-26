#pragma once

#include"D3DAPP.h"

D3DAPP::D3DAPP(HWND Hwnd, UINT Width, UINT Height) : D3DBASE(Hwnd, Width, Height), NewWidth(Width), NewHeight(Height)
{
	//��ʼ������߳�����
	RealThreadNum = thread::hardware_concurrency() ? thread::hardware_concurrency() : 4;

	//��ʼ��֡����ͬ���¼�
	DrawCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

#ifndef NDEBUG
	cout << "������߳�������" << RealThreadNum << endl;
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

	//��������ӽǸı��߳�
	thread th1(&D3DAPP::ChangeAngle, this);
	th1.detach();

	//����֡����ͬ������
	thread th2(&D3DAPP::DrawCompleteDeal, this);
	th2.detach();

	Sleep(100);

	wstring Texture1 = L"Texture1.dds";
	wstring Texture2 = L"Texture2.dds";

	PBR_ITEM MetalPBR({ 0.8,0.8,0.8 }, 0.3f, 0.9f);

	CreateGeometry(L"������", common, cube0, Texture1);
	GeometryList.find(L"������")->second->PushInstance({ 0, 0,0 }, { 0,0,0 }, { 1,1,1 }, MetalPBR);

	CreateGeometry(L"�ذ�", common, flat0, Texture2);
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			GeometryList.find(L"�ذ�")->second->PushInstance({ float(-2 + 2*i), -3, float(-2 + 2*j) }, { 0,0,0 }, { 1,1,1 }, MetalPBR);
		}
	}
	GeometryList.find(L"�ذ�")->second->PushInstance({ -3,0,0 }, { 0,0,-XM_PI / 2 }, { 3,3,3 }, MetalPBR);
	GeometryList.find(L"�ذ�")->second->PushInstance({ 0,0,-3 }, { XM_PI / 2,0,0 }, { 3,3,3 }, MetalPBR);
}

void D3DAPP::InitialTextureList()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;                       //SRV������

	//���"Texture1.dds"
	{
		D3D12_TEX2D_SRV TexSrv;
		TexSrv.MipLevels = -1;                                         //mipmap�㼶��-1��ʾ����
		TexSrv.MostDetailedMip = 0;                                    //ϸ����ߵĲ㼶
		TexSrv.ResourceMinLODClamp = 0.0;                              //�ɷ��ʵ���С�㼶��0.0��ʾȫ���ɷ���
		TexSrv.PlaneSlice = 0;

		SRVDesc.Format = DXGI_FORMAT_BC3_UNORM;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;         //����ά�ȣ�2D
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //������������������������
		SRVDesc.Texture2D = TexSrv;
	}
	AddTexture(L"Texture1.dds",SRVDesc);

	//���"Texture2.dds"
	{
		D3D12_TEX2D_SRV TexSrv;
		TexSrv.MipLevels = -1;                                         //mipmap�㼶��-1��ʾ����
		TexSrv.MostDetailedMip = 0;                                    //ϸ����ߵĲ㼶
		TexSrv.ResourceMinLODClamp = 0.0;                              //�ɷ��ʵ���С�㼶��0.0��ʾȫ���ɷ���
		TexSrv.PlaneSlice = 0;

		SRVDesc.Format = DXGI_FORMAT_BC3_UNORM;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;         //����ά�ȣ�2D
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //������������������������
		SRVDesc.Texture2D = TexSrv;
	}
	AddTexture(L"Texture2.dds", SRVDesc);

	//���"Texture3.dds"
	{
		D3D12_TEX2D_SRV TexSrv;
		TexSrv.MipLevels = -1;                                         //mipmap�㼶��-1��ʾ����
		TexSrv.MostDetailedMip = 0;                                    //ϸ����ߵĲ㼶
		TexSrv.ResourceMinLODClamp = 0.0;                              //�ɷ��ʵ���С�㼶��0.0��ʾȫ���ɷ���
		TexSrv.PlaneSlice = 0;

		SRVDesc.Format = DXGI_FORMAT_BC3_UNORM;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;         //����ά�ȣ�2D
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //������������������������
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
		wcout << L"�������ʧ�ܣ����ơ�" << fileName << L"���Ѵ���" << endl;
#endif
	}
	else
	{
		ComPtr<ID3D12Resource> Texture = UploadTexture(fileName);
		TextureList.insert({ fileName,{Texture,sRVDesc} });
#ifndef NDEBUG
		wcout << L"����" << fileName << L"����ӳɹ�" << endl;
#endif
	}
}

void D3DAPP::InitialSamplerDescList()
{
	D3D12_SAMPLER_DESC SamplerDesc = {};

	//Ĭ�ϲ�����
	{
		SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;             //���˷�ʽ��ʹ�����Բ�ֵ������С���Ŵ�� mip ������
		SamplerDesc.AddressU = SamplerDesc.AddressV =
			SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;       //��U��V��W��3D�����ϵ�Ѱַģʽ����uv�������Ӵ�ƽ�̴���
		SamplerDesc.MipLODBias = 0;                                       //�����ѯƫ�ƣ������㼶+MipLODBias���в���
		SamplerDesc.MaxAnisotropy = 1;                                    //����������ֵ�������������Ч
		SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;        //���Դ����С�ڻ����Ŀ�����ݣ���Ƚ�ͨ��
		//SamplerDesc.BorderColor                                         //���UVWָ��D3D12_TEXTURE_ADDRESS_MODE_BORDER���������
		SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                           //���mipmap�㼶
		SamplerDesc.MinLOD = 0;                                           //��Сmipmap�㼶
	}
	SamplerDescList.insert({ "DEFSampler",SamplerDesc });

	//ShadowMap PCF
	{
		SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		SamplerDesc.AddressU = SamplerDesc.AddressV =
			SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;       //��U��V��W��3D�����ϵ�Ѱַģʽ��0-1��Χ��������������Ϊָ���߿���ɫ
		SamplerDesc.MipLODBias = 0;                                       //�����ѯƫ�ƣ������㼶+MipLODBias���в���
		SamplerDesc.MaxAnisotropy = 1;                                    //����������ֵ�������������Ч
		SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;        //ָ�����������������в������ݽ��бȽϵĺ���������ʵ����Ӱ��ͼ��Ч��
		//SamplerDesc.BorderColor                                         //���UVWָ��D3D12_TEXTURE_ADDRESS_MODE_BORDER���������
		SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                           //���mipmap�㼶
		SamplerDesc.MinLOD = 0;                                           //��Сmipmap�㼶
	}
	SamplerDescList.insert({ "ShadowSampler",SamplerDesc });

	{
		SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;             //���˷�ʽ��ʹ�����Բ�ֵ������С���Ŵ�� mip ������
		SamplerDesc.AddressU = SamplerDesc.AddressV =
			SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;       //��U��V��W��3D�����ϵ�Ѱַģʽ����uv�������Ӵ�ƽ�̴���
		SamplerDesc.MipLODBias = 0;                                       //�����ѯƫ�ƣ������㼶+MipLODBias���в���
		SamplerDesc.MaxAnisotropy = 1;                                    //����������ֵ�������������Ч
		SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;        //���Դ����С�ڻ����Ŀ�����ݣ���Ƚ�ͨ��
		//SamplerDesc.BorderColor                                         //���UVWָ��D3D12_TEXTURE_ADDRESS_MODE_BORDER���������
		SamplerDesc.MaxLOD = 0;                                            //���mipmap�㼶
		SamplerDesc.MinLOD = 0;                                           //��Сmipmap�㼶
	}
	SamplerDescList.insert({ "SSAOSampler",SamplerDesc });
}

//��Դ������-----------------------------------------------------------------------------------------------

void D3DAPP::InitialDrawResource()
{
//AO BlurȨ����ԴGaussian
	//�����˹Ȩ��
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

	//��������������
	ComPtr<ID3D12Resource> GaussWeightCB;
	auto GaussWeightCBVDesc = CreateConstantBuffer(GaussWeightCB, sizeof(XMFLOAT4) * GaussWeight.size());
	UpdateUB(GaussWeight.data(), sizeof(XMFLOAT4) * GaussWeight.size(), GaussWeightCB);

//��ʼ������DrawResource
	for (int i = 0; i < 3; i++)
	{
		auto DrawResource = make_shared<DRAW_RESOURCE>();

	//����һ��֡��Դ����������
		DrawResource->FrameBufferViewDesc = CreateConstantBuffer(DrawResource->FrameBuffer, sizeof(FRAME_DATA));

	//�����������ƽ�й�֡��Դ����������
		DrawResource->AmbParaLightBufferViewDesc = CreateConstantBuffer(DrawResource->AmbParaLightBuffer, sizeof(LIGHT_FRAME_DATA));

	//�������ԴSR��Դ����������������
		//�������Դ������
		DrawResource->PointLightBuffer = CreateUploadBuffer(sizeof(POINT_LIGHT_FRAME_DATA) * GetPointLightNum());
		
		//���ԴSRV����
		D3D12_BUFFER_SRV BS;
		BS.FirstElement = 0;
		BS.NumElements = GetPointLightNum();
		BS.StructureByteStride = sizeof(POINT_LIGHT_FRAME_DATA);
		BS.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		DrawResource->PointLightSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		DrawResource->PointLightSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		DrawResource->PointLightSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //������������������������
		DrawResource->PointLightSRVDesc.Buffer = BS;
		
	//��ʼ����Ӱ��ͼ���
		//������Ӱ��ͼ��Դ��DSV
		DrawResource->ShadowMapBufferArray = InitialShadowMap(DrawResource->ShadowItemList);

		//������Ӱ��ͼ��Դ��SRV����
		DrawResource->ShadowMapSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		DrawResource->ShadowMapSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		DrawResource->ShadowMapSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		DrawResource->ShadowMapSRVDesc.Texture2DArray.MostDetailedMip = 0;
		DrawResource->ShadowMapSRVDesc.Texture2DArray.MipLevels = 1;
		DrawResource->ShadowMapSRVDesc.Texture2DArray.FirstArraySlice = 0;
		DrawResource->ShadowMapSRVDesc.Texture2DArray.ArraySize = GetPointLightNum();
		DrawResource->ShadowMapSRVDesc.Texture2DArray.PlaneSlice = 0;
		DrawResource->ShadowMapSRVDesc.Texture2DArray.ResourceMinLODClamp = 0.f;

		//������ԴVPT������Դ
		DrawResource->PointLightVPTListBuffer = CreateUploadBuffer(sizeof(XMFLOAT4X4) * GetPointLightNum());

		//���ԴVPT������ԴSRV����
		D3D12_BUFFER_SRV BS2;
		BS2.FirstElement = 0;
		BS2.NumElements = GetPointLightNum();
		BS2.StructureByteStride = sizeof(XMFLOAT4X4);
		BS2.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		DrawResource->PointLightVPTListSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		DrawResource->PointLightVPTListSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		DrawResource->PointLightVPTListSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //������������������������
		DrawResource->PointLightVPTListSRVDesc.Buffer = BS2;

	//SSAO���
		//����HBAO������Դ��ΪĬ�϶ѳ���������
		DrawResource->HBAOResourceCBViewDesc = CreateConstantBuffer(DrawResource->HBAOResource, sizeof(SSAO_RESOURCE)/*, D3D12_HEAP_TYPE_DEFAULT*/);

		//��ò�������
		SSAO_RESOURCE SSAOResource;
		{
			// 8��������ǵ�����
			SSAOResource.NormalList[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
			SSAOResource.NormalList[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

			SSAOResource.NormalList[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
			SSAOResource.NormalList[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

			SSAOResource.NormalList[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
			SSAOResource.NormalList[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

			SSAOResource.NormalList[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
			SSAOResource.NormalList[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

			// 6�������ĵ�����
			SSAOResource.NormalList[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
			SSAOResource.NormalList[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

			SSAOResource.NormalList[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
			SSAOResource.NormalList[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

			SSAOResource.NormalList[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
			SSAOResource.NormalList[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);
		}
		
		//UpdateDB(DrawResource->HBAOResource, &SSAOResource, sizeof(SSAOResource), D3D12_RESOURCE_STATE_GENERIC_READ);
		UpdateUB(&SSAOResource, sizeof(SSAOResource),DrawResource->HBAOResource);

	//AOͼģ��
		DrawResource->AOBlurWeightCB = GaussWeightCB;

		DrawResource->AOBlurCSUHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 10);

		//������������
		CD3DX12_CPU_DESCRIPTOR_HANDLE AOBlurHeapHandle(DrawResource->AOBlurCSUHeap->GetCPUDescriptorHandleForHeapStart());

		//������˹Ȩ��CBV
		Device->CreateConstantBufferView(&GaussWeightCBVDesc, AOBlurHeapHandle);

		//����ģ��������ԴCBV
		AOBlurHeapHandle.Offset(1, CSUSize);
		auto CBVDesc = CreateConstantBuffer(DrawResource->AOBlurResource, sizeof(AO_BUFFER_BLUR_DATA));
		Device->CreateConstantBufferView(&CBVDesc, AOBlurHeapHandle);

	//��ʼ��ʵ����ȾĿ�껺������GBuffer
		//������������
		DrawResource->RTVHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 8);
		DrawResource->GBufferCSUHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 20);

		//�����ӳ���Ⱦ֡��Դ����������
		auto DeferredRenderCBVDesc = CreateConstantBuffer(DrawResource->DeferredRenderCB, sizeof(DEFERRED_RENDER_DATA));

		//��ʼ���ӳ���Ⱦ��������
		{
			//�������Ѿ��
			CD3DX12_CPU_DESCRIPTOR_HANDLE HeapHandle(DrawResource->GBufferCSUHeap->GetCPUDescriptorHandleForHeapStart());

			//������1��ȫ��֡��Դ������������
			auto HeapHandle1 = HeapHandle;
			HeapHandle1.Offset(1, CSUSize);
			Device->CreateConstantBufferView(&DrawResource->FrameBufferViewDesc, HeapHandle1);

			//������2��
			auto HeapHandle2 = HeapHandle;
			HeapHandle2.Offset(2, CSUSize);
			Device->CreateConstantBufferView(&DrawResource->AmbParaLightBufferViewDesc, HeapHandle2);

			//������4��
			auto HeapHandle4 = HeapHandle;
			HeapHandle4.Offset(3, CSUSize);
			Device->CreateConstantBufferView(&DrawResource->HBAOResourceCBViewDesc, HeapHandle4);

			//������5��
			auto HeapHandle5 = HeapHandle;
			HeapHandle5.Offset(5, CSUSize);
			Device->CreateShaderResourceView(DrawResource->PointLightBuffer.Get(), &DrawResource->PointLightSRVDesc, HeapHandle5);

			//������8��
			auto HeapHandle8 = HeapHandle;
			HeapHandle8.Offset(8, CSUSize);
			Device->CreateShaderResourceView(DrawResource->ShadowMapBufferArray.Get(), &DrawResource->ShadowMapSRVDesc, HeapHandle8);

			//������9��
			auto HeapHandle9 = HeapHandle;
			HeapHandle9.Offset(9, CSUSize);
			Device->CreateShaderResourceView(DrawResource->PointLightVPTListBuffer.Get(), &DrawResource->PointLightVPTListSRVDesc, HeapHandle9);

			//������10��
			auto HeapHandle10 = HeapHandle;
			HeapHandle10.Offset(10, CSUSize);
			Device->CreateConstantBufferView(&DeferredRenderCBVDesc, HeapHandle10);
		}

		RecreateRTs(DrawResource);

		DrawResource->DeferredRenderSamplerDescHeap = CreateDefalutSamplerHeap(i);

		DrawResource->PointVPTMatList = vector<XMFLOAT4X4>(GetPointLightNum());


	//��������Դ�����б�
		DrawResourceList.push_back(DrawResource);
	}
}

ComPtr<ID3D12Resource> D3DAPP::InitialShadowMap(vector<SHADOW_ITEM>& shadowItemList)
{
	//���Դ����
	int Num = GetPointLightNum();

//������Դ
	//����dsbuffer����Դ����
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

	//������Դ�����ʽ
	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ClearValue.DepthStencil.Depth = 1.f;
	ClearValue.DepthStencil.Stencil = 0;

	//������Դ
	ComPtr<ID3D12Resource> ShadowMapResource;
	auto HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(Device->CreateCommittedResource(&HeapProperties,
		D3D12_HEAP_FLAG_NONE, &ShadowMapDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		&ClearValue, IID_PPV_ARGS(ShadowMapResource.GetAddressOf())));
	
//������������VP������������
	for (int i = 0; i < Num; i++)
	{
		//��������dsbuffer����Դ��λ��
		D3D12_DEPTH_STENCIL_VIEW_DESC DSViewDesc;
		DSViewDesc.Format = DSFormat;
		DSViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		DSViewDesc.Flags = D3D12_DSV_FLAG_NONE;
		DSViewDesc.Texture2DArray.MipSlice = 0;
		DSViewDesc.Texture2DArray.FirstArraySlice = i;
		DSViewDesc.Texture2DArray.ArraySize = 1;

		//��������������������
		ComPtr<ID3D12DescriptorHeap> ShadowMapDescHeap;
		ShadowMapDescHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
		Device->CreateDepthStencilView(ShadowMapResource.Get(), &DSViewDesc, ShadowMapDescHeap->GetCPUDescriptorHandleForHeapStart());

		//�������������б�
		shadowItemList.push_back(SHADOW_ITEM());
		shadowItemList[i].DescHeap = ShadowMapDescHeap;

		//����VP����������
		CreateConstantBuffer(shadowItemList[i].VPMatBuffer, sizeof(XMFLOAT4X4));
	}

	return ShadowMapResource;
}

void D3DAPP::RecreateRTs(shared_ptr<DRAW_RESOURCE> drawResource)
{
//�ͷ�ԭ��Դ
	drawResource->RTBList.clear();
	//for (auto iter = rTBList.begin(); iter != rTBList.end(); iter++)
	//{
	//	iter->Reset();
	//}

//����������Ѿ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(drawResource->RTVHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE CSUHeapHanlde(drawResource->GBufferCSUHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE AOBlurCSUHeapHandle(drawResource->AOBlurCSUHeap->GetCPUDescriptorHandleForHeapStart());
	CSUHeapHanlde.Offset(15, CSUSize);

//������Դ����Ĭ�϶�
	CD3DX12_HEAP_PROPERTIES RTBProperties(D3D12_HEAP_TYPE_DEFAULT);

//������RealRT
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
	RTDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;       //����Ϊ��Դ����RTV��UAV

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

    //����������
	Device->CreateRenderTargetView(drawResource->RTBList[0].Get(),nullptr,RTVHeapHandle);

//GBuffer��Դ���ֵ
	D3D12_CLEAR_VALUE GBufferClearValue;
	GBufferClearValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	GBufferClearValue.Color[0] = 0;
	GBufferClearValue.Color[1] = 0;
	GBufferClearValue.Color[2] = 0;
	GBufferClearValue.Color[3] = 0;

//������һ��GBuffer������Albedo���ֲڶ�Roughness
	//������Դ������
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
	GBuffer0Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;       //����Ϊ��Դ����RTV��UAV

	ComPtr<ID3D12Resource> GBuffer0;
	ThrowIfFailed(Device->CreateCommittedResource(&RTBProperties, D3D12_HEAP_FLAG_NONE, &GBuffer0Desc, D3D12_RESOURCE_STATE_COMMON, &GBufferClearValue, IID_PPV_ARGS(GBuffer0.GetAddressOf())));
	GBuffer0->SetName(L"GBuffer0");
	drawResource->RTBList.push_back(GBuffer0);

	//����RTV
	RTVHeapHandle.Offset(1, RTVSize);
	Device->CreateRenderTargetView(drawResource->RTBList[1].Get(), nullptr, RTVHeapHandle);

	drawResource->GBufferRTVHeapHandle = RTVHeapHandle;

	//������Դ��SRV
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

//�����ڶ���GBuffer�������ŷ�����Normal��������Metallicity
	//������Դ������
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
	GBuffer1Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;       //����Ϊ��Դ����RTV��UAV

	ComPtr<ID3D12Resource> GBuffer1;
	ThrowIfFailed(Device->CreateCommittedResource(&RTBProperties, D3D12_HEAP_FLAG_NONE, &GBuffer1Desc, D3D12_RESOURCE_STATE_COMMON, &GBufferClearValue, IID_PPV_ARGS(GBuffer1.GetAddressOf())));
	GBuffer1->SetName(L"GBuffer1");
	drawResource->RTBList.push_back(GBuffer1);

	//����RTV
	RTVHeapHandle.Offset(1, RTVSize);
	Device->CreateRenderTargetView(drawResource->RTBList[2].Get(), nullptr, RTVHeapHandle);

	//������Դ��SRV
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

//����������GBuffer�������ŷ�����ϵ��F0
	//������Դ������
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
	GBuffer2Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;       //����Ϊ��Դ����RTV��UAV

	ComPtr<ID3D12Resource> GBuffer2;
	ThrowIfFailed(Device->CreateCommittedResource(&RTBProperties, D3D12_HEAP_FLAG_NONE, &GBuffer2Desc, D3D12_RESOURCE_STATE_COMMON, &GBufferClearValue, IID_PPV_ARGS(GBuffer2.GetAddressOf())));
	GBuffer2->SetName(L"GBuffer2");
	drawResource->RTBList.push_back(GBuffer2);

	//����RTV
	RTVHeapHandle.Offset(1, RTVSize);
	Device->CreateRenderTargetView(drawResource->RTBList[3].Get(), nullptr, RTVHeapHandle);

	//������Դ��SRV
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

//�������ĸ�GBuffer�������ģ�建������SRV
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

//���������GBuffer��ΪA0
	D3D12_CLEAR_VALUE AOGBufferClearValue;
	AOGBufferClearValue.Format = DXGI_FORMAT_R32G32_FLOAT;
	AOGBufferClearValue.Color[0] = 0;
	AOGBufferClearValue.Color[1] = 0;
	AOGBufferClearValue.Color[2] = 0;
	AOGBufferClearValue.Color[3] = 0;
	
	//������Դ������
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
	GBuffer4Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;       //����Ϊ��Դ����RTV��UAV

	ComPtr<ID3D12Resource> GBuffer4;
	ThrowIfFailed(Device->CreateCommittedResource(&RTBProperties, D3D12_HEAP_FLAG_NONE, &GBuffer4Desc, D3D12_RESOURCE_STATE_COMMON, &AOGBufferClearValue, IID_PPV_ARGS(GBuffer4.GetAddressOf())));
	GBuffer4->SetName(L"GBuffer4");
	drawResource->RTBList.push_back(GBuffer4);

	//����RTV
	RTVHeapHandle.Offset(1, RTVSize);
	Device->CreateRenderTargetView(drawResource->RTBList[4].Get(), nullptr, RTVHeapHandle);
	drawResource->SSAORTVHeapHandle = RTVHeapHandle;

	//������Դ��SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC GBuffer4SRVDesc;
	GBuffer4SRVDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
	GBuffer4SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	GBuffer4SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	GBuffer4SRVDesc.Texture2D.MostDetailedMip = 0;
	GBuffer4SRVDesc.Texture2D.MipLevels = 1;
	GBuffer4SRVDesc.Texture2D.PlaneSlice = 0;
	GBuffer4SRVDesc.Texture2D.ResourceMinLODClamp = 0;

	Device->CreateShaderResourceView(drawResource->RTBList[4].Get(), &GBuffer4SRVDesc, CSUHeapHanlde);

	//����UAV
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
	//���λ��
	MoveCamera();

	FRAME_DATA FrameData;

//����ȫ��֡��Դ
	//����ͶӰ�任����
	auto ProjMat = CalcuProjMat(AngleSize, ClientWidth, ClientHeight);
	
	//�������λ�ú�������ռ�任����
	auto ViewMat = CameraFrameSource(FrameData.CameraPosition);

	//����VP����
	FrameData.VPMat = LoadMat(ViewMat * ProjMat);
	DrawResourceList[index]->VPMat = FrameData.VPMat;

	//����ʱ��
	auto Time = GetRunTime();
	DrawResourceList[index]->CurrentTime = Time;
	FrameData.Time = Time;

	UpdateUB(&FrameData, sizeof(FRAME_DATA), DrawResourceList[index]->FrameBuffer);

//���¹���֡��Դ
	//���»������ƽ�й�
	LIGHT_FRAME_DATA LightFrameData = GetLightResource();
	UpdateUB(&LightFrameData, sizeof(LIGHT_FRAME_DATA), DrawResourceList[index]->AmbParaLightBuffer);

	//���µ��Դ
	UpdatePointLightResource(DrawResourceList[index]->PointLightBuffer, Time);

//������Ӱ���
	//auto PointLightRotateMat = XMMatrixRotationY(Time);
	for (int i = 0; i < GetPointLightNum(); i++)
	{
		//������Դ��ǰ��λ
		auto PointLightPos = GetPointLightPosition(i);
		auto LightPositionMat = XMLoadFloat3(&PointLightPos);
		//LightPositionMat = XMVector3TransformCoord(LightPositionMat, PointLightRotateMat);

		//����۲졪��ͶӰ�任����
		auto ViewMat = XMMatrixLookAtLH(LightPositionMat, XMVectorSet(0, 0, 0, 1), XMVectorSet(0, 1, 0, 0));

		DrawResourceList[index]->ShadowItemList[i].PVMat = ViewMat * PointLightViewMat;
	}

//�����ӳ���Ⱦ��������
	DEFERRED_RENDER_DATA DeferredRenderData;
	DeferredRenderData.InverseViewMat = LoadMat(XMMatrixInverse(nullptr, ViewMat));
	DeferredRenderData.ClientHeight = ClientHeight;
	DeferredRenderData.ClientWidth = ClientWidth;
	DeferredRenderData.A = float(1000) / float(1000 - 1);
	DeferredRenderData.B = -1 * DeferredRenderData.A;
	DeferredRenderData.TanHalfFov = tan(AngleSize * XM_PI / 2);

	UpdateUB(&DeferredRenderData, sizeof(DEFERRED_RENDER_DATA), DrawResourceList[index]->DeferredRenderCB);

//����AOģ��������Դ
	AO_BUFFER_BLUR_DATA AOBlurData = { ClientWidth,ClientHeight };
	UpdateUB(&AOBlurData, sizeof(AO_BUFFER_BLUR_DATA), DrawResourceList[index]->AOBlurResource);
}

//��ǩ������-----------------------------------------------------------------------------------------------

void D3DAPP::InitialRootSigList()
{
	//����Ĭ�ϸ�ǩ������������б���
	RootSigList.insert({ "DEFRootSig",CreateDefaultRootSig() });
	RootSigList.insert({ "ShadowMapRootSig", CreateShadowMapRootSig() });
	RootSigList.insert({ "DeferredRenderRootSig",CreateDeferredRenderRootSig() });
	RootSigList.insert({ "SSAORootSig",CreateSSAORootSig() });
	RootSigList.insert({ "AOBlur",CreateAOBlurSig() });

#ifndef NDEBUG
	cout << "��ǩ���б��ʼ�����" << endl;
#endif
}

ComPtr<ID3D12RootSignature> D3DAPP::CreateDefaultRootSig()
{
//��һ��������
	//����CBV��������Χ
	D3D12_DESCRIPTOR_RANGE CSUDescRange[3];

	//CBV��������Χ
	CSUDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //����ΪCBV
	CSUDescRange[0].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[0].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[0].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[0].OffsetInDescriptorsFromTableStart = 0;            //�����������е�ƫ����

	//SRV��������Χ
	CSUDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //����ΪSRV
	CSUDescRange[1].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[1].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[1].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[1].OffsetInDescriptorsFromTableStart = 5;            //�����������е�ƫ����

	//UAV��������Χ
	CSUDescRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;       //����ΪUAV
	CSUDescRange[2].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[2].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[2].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[2].OffsetInDescriptorsFromTableStart = 10;            //�����������е�ƫ����

	//������������
	D3D12_ROOT_DESCRIPTOR_TABLE CSURootTable;
	CSURootTable.NumDescriptorRanges = 3;                            //��������Χ����
	CSURootTable.pDescriptorRanges = CSUDescRange;                      //��������Χ����

//�ڶ���������
	D3D12_DESCRIPTOR_RANGE SamplerDescRange[1];

	SamplerDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	SamplerDescRange[0].NumDescriptors = 5;
	SamplerDescRange[0].OffsetInDescriptorsFromTableStart = 0;
	SamplerDescRange[0].BaseShaderRegister = 0;
	SamplerDescRange[0].RegisterSpace = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE SamplerRootTable;
	SamplerRootTable.NumDescriptorRanges = 1;
	SamplerRootTable.pDescriptorRanges = SamplerDescRange;

//����������
	D3D12_ROOT_PARAMETER RootParameters[2];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     //����������
	RootParameters[0].DescriptorTable = CSURootTable;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[1].DescriptorTable = SamplerRootTable;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

//��ǩ��
	//������ǩ��
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(                                   //������ǩ��
		2,                                                                     //������������2��
		RootParameters,                                                       //����������
		0,                                                                     //��̬����������
		nullptr,                                                               //�뾲̬�������й�
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);         //����ѡ�������������趨��һ�鶥�㻺�����󶨵����벼�֣�

	return CreateRootSig(RootSigDesc);
}

ComPtr<ID3D12RootSignature> D3DAPP::CreateShadowMapRootSig()
{
//��һ��������
	//����CBV��������Χ
	D3D12_DESCRIPTOR_RANGE CSUDescRange[3];

	//CBV��������Χ
	CSUDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //����ΪCBV
	CSUDescRange[0].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[0].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[0].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[0].OffsetInDescriptorsFromTableStart = 0;            //�����������е�ƫ����

	//SRV��������Χ
	CSUDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //����ΪSRV
	CSUDescRange[1].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[1].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[1].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[1].OffsetInDescriptorsFromTableStart = 5;            //�����������е�ƫ����

	//UAV��������Χ
	CSUDescRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;       //����ΪUAV
	CSUDescRange[2].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[2].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[2].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[2].OffsetInDescriptorsFromTableStart = 10;            //�����������е�ƫ����

	//������������
	D3D12_ROOT_DESCRIPTOR_TABLE CSURootTable;
	CSURootTable.NumDescriptorRanges = 3;                            //��������Χ����
	CSURootTable.pDescriptorRanges = CSUDescRange;                      //��������Χ����

//�ڶ���������
	//������������
	D3D12_ROOT_DESCRIPTOR RootDesc;
	RootDesc.RegisterSpace = 1;
	RootDesc.ShaderRegister = 0;

//����������
	D3D12_ROOT_PARAMETER RootParameters[2];

	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     //����������
	RootParameters[0].DescriptorTable = CSURootTable;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	RootParameters[1].Descriptor = RootDesc;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


//��ǩ��
	//������ǩ��
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(                                   //������ǩ��
		2,                                                                     //������������2��
		RootParameters,                                                       //����������
		0,                                                                     //��̬����������
		nullptr,                                                               //�뾲̬�������й�
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);         //����ѡ�������������趨��һ�鶥�㻺�����󶨵����벼�֣�

	return CreateRootSig(RootSigDesc);
}

ComPtr<ID3D12RootSignature> D3DAPP::CreateSSAORootSig()
{
	//��һ��������
	//����CBV��������Χ
	D3D12_DESCRIPTOR_RANGE CSUDescRange[4];

	CSUDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //����ΪCBV
	CSUDescRange[0].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[0].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[0].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[0].OffsetInDescriptorsFromTableStart = 0;            //�����������е�ƫ����

	CSUDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //����ΪSRV
	CSUDescRange[1].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[1].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[1].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[1].OffsetInDescriptorsFromTableStart = 5;            //�����������е�ƫ����

	CSUDescRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //����ΪCBV
	CSUDescRange[2].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[2].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[2].RegisterSpace = 2;                                //��ɫ���ռ�
	CSUDescRange[2].OffsetInDescriptorsFromTableStart = 10;            //�����������е�ƫ����

	CSUDescRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //����ΪSRV
	CSUDescRange[3].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[3].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[3].RegisterSpace = 1;                                //��ɫ���ռ�
	CSUDescRange[3].OffsetInDescriptorsFromTableStart = 15;            //�����������е�ƫ����

	//������������
	D3D12_ROOT_DESCRIPTOR_TABLE CSURootTable;
	CSURootTable.NumDescriptorRanges = 4;                            //��������Χ����
	CSURootTable.pDescriptorRanges = CSUDescRange;                      //��������Χ����

	//�ڶ���������
	D3D12_DESCRIPTOR_RANGE SamplerDescRange[1];

	SamplerDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	SamplerDescRange[0].NumDescriptors = 5;
	SamplerDescRange[0].OffsetInDescriptorsFromTableStart = 0;
	SamplerDescRange[0].BaseShaderRegister = 0;
	SamplerDescRange[0].RegisterSpace = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE SamplerRootTable;
	SamplerRootTable.NumDescriptorRanges = 1;
	SamplerRootTable.pDescriptorRanges = SamplerDescRange;

	//����������
	D3D12_ROOT_PARAMETER RootParameters[2];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     //����������
	RootParameters[0].DescriptorTable = CSURootTable;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[1].DescriptorTable = SamplerRootTable;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//��ǩ��
		//������ǩ��
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(                                   //������ǩ��
		2,                                                                     //������������2��
		RootParameters,                                                       //����������
		0,                                                                     //��̬����������
		nullptr,                                                               //�뾲̬�������й�
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);         //����ѡ�������������趨��һ�鶥�㻺�����󶨵����벼�֣�

	return CreateRootSig(RootSigDesc);
}

ComPtr<ID3D12RootSignature>  D3DAPP::CreateDeferredRenderRootSig()
{
//��һ��������
	//����CBV��������Χ
	D3D12_DESCRIPTOR_RANGE CSUDescRange[4];

	CSUDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //����ΪCBV
	CSUDescRange[0].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[0].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[0].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[0].OffsetInDescriptorsFromTableStart = 0;            //�����������е�ƫ����

	CSUDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //����ΪSRV
	CSUDescRange[1].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[1].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[1].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[1].OffsetInDescriptorsFromTableStart = 5;            //�����������е�ƫ����

	CSUDescRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //����ΪCBV
	CSUDescRange[2].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[2].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[2].RegisterSpace = 2;                                //��ɫ���ռ�
	CSUDescRange[2].OffsetInDescriptorsFromTableStart = 10;            //�����������е�ƫ����

	CSUDescRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;      //����ΪSRV
	CSUDescRange[3].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[3].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[3].RegisterSpace = 1;                                //��ɫ���ռ�
	CSUDescRange[3].OffsetInDescriptorsFromTableStart = 15;            //�����������е�ƫ����

	//������������
	D3D12_ROOT_DESCRIPTOR_TABLE CSURootTable;
	CSURootTable.NumDescriptorRanges = 4;                            //��������Χ����
	CSURootTable.pDescriptorRanges = CSUDescRange;                      //��������Χ����

	//�ڶ���������
	D3D12_DESCRIPTOR_RANGE SamplerDescRange[1];

	SamplerDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	SamplerDescRange[0].NumDescriptors = 5;
	SamplerDescRange[0].OffsetInDescriptorsFromTableStart = 0;
	SamplerDescRange[0].BaseShaderRegister = 0;
	SamplerDescRange[0].RegisterSpace = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE SamplerRootTable;
	SamplerRootTable.NumDescriptorRanges = 1;
	SamplerRootTable.pDescriptorRanges = SamplerDescRange;

	//����������
	D3D12_ROOT_PARAMETER RootParameters[2];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     //����������
	RootParameters[0].DescriptorTable = CSURootTable;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[1].DescriptorTable = SamplerRootTable;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

//��ǩ��
	//������ǩ��
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(                                   //������ǩ��
		2,                                                                     //������������2��
		RootParameters,                                                       //����������
		0,                                                                     //��̬����������
		nullptr,                                                               //�뾲̬�������й�
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);         //����ѡ�������������趨��һ�鶥�㻺�����󶨵����벼�֣�

	return CreateRootSig(RootSigDesc);
}

ComPtr<ID3D12RootSignature>  D3DAPP::CreateAOBlurSig()
{
//��һ��������
	D3D12_DESCRIPTOR_RANGE CSUDescRange[2];

	CSUDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;      //����ΪCBV
	CSUDescRange[0].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[0].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[0].RegisterSpace = 3;                                //��ɫ���ռ�
	CSUDescRange[0].OffsetInDescriptorsFromTableStart = 0;            //�����������е�ƫ����

	CSUDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;      //����ΪUAV
	CSUDescRange[1].NumDescriptors = 5;                               //��Χ������������
	CSUDescRange[1].BaseShaderRegister = 0;                           //��׼��ɫ���Ĵ���
	CSUDescRange[1].RegisterSpace = 0;                                //��ɫ���ռ�
	CSUDescRange[1].OffsetInDescriptorsFromTableStart = 5;            //�����������е�ƫ����

	//������������
	D3D12_ROOT_DESCRIPTOR_TABLE CSURootTable;
	CSURootTable.NumDescriptorRanges = 2;                            //��������Χ����
	CSURootTable.pDescriptorRanges = CSUDescRange;                      //��������Χ����

//����������
	D3D12_ROOT_PARAMETER RootParameters[1];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     //����������
	RootParameters[0].DescriptorTable = CSURootTable;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

//��ǩ��
	//������ǩ��
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(                                   //������ǩ��
		1,                                                                     //������������2��
		RootParameters,                                                       //����������
		0,                                                                     //��̬����������
		nullptr,                                                               //�뾲̬�������й�
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);         //����ѡ�������������趨��һ�鶥�㻺�����󶨵����벼�֣�

	return CreateRootSig(RootSigDesc);
}

//��ɫ������-----------------------------------------------------------------------------------------------

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
	cout << "��ɫ���б��ʼ�����" << endl;
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
		cout << "��ɫ����" << Name << "δ��ӳɹ�" << endl;
	}
#endif // NDEBUG

}

//IA����-----------------------------------------------------------------------------------------------

void D3DAPP::InitialIADesc()
{
//���벼������
	//���㡢��ɫ
	vector<D3D12_INPUT_ELEMENT_DESC> P_C =
	{
		//��������ṹ���һ��Ԫ�أ�����λ����Ϣ
		{"POSITION",                                                         //���壺��������
		0,                                                                   //���ӵ������ϵ������������ͬ�����壬����Ϊ0
		DXGI_FORMAT_R32G32B32_FLOAT,                                         //����Ԫ�����ͣ�3D 32λ��������
		0,                                                                   //ָ������Ԫ�����õ�����������
		0,                                                                   //�ض�������У�����ṹ���׵�ַ��Ԫ����ʼ��ַ��ƫ����
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,                          //��־��������۵��������ͣ���ʵ����
		0},                                                                  //ʵ��������ʵ������Ϊ0
		//��������ṹ��ڶ���Ԫ�أ�������ɫ��Ϣ
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	//���㡢���ߡ���������
	vector<D3D12_INPUT_ELEMENT_DESC> P_N_T =
	{
		//������Ϣ
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//������Ϣ
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//�����Ӧ��������
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	//���㡢���ߡ��������ꡢ��ɫ
	vector<D3D12_INPUT_ELEMENT_DESC> P_N_T_C =
	{
		//����
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//����
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//��������
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//��ɫ
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	AddIADesc("P_C", P_C);
	AddIADesc("P_N_T", P_N_T);
	AddIADesc("Common", P_N_T_C);

#ifndef NDEBUG
	cout << "�������벼�������б��ʼ�����" << endl;
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
		cout << "���벼��������" << Name << "δ��ӳɹ�" << endl;
	}
#endif // NDEBUG

}

//PSO����-----------------------------------------------------------------------------------------------

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
	cout << "PSO�б��ʼ�����" << endl;
	cout << "PSO������" << PSOList.size() << endl;
#endif
}

ComPtr<ID3D12PipelineState> D3DAPP::CreateShadowMapPSO(ComPtr<ID3D12RootSignature> RootSignature,
	D3D12_SHADER_BYTECODE VSByteCodeDesc,D3D12_INPUT_LAYOUT_DESC IAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
	ComPtr<ID3D12PipelineState> PSO;

//����PSO
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
	PSODesc.NumRenderTargets = 0;                                              //����ȾĿ��
	PSODesc.SampleDesc.Count = 1;
	PSODesc.SampleDesc.Quality = 0;
	PSODesc.DSVFormat = DSFormat;
	PSODesc.InputLayout = IAInputDesc;

//����PSO
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	return PSO;
}

ComPtr<ID3D12PipelineState> D3DAPP::CreateGBufferPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
	D3D12_INPUT_LAYOUT_DESC iAInputDesc, vector<DXGI_FORMAT> rTFormatList, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
	ComPtr<ID3D12PipelineState> PSO;

	//����PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

	PSODesc.pRootSignature = rootSignature.Get();                              //�󶨸�ǩ��
	PSODesc.VS = vSByteCodeDesc;                                               //�󶨶�����ɫ��
	PSODesc.PS = pSByteCodeDesc;                                               //��������ɫ��
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);          //ʹ��Ĭ�Ϲ�դ��
	//PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //�߿�ģʽ
	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //����ȫ��������
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                    //���״̬��Ĭ��
	PSODesc.SampleMask = UINT_MAX;                                             //���ز�����أ������в����㶼���в���
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);     //��ȡ�ģ��״̬��Ĭ��
	PSODesc.PrimitiveTopologyType = topologyType;                              //ͼԪ�������ͣ�������
	for (int i = 0; i < rTFormatList.size() && i < 8; i++)                     //RT��ʽ
	{
		PSODesc.RTVFormats[i] = rTFormatList[i];
	}
	PSODesc.NumRenderTargets = rTFormatList.size() < 8 ? rTFormatList.size() : 8;   //��ȾĿ���ʽ����Ԫ������
	PSODesc.SampleDesc.Count = 1;                                              //ÿ�����ض��ز�����
	PSODesc.SampleDesc.Quality = 0;                                            //���ز�������
	PSODesc.DSVFormat = DSFormat;                                              //���ģ�建������ʽ
	PSODesc.InputLayout = iAInputDesc;                                         //���벼������

	//����PSO
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	return PSO;
}

ComPtr<ID3D12PipelineState> D3DAPP::CreateSSAOPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
	D3D12_INPUT_LAYOUT_DESC iAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
	ComPtr<ID3D12PipelineState> PSO;

	//����PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

	PSODesc.pRootSignature = rootSignature.Get();                              //�󶨸�ǩ��
	PSODesc.VS = vSByteCodeDesc;                                               //�󶨶�����ɫ��
	PSODesc.PS = pSByteCodeDesc;                                               //��������ɫ��
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);          //ʹ��Ĭ�Ϲ�դ��
	//PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //�߿�ģʽ
	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //����ȫ��������
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                    //���״̬��Ĭ��
	PSODesc.SampleMask = UINT_MAX;                                             //���ز�����أ������в����㶼���в���
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);     //��ȡ�ģ��״̬��Ĭ��
	PSODesc.PrimitiveTopologyType = topologyType;                              //ͼԪ�������ͣ�������
	PSODesc.RTVFormats[0] = DXGI_FORMAT_R32G32_FLOAT;
	PSODesc.NumRenderTargets = 1;                                              //��ȾĿ���ʽ����Ԫ������
	PSODesc.SampleDesc.Count = 1;                                              //ÿ�����ض��ز�����
	PSODesc.SampleDesc.Quality = 0;                                            //���ز�������
	PSODesc.DSVFormat = DXGI_FORMAT_UNKNOWN;                                   //���ģ�建������ʽ
	PSODesc.InputLayout = iAInputDesc;                                         //���벼������

	//����PSO
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	return PSO;
}

ComPtr<ID3D12PipelineState> D3DAPP::CreateDeferredRenderPSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE vSByteCodeDesc, D3D12_SHADER_BYTECODE pSByteCodeDesc,
	D3D12_INPUT_LAYOUT_DESC iAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
	ComPtr<ID3D12PipelineState> PSO;

	//����PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

	PSODesc.pRootSignature = rootSignature.Get();                              //�󶨸�ǩ��
	PSODesc.VS = vSByteCodeDesc;                                               //�󶨶�����ɫ��
	PSODesc.PS = pSByteCodeDesc;                                               //��������ɫ��
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);          //ʹ��Ĭ�Ϲ�դ��
	//PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //�߿�ģʽ
	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //����ȫ��������
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                    //���״̬��Ĭ��
	PSODesc.SampleMask = UINT_MAX;                                             //���ز�����أ������в����㶼���в���
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);     //��ȡ�ģ��״̬��Ĭ��
	PSODesc.PrimitiveTopologyType = topologyType;                              //ͼԪ�������ͣ�������
	PSODesc.RTVFormats[0] = RTFormat;
	PSODesc.NumRenderTargets = 1;                                              //��ȾĿ���ʽ����Ԫ������
	PSODesc.SampleDesc.Count = 1;                                              //ÿ�����ض��ز�����
	PSODesc.SampleDesc.Quality = 0;                                            //���ز�������
	PSODesc.DSVFormat = DXGI_FORMAT_UNKNOWN;                                   //���ģ�建������ʽ
	PSODesc.InputLayout = iAInputDesc;                                         //���벼������

	//����PSO
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));
	
	return PSO;
}

ComPtr<ID3D12PipelineState> D3DAPP::CreateComputePSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE cSByteCodeDesc)
{
	ComPtr<ID3D12PipelineState> PSO;

	//����PSO
	D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc;
	PSODesc.pRootSignature = rootSignature.Get();
	PSODesc.CS = cSByteCodeDesc;
	PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	PSODesc.CachedPSO = {};
	PSODesc.NodeMask = GPUNodeMask;

	//����PSO
	ThrowIfFailed(Device->CreateComputePipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	return PSO;
}

//���Ʋ���-----------------------------------------------------------------------------------------------

void D3DAPP::Draw(int drawIndex)
{
//���֮ǰ����ʱ��Դ
	thread ReleaseDrawResource(&DRAW_RESOURCE::ReleaseTempResource,DrawResourceList[drawIndex]);

//����׼������
	ComPtr<ID3D12CommandAllocator> PreCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> PreCmdList = CreateCmdList(PreCmdAllocator,L"PreCmd");

	//DSB��ʼ��
	D3D12_CPU_DESCRIPTOR_HANDLE DSVHeapHandle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
	PreCmdList->ClearDepthStencilView(DSVHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,                     //��־����ʾҪ������������ͣ����/ģ�建����������Ȼ�������ģ�建���������
		1.f, 0,                                                                //���/ģ�����ֵ
		0, nullptr);                                                           //ͬ��
	
    //��ʼ��ʵ����ȾĿ�껺����
	CD3DX12_CPU_DESCRIPTOR_HANDLE RealRTVHeapHandle(DrawResourceList[drawIndex]->RTVHeap->GetCPUDescriptorHandleForHeapStart(), 0, RTVSize);

	CD3DX12_RESOURCE_BARRIER RealRTBufferBarrierRender = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->RTBList[0].Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
	PreCmdList->ResourceBarrier(1, &RealRTBufferBarrierRender);

	PreCmdList->ClearRenderTargetView(RealRTVHeapHandle,Colors::LightSteelBlue,0,nullptr);

	//ִ��
	ExeCmd(PreCmdList, MainCmdQueue);
	
	//ȷ��ʵ����ȾĿ�����Ȼ�����
	DrawResourceList[drawIndex]->RTVHeapHandle = RealRTVHeapHandle;
	DrawResourceList[drawIndex]->DSVHeapHandle = DSVHeapHandle;
	
	//����׼������
	ReleaseDrawResource.join();

	//��ֹ�������������ǰ����
	DrawResourceList[drawIndex]->TempAllocatorList.push_back(PreCmdAllocator);
	
//������Ӱ��ͼ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
	int Num1 = GeometryList.size() / RealThreadNum;                           //ÿ���߳���С��������
	int Num2 = GeometryList.size() % RealThreadNum;                           //�����1��������߳�����

	unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter = GeometryList.begin();
	
	//���μ���ÿ�����Դ����Ӱ��ͼ
	for (int PointIndex = 0; PointIndex < GetPointLightNum(); PointIndex++)
	{
		iter = GeometryList.begin();
	//Ԥ������
		ComPtr<ID3D12CommandAllocator> ShadowPreCmdAllocator;
		ComPtr<ID3D12GraphicsCommandList> ShadowPreCmdList = CreateCmdList(ShadowPreCmdAllocator, L"ShadowPreCmd");
		
		//���DS���������
		DrawResourceList[drawIndex]->ShadowMapDSHandle = DrawResourceList[drawIndex]->ShadowItemList[PointIndex].DescHeap->GetCPUDescriptorHandleForHeapStart();

		//������Ӱ��ͼ��Դ״̬
		CD3DX12_RESOURCE_BARRIER ShadowMapPreBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->ShadowMapBufferArray.Get(), 
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		ShadowPreCmdList->ResourceBarrier(1, &ShadowMapPreBarrier);

		//DSB��ʼ��
		ShadowPreCmdList->ClearDepthStencilView(DrawResourceList[drawIndex]->ShadowMapDSHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

		//ִ��
		ExeCmd(ShadowPreCmdList, MainCmdQueue);

		DrawResourceList[drawIndex]->TempAllocatorList.push_back(ShadowPreCmdAllocator);

	//����VP����
		auto PointLightPos = GetPointLightPosition(PointIndex);
		auto LightPositionMat = XMLoadFloat3(&PointLightPos);                                                 //���Դλ��

		//����۲졪ͶӰ�任����
		auto ViewMat = XMMatrixLookAtLH(LightPositionMat, XMVectorSet(0, 0, 0, 1), XMVectorSet(0, 1, 0, 0));
		auto VPXMMat = ViewMat * PointLightViewMat;
		auto VPMat = LoadMat(VPXMMat);
		UpdateUB(&VPMat, sizeof(VPMat), DrawResourceList[drawIndex]->ShadowItemList[PointIndex].VPMatBuffer);

		//VPMat��Դ�����ַ�����԰�
		DrawResourceList[drawIndex]->PointVPMatBufferAddr = DrawResourceList[drawIndex]->ShadowItemList[PointIndex].VPMatBuffer->GetGPUVirtualAddress();
		
	//����VPT����
		DrawResourceList[drawIndex]->PointVPTMatList[PointIndex] = LoadMat(VPXMMat * TMat);
		
		//��ʼ����
		for (int i = 0; i < RealThreadNum; i++)
		{
			if (iter == GeometryList.end())
			{
				//֪ͨ���߳����������ύ���
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

		//�ȴ����л����߳�ִ�����
		{
			unique_lock<mutex> ul(DrawResourceList[drawIndex]->DrawThreadNumMutex);
			DrawResourceList[drawIndex]->DrawCompleteCV.wait(ul, [this, drawIndex] {return DrawResourceList[drawIndex]->DrawThreadCompleteNum == RealThreadNum; });
			DrawResourceList[drawIndex]->DrawThreadCompleteNum = 0;
		}

	//��ɹ���
		ComPtr<ID3D12CommandAllocator> ShadowMapPostCmdAllocator;
		ComPtr<ID3D12GraphicsCommandList> ShadowMapPostCmdList = CreateCmdList(ShadowMapPostCmdAllocator, L"ShadowMapPostCmd");

		//������Ӱ��ͼ��Դ״̬
		CD3DX12_RESOURCE_BARRIER ShadowMapBackBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			DrawResourceList[drawIndex]->ShadowMapBufferArray.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ/*, PointIndex*/);
		ShadowMapPostCmdList->ResourceBarrier(1, &ShadowMapBackBarrier);

		ExeCmd(ShadowMapPostCmdList, MainCmdQueue);

		DrawResourceList[drawIndex]->TempAllocatorList.push_back(ShadowMapPostCmdAllocator);
	}

	//����VPT����
	UpdateUB(DrawResourceList[drawIndex]->PointVPTMatList.data(),
		DrawResourceList[drawIndex]->PointVPTMatList.size() * sizeof(XMFLOAT4X4),
		DrawResourceList[drawIndex]->PointLightVPTListBuffer);

//GBuffer����------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	Num1 = GeometryList.size() / RealThreadNum;                           //ÿ���߳���С��������
	Num2 = GeometryList.size() % RealThreadNum;                           //�����1��������߳�����

	iter = GeometryList.begin();

	ComPtr<ID3D12CommandAllocator> GBufferPreCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> GBufferPreCmdList = CreateCmdList(GBufferPreCmdAllocator, L"GBufferPreCmd");

	//����GBuffer����״̬��D3D12_RESOURCE_STATE_RENDER_TARGET
	int GBufferNum = DrawResourceList[drawIndex]->RTBList.size() - 2;        //GBuffer����
	vector<CD3DX12_RESOURCE_BARRIER> GBufferBarrierListRT;                     //GBuffer��������
	for (int i = 0; i < GBufferNum; i++)
	{
		CD3DX12_RESOURCE_BARRIER GBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			DrawResourceList[drawIndex]->RTBList[i + 1].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
		GBufferBarrierListRT.push_back(GBufferBarrier);
	}
	GBufferPreCmdList->ResourceBarrier(GBufferNum, GBufferBarrierListRT.data());

	//����GBuffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE GBufferRTVHandle(DrawResourceList[drawIndex]->GBufferRTVHeapHandle);
	float GBufferClearValue[4] = { 0,0,0,0 };
	for (int i = 0; i < GBufferNum; i++)
	{
		GBufferPreCmdList->ClearRenderTargetView(GBufferRTVHandle, GBufferClearValue, 0, nullptr);
		GBufferRTVHandle.Offset(1, RTVSize);
	}

	ExeCmd(GBufferPreCmdList, MainCmdQueue);

	DrawResourceList[drawIndex]->TempAllocatorList.push_back(GBufferPreCmdAllocator);

	//����GBuffer
	for (int i = 0; i < RealThreadNum; i++)
	{
		if (iter == GeometryList.end())
		{
			//֪ͨ���߳����������ύ���
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

	//�ȴ����л����߳�ִ�����
	{
		unique_lock<mutex> ul(DrawResourceList[drawIndex]->DrawThreadNumMutex);
		DrawResourceList[drawIndex]->DrawCompleteCV.wait(ul, [this, drawIndex] {return DrawResourceList[drawIndex]->DrawThreadCompleteNum == RealThreadNum; });
		DrawResourceList[drawIndex]->DrawThreadCompleteNum = 0;
	}

	//����GBuffer����״̬��D3D12_RESOURCE_STATE_COMMON
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

/*COMMON�ǻ���������-------------------------------------------------------------------------------*/
	//Num1 = GeometryList.size() / RealThreadNum;                           //ÿ���߳���С��������
	//Num2 = GeometryList.size() % RealThreadNum;                           //�����1��������߳�����
	//
	//iter = GeometryList.begin();
	//
	//for (int i = 0; i < RealThreadNum; i++)
	//{
	//	if (iter == GeometryList.end())
	//	{
	//		//֪ͨ���߳����������ύ���
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
	////�ȴ����л����߳�ִ�����
	//{
	//	unique_lock<mutex> ul(DrawResourceList[drawIndex]->DrawThreadNumMutex);
	//	DrawResourceList[drawIndex]->DrawCompleteCV.wait(ul, [this, drawIndex] {return DrawResourceList[drawIndex]->DrawThreadCompleteNum == RealThreadNum; });
	//	DrawResourceList[drawIndex]->DrawThreadCompleteNum = 0;
	//}

//SSAO(HBAO)
	ComPtr<ID3D12CommandAllocator> SSAOCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> SSAOCmdList = CreateCmdList(SSAOCmdAllocator, L"SSAOCmd");

	//����SSAOBuffer����״̬��D3D12_RESOURCE_STATE_RENDER_TARGET
	CD3DX12_RESOURCE_BARRIER SSAOBufferBarrierRT = CD3DX12_RESOURCE_BARRIER::Transition(
		DrawResourceList[drawIndex]->RTBList[4].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
	SSAOCmdList->ResourceBarrier(1, &SSAOBufferBarrierRT);

	//����SSAOBuffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE SSAORTVHandle(DrawResourceList[drawIndex]->SSAORTVHeapHandle);
	float SSAOBufferClearValue[4] = { 0,0,0,0 };
	SSAOCmdList->ClearRenderTargetView(SSAORTVHandle, SSAOBufferClearValue, 0, nullptr);

	//
	SetViewPortAndSciRect(SSAOCmdList);
	SSAOCmdList->SetPipelineState(PSOList["PSOSSAO"].Get());
	SSAOCmdList->SetGraphicsRootSignature(RootSigList["SSAORootSig"].Get());
	SSAOCmdList->OMSetRenderTargets(1, &SSAORTVHandle, TRUE, nullptr);

	//����������
	ID3D12DescriptorHeap* SSAODescHeaps[] = { DrawResourceList[drawIndex]->GBufferCSUHeap.Get(),DrawResourceList[drawIndex]->DeferredRenderSamplerDescHeap.Get() };
	SSAOCmdList->SetDescriptorHeaps(2, SSAODescHeaps);

	//���ø�ǩ����Ӧ��Դ
	SSAOCmdList->SetGraphicsRootDescriptorTable(0, DrawResourceList[drawIndex]->GBufferCSUHeap->GetGPUDescriptorHandleForHeapStart());
	SSAOCmdList->SetGraphicsRootDescriptorTable(1, DrawResourceList[drawIndex]->DeferredRenderSamplerDescHeap->GetGPUDescriptorHandleForHeapStart());

	//ָ��ͼԪ����
	SSAOCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//����
	SSAOCmdList->DrawInstanced(3, 1, 0, 0);

	//����SSAOBuffer����״̬��D3D12_RESOURCE_STATE_COMMON
	CD3DX12_RESOURCE_BARRIER SSAOBufferBarrierCommon = CD3DX12_RESOURCE_BARRIER::Transition(
		DrawResourceList[drawIndex]->RTBList[4].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
	SSAOCmdList->ResourceBarrier(1, &SSAOBufferBarrierCommon);

	ExeCmd(SSAOCmdList, MainCmdQueue);

	DrawResourceList[drawIndex]->TempAllocatorList.push_back(SSAOCmdAllocator);

//��AOͼ����ģ������
	ComPtr<ID3D12CommandAllocator> AOBufferBlurCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> AOBufferBlurCmdList = CreateCmdList(AOBufferBlurCmdAllocator, L"AOBufferBlur");

	//��ģ��
	AOBufferBlurCmdList->SetPipelineState(PSOList["PSOAOBufferBlurRow"].Get());
	AOBufferBlurCmdList->SetComputeRootSignature(RootSigList["AOBlur"].Get());
	ID3D12DescriptorHeap* AOBlurDescHeaps[] = { DrawResourceList[drawIndex]->AOBlurCSUHeap.Get() };
	AOBufferBlurCmdList->SetDescriptorHeaps(1, AOBlurDescHeaps);
	AOBufferBlurCmdList->SetComputeRootDescriptorTable(0, DrawResourceList[drawIndex]->AOBlurCSUHeap->GetGPUDescriptorHandleForHeapStart());
	AOBufferBlurCmdList->Dispatch(ClientHeight, 1, 1);                                   //�����߳�

	//��ģ��
	AOBufferBlurCmdList->SetPipelineState(PSOList["PSOAOBufferBlurCol"].Get());
	AOBufferBlurCmdList->SetComputeRootSignature(RootSigList["AOBlur"].Get());
	//AOBufferBlurCmdList->SetDescriptorHeaps(1, AOBlurDescHeaps);
	AOBufferBlurCmdList->SetComputeRootDescriptorTable(0, DrawResourceList[drawIndex]->AOBlurCSUHeap->GetGPUDescriptorHandleForHeapStart());
	AOBufferBlurCmdList->Dispatch(ClientWidth, 1, 1);                                   //�����߳�

	ExeCmd(AOBufferBlurCmdList, MainCmdQueue);

	DrawResourceList[drawIndex]->TempAllocatorList.push_back(AOBufferBlurCmdAllocator);


//�ӳ���Ⱦ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	ComPtr<ID3D12CommandAllocator> DeferredRenderCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> DeferredRenderCmdList = CreateCmdList(DeferredRenderCmdAllocator, L"DeferredRenderCmd");

	SetViewPortAndSciRect(DeferredRenderCmdList);
	DeferredRenderCmdList->SetPipelineState(PSOList["PSODeferredRender"].Get());
	DeferredRenderCmdList->SetGraphicsRootSignature(RootSigList["DeferredRenderRootSig"].Get());
	DeferredRenderCmdList->OMSetRenderTargets(1, &DrawResourceList[drawIndex]->RTVHeapHandle, TRUE, nullptr);

	//����������
	ID3D12DescriptorHeap* DeferredRenderDescHeaps[] = { DrawResourceList[drawIndex]->GBufferCSUHeap.Get(),DrawResourceList[drawIndex]->DeferredRenderSamplerDescHeap.Get() };
	DeferredRenderCmdList->SetDescriptorHeaps(2, DeferredRenderDescHeaps);

	//���ø�ǩ����Ӧ��Դ
	DeferredRenderCmdList->SetGraphicsRootDescriptorTable(0, DrawResourceList[drawIndex]->GBufferCSUHeap->GetGPUDescriptorHandleForHeapStart());
	DeferredRenderCmdList->SetGraphicsRootDescriptorTable(1, DrawResourceList[drawIndex]->DeferredRenderSamplerDescHeap->GetGPUDescriptorHandleForHeapStart());

	//ָ��ͼԪ����
	DeferredRenderCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//����
	DeferredRenderCmdList->DrawInstanced(3, 1, 0, 0);

	ExeCmd(DeferredRenderCmdList, MainCmdQueue);

	DrawResourceList[drawIndex]->TempAllocatorList.push_back(DeferredRenderCmdAllocator);

//������
	//tone mapping
	//ComPtr<ID3D12CommandAllocator> ToneMappingAllocator;
	//ComPtr<ID3D12GraphicsCommandList> ToneMappingCmdList = CreateCmdList(ToneMappingAllocator);
	//
	//ToneMappingCmdList->Dispatch(32, 1, 1);                               //�����߳�

//��������------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	ComPtr<ID3D12CommandAllocator> EndCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> EndCmdList = CreateCmdList(EndCmdAllocator, L"EmdCmd");

	//MSAA��δ���������ã�
	{
		////ʵ����ȾĿ�껺����תΪ������Դ״̬
		//CD3DX12_RESOURCE_BARRIER RealBufferBarrierResolve = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->RTBList[0].Get(),
		//	D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
		//EndCmdList->ResourceBarrier(1, &RealBufferBarrierResolve);

		////��̨������תΪ������̬
		//CD3DX12_RESOURCE_BARRIER BackBufferBarrierResolveDest = CD3DX12_RESOURCE_BARRIER::Transition(SwapChainBuffer[BackBufferIndex].Get(),
		//	D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST);
		//EndCmdList->ResourceBarrier(1, &BackBufferBarrierResolveDest);

		////MSAA����
		//EndCmdList->ResolveSubresource(SwapChainBuffer[BackBufferIndex].Get(), 0, DrawResourceList[drawIndex]->RTBList[0].Get(), 0, RTFormat);

		////ʵ����ȾĿ�껺����תΪcommon״̬ D3D12_RESOURCE_STATE_COMMON
		//CD3DX12_RESOURCE_BARRIER RealBufferBarrierCommon = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->RTBList[0].Get(),
		//	D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_COMMON);
		//EndCmdList->ResourceBarrier(1, &RealBufferBarrierCommon);

		////����������Ѿ��
		////CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart(), BackBufferIndex, RTVSize);

		////��̨�������ӱ�����̬תΪ����״̬
		//CD3DX12_RESOURCE_BARRIER BackBufferBarrierPresent = CD3DX12_RESOURCE_BARRIER::Transition(SwapChainBuffer[BackBufferIndex].Get(),
		//	D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT);
	}

	//��ʵ����ȾĿ�껺�������Ƶ���̨������
	{
	//״̬ת��
		//ʵ����ȾĿ�껺����תΪ����Դ״̬
		CD3DX12_RESOURCE_BARRIER RealBufferBarrierCopySource = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->RTBList[0].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
		EndCmdList->ResourceBarrier(1, &RealBufferBarrierCopySource);

		//��̨������תΪ����Ŀ��̬
		CD3DX12_RESOURCE_BARRIER BackBufferBarrierResolveDest = CD3DX12_RESOURCE_BARRIER::Transition(SwapChainBuffer[BackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
		EndCmdList->ResourceBarrier(1, &BackBufferBarrierResolveDest);

	//����
		//��������Դ
		D3D12_TEXTURE_COPY_LOCATION CopySrc;
		CopySrc.pResource = DrawResourceList[drawIndex]->RTBList[0].Get();
		CopySrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		CopySrc.SubresourceIndex = 0;

		//��������Ŀ��
		D3D12_TEXTURE_COPY_LOCATION CopyDst;
		CopyDst.pResource = SwapChainBuffer[BackBufferIndex].Get();
		CopyDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		CopyDst.SubresourceIndex = 0;

		//����
		EndCmdList->CopyTextureRegion(&CopyDst, 0, 0, 0, &CopySrc, NULL);

	//״̬ת��
		//ʵ����ȾĿ�껺����תΪcommon״̬ D3D12_RESOURCE_STATE_COMMON
		CD3DX12_RESOURCE_BARRIER RealBufferBarrierCommon = CD3DX12_RESOURCE_BARRIER::Transition(DrawResourceList[drawIndex]->RTBList[0].Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
		EndCmdList->ResourceBarrier(1, &RealBufferBarrierCommon);

		//��̨�������ӱ�����̬תΪ����״̬
		CD3DX12_RESOURCE_BARRIER BackBufferBarrierPresent = CD3DX12_RESOURCE_BARRIER::Transition(SwapChainBuffer[BackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

		EndCmdList->ResourceBarrier(1, &BackBufferBarrierPresent);
	}

	ExeCmd(EndCmdList, MainCmdQueue);

	//��ֹ�������������ǰ����
	DrawResourceList[drawIndex]->TempAllocatorList.push_back(EndCmdAllocator);

	//����ͼ�񲢽���ǰ��̨������
	SwapChain->Present(0, 0);

	BackBufferIndex = (BackBufferIndex + 1) % 2;

//���ƽ���
	//֪ͨ���ƽ���
	FluCmdQueue(MainCmdQueue, DrawCompleteEvent);
}

void D3DAPP::DrawGeometries(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawIndex, int geoNum)
{
	//���λ��ƶ������
	for (int i = 0; i < geoNum; i++)
	{
		shared_ptr<GEOMETRY> TempGeo = iter->second;
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->GeoListMutex);
			DrawResourceList[drawIndex]->TempGeoList.push_back(TempGeo);
		}

		//���������б�������
		ComPtr<ID3D12CommandAllocator> Allocator;
		ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(Allocator, L"DrawGeometry");

		SetViewPortAndSciRect(CmdList);

		//����ˮ��״̬
		CmdList->SetPipelineState(PSOList[GetPSOIndex(TempGeo)].Get());

		//�󶨸�ǩ��
		CmdList->SetGraphicsRootSignature(RootSigList[GetRootSigIndex(TempGeo)].Get());

		//���Ʋ�ִ��
		TempGeo->DrawGeometry(CmdList, DrawResourceList[drawIndex], drawIndex);
		ExeCmd(CmdList, MainCmdQueue);

		//��ֹ�������������ǰ����
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->AllocatorMutex);
			DrawResourceList[drawIndex]->TempAllocatorList.push_back(Allocator);
		}
		
		iter++;
	}
	
	//֪ͨ���߳����������ύ���
	{
		lock_guard<mutex> lg(DrawResourceList[drawIndex]->DrawThreadNumMutex);
		DrawResourceList[drawIndex]->DrawThreadCompleteNum++;
	}
	DrawResourceList[drawIndex]->DrawCompleteCV.notify_all();
}

void D3DAPP::DrawShadowMaps(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawIndex, int geoNum)
{
	//���λ��ƶ������

	for (int i = 0; i < geoNum; i++)
	{
		shared_ptr<GEOMETRY> TempGeo = iter->second;
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->GeoListMutex);
			DrawResourceList[drawIndex]->TempGeoList.push_back(TempGeo);
		}

		//���������б�������
		ComPtr<ID3D12CommandAllocator> Allocator;
		ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(Allocator, L"DrawShadowMap");

		//�����ӿ�
		D3D12_VIEWPORT ShadowMapViewPort;
		ShadowMapViewPort.TopLeftX = 0;                                                   //�ӿ����϶����ں�̨��������x����
		ShadowMapViewPort.TopLeftY = 0;                                                   //���ϣ�y����
		ShadowMapViewPort.Width = static_cast<float>(ShadowWidht);                        //�ӿڿ��
		ShadowMapViewPort.Height = static_cast<float>(ShadowHeight);                      //�ӿڸ߶�
		ShadowMapViewPort.MinDepth = 0.0f;                                                //���ֵӳ��������˵�ֵ
		ShadowMapViewPort.MaxDepth = 1.0f;                                                //���ֵӳ��������Ҷ˵�ֵ

		CmdList->RSSetViewports(1, &ShadowMapViewPort);

		//���òü�����
		D3D12_RECT ShadowMapSciRect;
		ShadowMapSciRect.left = 0;                                                        //�ü����������Ͻ�x����
		ShadowMapSciRect.top = 0;                                                         //���½�y����
		ShadowMapSciRect.right = ShadowWidht;                                             //���½�x����
		ShadowMapSciRect.bottom = ShadowHeight;                                           //���½�y����

		CmdList->RSSetScissorRects(1, &ShadowMapSciRect);

		//����ˮ��״̬
		CmdList->SetPipelineState(PSOList[GetShadowMapPSOIndex(TempGeo)].Get());

		//�󶨸�ǩ��
		CmdList->SetGraphicsRootSignature(RootSigList[GetShadowMapRootSigIndex(TempGeo)].Get());

		//���Ʋ�ִ��
		TempGeo->DrawShadowMap(CmdList, DrawResourceList[drawIndex], drawIndex);
		ExeCmd(CmdList, MainCmdQueue);

		//��ֹ�������������ǰ����
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->AllocatorMutex);
			DrawResourceList[drawIndex]->TempAllocatorList.push_back(Allocator);
		}

		iter++;
	}

	//֪ͨ���߳����������ύ���
	{
		lock_guard<mutex> lg(DrawResourceList[drawIndex]->DrawThreadNumMutex);
		DrawResourceList[drawIndex]->DrawThreadCompleteNum++;
	}
	DrawResourceList[drawIndex]->DrawCompleteCV.notify_all();
}

void D3DAPP::DrawGBuffer(unordered_map<wstring, shared_ptr<GEOMETRY>>::iterator iter, int drawIndex, int geoNum)
{
	//���λ��ƶ������
	for (int i = 0; i < geoNum; i++)
	{
		shared_ptr<GEOMETRY> TempGeo = iter->second;
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->GeoListMutex);
			DrawResourceList[drawIndex]->TempGeoList.push_back(TempGeo);
		}

		//���������б�������
		ComPtr<ID3D12CommandAllocator> Allocator;
		ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(Allocator, L"DrawGBuffer");

		SetViewPortAndSciRect(CmdList);

		//����ˮ��״̬
		CmdList->SetPipelineState(PSOList[GetGBufferPSOIndex(TempGeo)].Get());

		//�󶨸�ǩ��
		CmdList->SetGraphicsRootSignature(RootSigList[GetRootSigIndex(TempGeo)].Get());

		//���Ʋ�ִ��
		TempGeo->DrawGBuffer(CmdList, DrawResourceList[drawIndex], drawIndex);
		ExeCmd(CmdList, MainCmdQueue);

		//��ֹ�������������ǰ����
		{
			lock_guard<mutex> lg(DrawResourceList[drawIndex]->AllocatorMutex);
			DrawResourceList[drawIndex]->TempAllocatorList.push_back(Allocator);
		}

		iter++;
	}

	//֪ͨ���߳����������ύ���
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
//������
	//�������
	case WM_LBUTTONDOWN:
	{
		//������ǰ���λ��
		CurrentX = GET_X_LPARAM(lparam);
		CurrentY = GET_Y_LPARAM(lparam);

		//������꣨�����ڴ����⣩
		SetCapture(hwnd);

		break;
	}
	//����ɿ�
	case WM_LBUTTONUP:
	{
		ReleaseCapture();
		break;
	}
	//����ƶ�
	case WM_MOUSEMOVE:
	{
		if (wparam & WM_LBUTTONDOWN)
		{
			int x = GET_X_LPARAM(lparam);
			int y = GET_Y_LPARAM(lparam);

			//����ˮƽ����ֵ�����ƶ�
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
//���̲���
	//����
	case WM_KEYDOWN:
	{
		switch (wparam)
		{
		//��W
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
		//��S
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
		//��D
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
		//��A
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
		//��E
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
		//��Q
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
	//�ɿ���
	case WM_KEYUP:
	{
		switch (wparam)
		{
		//��W
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
		//��S
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
		//��D
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
		//��A
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
		//��E
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
		//��Q
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
//����
	//���䴰�ڴ�С
	case WM_SIZE:
	{
		//�������´��ڴ�С
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
/*�����ݣ�0-4������������5-9��ɫ����Դ,10-14
* 0���任����֡��Դ������
* 1��ȫ��֡��Դ������
* 5�����Դ����
* 6������ɫ��ͼ
* 7��ʵ������
*/

//������������
	int ViewNum = 15;
	ComPtr<ID3D12DescriptorHeap> DescHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, ViewNum);

	//�������Ѿ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE HeapHandle(                             //���RTV���׾��
		DescHeap->GetCPUDescriptorHandleForHeapStart());

//������0��


//������1��ȫ��֡��Դ������������
	auto HeapHandle1 = HeapHandle;
	HeapHandle1.Offset(1, CSUSize);
	Device->CreateConstantBufferView(&DrawResourceList[drawIndex]->FrameBufferViewDesc, HeapHandle1);

//������2��
	auto HeapHandle2 = HeapHandle;
	HeapHandle2.Offset(2, CSUSize);
	Device->CreateConstantBufferView(&DrawResourceList[drawIndex]->AmbParaLightBufferViewDesc, HeapHandle2);

//������3��
	//auto HeapHandle3 = HeapHandle;
	//HeapHandle3.Offset(3, CSUSize);
	//Device->CreateConstantBufferView(, HeapHandle3);

////������4��
//	HeapHandle.Offset(1, CSUSize);

//������5��
	auto HeapHandle5 = HeapHandle;
	HeapHandle5.Offset(5, CSUSize);
	Device->CreateShaderResourceView(DrawResourceList[drawIndex]->PointLightBuffer.Get(), &DrawResourceList[drawIndex]->PointLightSRVDesc, HeapHandle5);

////������6��
//	HeapHandle.Offset(1, CSUSize);
//
////������7��
//	HeapHandle.Offset(1, CSUSize);
//
//������8��
	auto HeapHandle8 = HeapHandle;
	HeapHandle8.Offset(8, CSUSize);
	Device->CreateShaderResourceView(DrawResourceList[drawIndex]->ShadowMapBufferArray.Get(), &DrawResourceList[drawIndex]->ShadowMapSRVDesc, HeapHandle8);

//������9��
	auto HeapHandle9 = HeapHandle;
	HeapHandle9.Offset(9, CSUSize);
	Device->CreateShaderResourceView(DrawResourceList[drawIndex]->PointLightVPTListBuffer.Get(), &DrawResourceList[drawIndex]->PointLightVPTListSRVDesc, HeapHandle9);

//������10��

	return DescHeap;
}

ComPtr<ID3D12DescriptorHeap> D3DAPP::CreateDefalutSamplerHeap(int drawIndex)
{
/*������:
*/
	int ViewNum = 15;
	ComPtr<ID3D12DescriptorHeap> DescHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, ViewNum);

	CD3DX12_CPU_DESCRIPTOR_HANDLE HeapHandle(                             //���RTV���׾��
		DescHeap->GetCPUDescriptorHandleForHeapStart());

	//������Ӱ��ͼ������
	auto HeapHandle1 = HeapHandle;
	HeapHandle1.Offset(1, CSUSize);
	auto& ShadowMapSamplerDesc = SamplerDescList["ShadowSampler"];
	Device->CreateSampler(&ShadowMapSamplerDesc, HeapHandle1);

	//����SSAO����Ӱ��ͼ������
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

	//����´��ڴ�С
	{
		unique_lock<mutex> guard(WndResizeMutex);
		WndResizeCV.wait(guard, [this] {return WndResize; });
		width = NewWidth;
		height = NewHeight;
	}

	//�ı䴰�ڴ�С
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

	//����ǰ���ƶ�����
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

	//���������ƶ�����
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

	//���������ƶ�����
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

	//�ƶ����
	Move(ForwardDistance.get(), RightwardDistance.get(), UpwardDistance.get());
}

void D3DAPP::CreateGeometry(wstring name, GEOMETRY_TYPE type, MESH_TYPE meshType, wstring materialName, vector<GEOMETRY_FRAME_DATA> geoFrameData)
{
	if (GeometryList.find(name) != GeometryList.end())                  //����������Ƿ����
	{
#ifndef NDEBUG
		wcout.imbue(locale("", LC_CTYPE));
		wcout << "�½�����ʧ�ܣ����֡� " << name << " ���Ѵ���" << endl;
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
			cout << "�½�����ʧ�ܣ����Ͳ�����" << endl;
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
		cout << "�½�����ʧ�ܣ��������Ͳ�����" << endl;
		break;
	}
	}
}

void D3DAPP::InsertGeometry(wstring name, shared_ptr<GEOMETRY> geometry)
{
	//������������������������������
	ComPtr<ID3D12Resource> VertexBuffer = CreateDefaultBuffer(geometry->GetVerticesSize(), geometry->GetVerticesAddr());
	ComPtr<ID3D12Resource> IndexBuffer = CreateDefaultBuffer(geometry->GetIndicesSize(), geometry->GetIndicesAddr());

	geometry->CreateVerticesResource(VertexBuffer);
	geometry->CreateIndicesResource(IndexBuffer);

	//��������
	GeometryList.insert({ name,geometry });

#ifndef NDEBUG
	wcout.imbue(locale("", LC_CTYPE));
	wcout << L"���壺�� " << name << L"�������ɹ�" << endl;
#endif
}

void D3DAPP::InsertBlendGeometry(wstring name, shared_ptr<GEOMETRY> geometry)
{
	//������������������������������
	ComPtr<ID3D12Resource> VertexBuffer = CreateDefaultBuffer(geometry->GetVerticesSize(), geometry->GetVerticesAddr());
	ComPtr<ID3D12Resource> IndexBuffer = CreateDefaultBuffer(geometry->GetIndicesSize(), geometry->GetIndicesAddr());

	geometry->CreateVerticesResource(VertexBuffer);
	geometry->CreateIndicesResource(IndexBuffer);

	//��������
	BlendGeometryList.insert({ name,geometry });

#ifndef NDEBUG
	wcout.imbue(locale("", LC_CTYPE));
	wcout << L"������壺�� " << name << L"�������ɹ�" << endl;
#endif
}

shared_ptr<GEOMETRY> D3DAPP::CreateCommon(wstring name, wstring materialName, vector<GEOMETRY_FRAME_DATA> geoFrameData)
{
	shared_ptr<COMMON> CommonGeometry = make_shared<COMMON>(name, geoFrameData);
	
	auto& TextureStruct = TextureList[materialName.data()];
	auto& SamplerDesc = SamplerDescList["DEFSampler"];

	//��������������Դ
	GEOMETRY_DRAW_RESOURCE* ResourceList = CommonGeometry->GetDrawResource();
	for (int i = 0; i < 3; i++)
	{
		//���������Դ
		ResourceList[i].Texture = TextureStruct.Texture;

		//������������
		ResourceList[i].CSUDescHeap = CreateDefalutCSUDescHeap(i);
		ResourceList[i].SamplerDescHeap = CreateDefalutSamplerHeap(i);

	//���CSU�������Ѿ��
		CD3DX12_CPU_DESCRIPTOR_HANDLE CSUHeapHandle(ResourceList[i].CSUDescHeap->GetCPUDescriptorHandleForHeapStart());

		//����������Դ��������
		auto Handle6 = CSUHeapHandle;
		Handle6.Offset(6, CSUSize);
		Device->CreateShaderResourceView(ResourceList[i].Texture.Get(), &TextureStruct.SRVDesc, Handle6);

		//����ʵ��������
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
		SRVDESC.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;          //������������������������
		SRVDESC.Buffer = BS;

		Device->CreateShaderResourceView(ResourceList[i].GeoFrameResource.Get(), &SRVDESC, Handle7);

	//����������
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
		//���ڴ�С�Ƿ�ı�
		if (WndResize)
		{
			unique_lock<mutex> ul(DrawMutex);
			DrawCV.wait(ul, [this] {return CompleteDrawNum == 3; });
			ResizeWnd();
			WndResize = false;
		}

		//�ȴ�֡���пճ�
		unique_lock<mutex> ul(DrawMutex);
		DrawCV.wait(ul, [this] {return CompleteDrawNum > 0; });
		{
			lock_guard<mutex> lg(CompleteDrawNumMutex);
			CompleteDrawNum--;
		}

		//������Դ������
		UpdateDrawResource(DrawIndex);
		Draw(DrawIndex);

		//֡����
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


