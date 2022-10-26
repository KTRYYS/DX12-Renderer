#pragma once

#include"Macro.h"
#include"Struct_definition.h"

#include<vector>

#include"d3dx12.h"

#include"tool.h"
#include<mutex>

void GeometryInitialCube0(shared_ptr<GEOMETRY> geometry);
void GeometryInitialFlat0(shared_ptr<GEOMETRY> geometry);

class GEOMETRY
{
protected:
//物体属性
	wstring Name;                 //物体名
	GEOMETRY_TYPE Type;           //物体类型

//物体帧资源
	vector<GEOMETRY_FRAME_DATA> FrameDataList;
	mutex FrameDataListMutex;

	//最大实例数量
	int MaxInstanceNum = 100;

//顶点和索引
	//数据
	vector<VERTEX> Vertices;
	vector<uint16_t> Indices;

	//数据缓冲区
	ComPtr<ID3D12Resource> VerticesResource;
	ComPtr<ID3D12Resource> IndicesResource;

	//描述
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;

	//图元拓扑类型
	D3D_PRIMITIVE_TOPOLOGY TopologyType;

//每帧绘制资源
	GEOMETRY_DRAW_RESOURCE GeoDrawResourceList[3];

public:
	GEOMETRY(wstring name, GEOMETRY_TYPE type, vector<GEOMETRY_FRAME_DATA> frameDataList) :
		Name(name), Type(type), FrameDataList(move(frameDataList)) {}
	GEOMETRY(wstring name, GEOMETRY_TYPE type) : GEOMETRY(name, type, vector<GEOMETRY_FRAME_DATA>()) {}

	virtual ~GEOMETRY() = default;
	GEOMETRY(const GEOMETRY&) = delete;
	GEOMETRY& operator = (const GEOMETRY&) = delete;

	//返回物体名
	wstring GetName()
	{
		return Name;
	}

	//返回物体类型
	GEOMETRY_TYPE GetType()
	{
		return Type;
	}

//网格
	//顶点
	void SetVertices(vector<VERTEX>&& vertices)
	{
		Vertices = vertices;
	}
	void* GetVerticesAddr()                             //获得顶点数据地址
	{
		return Vertices.data();
	}
	int GetVerticesSize()                               //获得顶点数据大小     
	{
		return Vertices.size() * sizeof(VERTEX);
	}
	void CreateVerticesResource(ComPtr<ID3D12Resource> verticesResource)           //获得顶点缓冲区
	{
		VerticesResource = verticesResource;

		//完成顶点缓冲区描述符初始化
		VertexBufferView.BufferLocation = VerticesResource->GetGPUVirtualAddress();
	}

	//索引
	void SetIndices(vector<uint16_t>&& indices)
	{
		Indices = indices;
	}
	void* GetIndicesAddr()
	{
		return Indices.data();
	}
	int GetIndicesSize()
	{
		return Indices.size() * sizeof(VERTEX);
	}
	void CreateIndicesResource(ComPtr<ID3D12Resource> indicesResource)
	{
		IndicesResource = indicesResource;

		//完成索引缓冲区描述符初始化
		IndexBufferView.BufferLocation = IndicesResource->GetGPUVirtualAddress();
	}

	//资源初始化
	void VerAndIndResInitial()
	{
		//初始化顶点缓冲区描述符（未全部初始化）
		VertexBufferView.SizeInBytes = GetVerticesSize();
		VertexBufferView.StrideInBytes = (UINT)sizeof(VERTEX);

		//初始化索引缓冲区描述符（未全部初始化）
		IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = GetIndicesSize();
	}

//帧资源
	//加入新实例
	void PushInstance(GEOMETRY_FRAME_DATA frameData)
	{
		if(FrameDataList.size()<100)
		{
			lock_guard<mutex> lg(FrameDataListMutex);
			FrameDataList.push_back(frameData);
			GeoDrawResourceList[0].isInstanceChange = GeoDrawResourceList[1].isInstanceChange =
				GeoDrawResourceList[2].isInstanceChange = true;
		}
		else
		{
			cout << "实例数量已经最大" << endl;
		}
	}
	void PushInstance(XMFLOAT3 position, XMFLOAT3 angles, XMFLOAT3 scale, PBR_ITEM pBRItem);

	//返回最大实例数量
	int GetMaxInstanceNum()
	{
		return MaxInstanceNum;
	}

//帧资源
	//获得帧资源
	GEOMETRY_DRAW_RESOURCE* GetDrawResource()
	{
		return GeoDrawResourceList;
	}

	//更新资源
	virtual void UpdateFrameDataList(int drawIndex);

//绘制
	virtual void DrawGeometry(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex) = 0;
	virtual void DrawShadowMap(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex) = 0;
	virtual void DrawGBuffer(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex) = 0;
};

class COMMON :public GEOMETRY
{
public:
	COMMON(wstring name, vector<GEOMETRY_FRAME_DATA> frameDataList) : GEOMETRY(name, common, frameDataList)
	{
		//指定图元拓扑
		TopologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	COMMON(wstring name) : GEOMETRY(name, common)
	{
		//指定图元拓扑
		TopologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

	virtual ~COMMON() = default;

	virtual void DrawGeometry(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex);
	virtual void DrawShadowMap(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex);
	virtual void DrawGBuffer(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex);
};