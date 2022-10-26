#include"GEOMETRY.h"

void GEOMETRY::PushInstance(XMFLOAT3 position, XMFLOAT3 angles, XMFLOAT3 scale, PBR_ITEM pBRItem)
{
	if (FrameDataList.size() < 100)
	{
		GEOMETRY_FRAME_DATA FrameData;
		FrameData.PBRItem = pBRItem;

		//计算M矩阵
		auto WldMatrix = CalcuWorldMat(position, angles, scale);
		XMStoreFloat4x4(&FrameData.MMat, XMMatrixTranspose(WldMatrix));

		//加入
		{
			lock_guard<mutex> lg(FrameDataListMutex);
			FrameDataList.push_back(FrameData);
			GeoDrawResourceList[0].isInstanceChange = GeoDrawResourceList[1].isInstanceChange =
				GeoDrawResourceList[2].isInstanceChange = true;
		}
	}
	else
	{
		cout << "实例数量已经最大" << endl;
	}
}

void GEOMETRY::UpdateFrameDataList(int drawIndex)
{
	lock_guard<mutex> lg(FrameDataListMutex);

	if(GeoDrawResourceList[drawIndex].isInstanceChange)
	{
		UpdateUB(FrameDataList.data(), sizeof(GEOMETRY_FRAME_DATA) * FrameDataList.size(), GeoDrawResourceList[drawIndex].GeoFrameResource);
		GeoDrawResourceList[drawIndex].isInstanceChange = false;
		GeoDrawResourceList[drawIndex].InstanceNum = FrameDataList.size();
	}
	
}

//COMMON
void COMMON::DrawGeometry(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex)
{	
	//绑定RT和DS
	CmdList->OMSetRenderTargets(1, &drawResource->RTVHeapHandle, TRUE, &drawResource->DSVHeapHandle);

	//绑定描述符堆
	ID3D12DescriptorHeap* DescHeaps[] = { GeoDrawResourceList[drawIndex].CSUDescHeap.Get(),GeoDrawResourceList[drawIndex].SamplerDescHeap.Get() };
	CmdList->SetDescriptorHeaps(2, DescHeaps);

	//设置根签名对应资源
	CmdList->SetGraphicsRootDescriptorTable(0, GeoDrawResourceList[drawIndex].CSUDescHeap->GetGPUDescriptorHandleForHeapStart());
	CmdList->SetGraphicsRootDescriptorTable(1, GeoDrawResourceList[drawIndex].SamplerDescHeap->GetGPUDescriptorHandleForHeapStart());

	//绑定顶点缓冲区和索引缓冲区
	CmdList->IASetVertexBuffers(0, 1, &VertexBufferView);
	CmdList->IASetIndexBuffer(&IndexBufferView);

	//指定图元拓扑
	CmdList->IASetPrimitiveTopology(TopologyType);

	//绘制
	CmdList->DrawIndexedInstanced(Indices.size(), GeoDrawResourceList[drawIndex].InstanceNum, 0, 0, 0);
}

void COMMON::DrawShadowMap(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex)
{
//更新资源
	UpdateFrameDataList(drawIndex);

//绘制部分
	//绑定DS
	CmdList->OMSetRenderTargets(0, nullptr, false, &drawResource->ShadowMapDSHandle);
		
	//绑定描述符堆
	ID3D12DescriptorHeap* DescHeaps[] = { GeoDrawResourceList[drawIndex].CSUDescHeap.Get(),GeoDrawResourceList[drawIndex].SamplerDescHeap.Get() };
	CmdList->SetDescriptorHeaps(2, DescHeaps);

	//设置根签名对应资源
	CmdList->SetGraphicsRootDescriptorTable(0, GeoDrawResourceList[drawIndex].CSUDescHeap->GetGPUDescriptorHandleForHeapStart());
	CmdList->SetGraphicsRootConstantBufferView(1, drawResource->PointVPMatBufferAddr);

	//绑定顶点缓冲区和索引缓冲区
	CmdList->IASetVertexBuffers(0, 1, &VertexBufferView);
	CmdList->IASetIndexBuffer(&IndexBufferView);

	//指定图元拓扑
	CmdList->IASetPrimitiveTopology(TopologyType);

	//绘制
	CmdList->DrawIndexedInstanced(Indices.size(), GeoDrawResourceList[drawIndex].InstanceNum, 0, 0, 0);
}

//
void COMMON::DrawGBuffer(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex)
{
	//绑定RT和DS
	CmdList->OMSetRenderTargets(drawResource->GBufferNum, &drawResource->GBufferRTVHeapHandle, TRUE, &drawResource->DSVHeapHandle);

	//绑定描述符堆
	ID3D12DescriptorHeap* DescHeaps[] = { GeoDrawResourceList[drawIndex].CSUDescHeap.Get(),GeoDrawResourceList[drawIndex].SamplerDescHeap.Get() };
	CmdList->SetDescriptorHeaps(2, DescHeaps);

	//设置根签名对应资源
	CmdList->SetGraphicsRootDescriptorTable(0, GeoDrawResourceList[drawIndex].CSUDescHeap->GetGPUDescriptorHandleForHeapStart());
	CmdList->SetGraphicsRootDescriptorTable(1, GeoDrawResourceList[drawIndex].SamplerDescHeap->GetGPUDescriptorHandleForHeapStart());

	//绑定顶点缓冲区和索引缓冲区
	CmdList->IASetVertexBuffers(0, 1, &VertexBufferView);
	CmdList->IASetIndexBuffer(&IndexBufferView);

	//指定图元拓扑
	CmdList->IASetPrimitiveTopology(TopologyType);

	//绘制
	CmdList->DrawIndexedInstanced(Indices.size(), GeoDrawResourceList[drawIndex].InstanceNum, 0, 0, 0);
}

//MESH-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GeometryInitialCube0(shared_ptr<GEOMETRY> geometry)
{
	vector<VERTEX> Vertices =
	{
		//立方体前面：0 1 2 3
		VERTEX({ XMFLOAT3(+1.0f, +1.0f, -1.0f),Normalize({+1.0f, +0.f, -0.f}),XMFLOAT2(0.f,0.f) }),
		VERTEX({ XMFLOAT3(+1.0f, +1.0f, +1.0f),Normalize({+1.0f, +0.f, -0.f}),XMFLOAT2(1.f,0.f) }),
		VERTEX({ XMFLOAT3(+1.0f, -1.0f, -1.0f),Normalize({+1.0f, +0.f, -0.f}),XMFLOAT2(0.f,1.f) }),
		VERTEX({ XMFLOAT3(+1.0f, -1.0f, +1.0f),Normalize({+1.0f, +0.f, -0.f}),XMFLOAT2(1.f,1.f) }),

		//立方体左面：4 5 6 7
		VERTEX({ XMFLOAT3(-1.0f, +1.0f, -1.0f),Normalize({0.f, 0.f, -1.0f}),XMFLOAT2(0.f,0.f) }),
		VERTEX({ XMFLOAT3(+1.0f, +1.0f, -1.0f),Normalize({0.f, 0.f, -1.0f}),XMFLOAT2(1.f,0.f) }),
		VERTEX({ XMFLOAT3(-1.0f, -1.0f, -1.0f),Normalize({0.f, 0.f, -1.0f}),XMFLOAT2(0.f,1.f)}),
		VERTEX({ XMFLOAT3(+1.0f, -1.0f, -1.0f),Normalize({0.f, 0.f, -1.0f}),XMFLOAT2(1.f,1.f) }),

		//立方体右面：8 9 10 11
		VERTEX({ XMFLOAT3(+1.0f, +1.0f, +1.0f),Normalize({0.f, 0.f, +1.0f}),XMFLOAT2(0.f,0.f) }),
		VERTEX({ XMFLOAT3(-1.0f, +1.0f, +1.0f),Normalize({0.f, 0.f, +1.0f}),XMFLOAT2(1.f,0.f) }),
		VERTEX({ XMFLOAT3(+1.0f, -1.0f, +1.0f),Normalize({0.f, 0.f, +1.0f}),XMFLOAT2(0.f,1.f) }),
		VERTEX({ XMFLOAT3(-1.0f, -1.0f, +1.0f),Normalize({0.f, 0.f, +1.0f}),XMFLOAT2(1.f,1.f) }),

		//立方体上面：12 13 14 15
		VERTEX({ XMFLOAT3(+1.0f, +1.0f, -1.0f),Normalize({0.f, +1.0f, 0.f}),XMFLOAT2(0.f,0.f) }),
		VERTEX({ XMFLOAT3(-1.0f, +1.0f, -1.0f),Normalize({0.f, +1.0f, 0.f}),XMFLOAT2(1.f,0.f) }),
		VERTEX({ XMFLOAT3(+1.0f, +1.0f, +1.0f),Normalize({0.f, +1.0f, 0.f}),XMFLOAT2(0.f,1.f) }),
		VERTEX({ XMFLOAT3(-1.0f, +1.0f, +1.0f),Normalize({0.f, +1.0f, 0.f}),XMFLOAT2(1.f,1.f) }),

		//立方体下面：16 17 18 19
		VERTEX({ XMFLOAT3(+1.0f, -1.0f, -1.0f),Normalize({0.f, -1.0f, 0.f}),XMFLOAT2(0.f,0.f) }),
		VERTEX({ XMFLOAT3(-1.0f, -1.0f, -1.0f),Normalize({0.f, -1.0f, 0.f}),XMFLOAT2(0.f,1.f)}),
		VERTEX({ XMFLOAT3(+1.0f, -1.0f, +1.0f),Normalize({0.f, -1.0f, 0.f}),XMFLOAT2(1.f,0.f) }),
		VERTEX({ XMFLOAT3(-1.0f, -1.0f, +1.0f),Normalize({0.f, -1.0f, 0.f}),XMFLOAT2(1.f,1.f) }),

		//立方体后面:20 21 22 23
		VERTEX({ XMFLOAT3(-1.0f, +1.0f, +1.0f),Normalize({-1.0f, +0.f, -0.f}),XMFLOAT2(0.f,0.f) }),
		VERTEX({ XMFLOAT3(-1.0f, +1.0f, -1.0f),Normalize({-1.0f, +0.f, -0.f}),XMFLOAT2(1.f,0.f) }),
		VERTEX({ XMFLOAT3(-1.0f, -1.0f, +1.0f),Normalize({-1.0f, +0.f, -0.f}),XMFLOAT2(0.f,1.f) }),
		VERTEX({ XMFLOAT3(-1.0f, -1.0f, -1.0f),Normalize({-1.0f, +0.f, -0.f}),XMFLOAT2(1.f,1.f)})
	};

	vector<uint16_t> Indices =
	{
		//前面
		0, 1, 2,   2, 1, 3,
		//左面
		4, 5, 6,   6, 5, 7,
		//右面
		8, 9, 10,  10, 9, 11,
		//上面
		12, 13, 14,   14, 13, 15,
		//下面
		17, 16, 18,   17, 18, 19,
		//后面
		20, 21, 22,   22, 21, 23
	};

	geometry->SetVertices(move(Vertices));
	geometry->SetIndices(move(Indices));
	geometry->VerAndIndResInitial();
}

void GeometryInitialFlat0(shared_ptr<GEOMETRY> geometry)
{
	vector<VERTEX> Vertices =
	{
		VERTEX{XMFLOAT3{-1,0,-1},XMFLOAT3{0,1,0},XMFLOAT2{0,0}},
		VERTEX{XMFLOAT3{-1,0,+1},XMFLOAT3{0,1,0},XMFLOAT2{1,0}},
		VERTEX{XMFLOAT3{+1,0,-1},XMFLOAT3{0,1,0},XMFLOAT2{0,1}},
		VERTEX{XMFLOAT3{+1,0,+1},XMFLOAT3{0,1,0},XMFLOAT2{1,1}},
	};

	vector<uint16_t> Indices =
	{
		0, 1, 2,   2, 1, 3,
	};

	geometry->SetVertices(move(Vertices));
	geometry->SetIndices(move(Indices));
	geometry->VerAndIndResInitial();
}