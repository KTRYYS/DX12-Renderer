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
//��������
	wstring Name;                 //������
	GEOMETRY_TYPE Type;           //��������

//����֡��Դ
	vector<GEOMETRY_FRAME_DATA> FrameDataList;
	mutex FrameDataListMutex;

	//���ʵ������
	int MaxInstanceNum = 100;

//���������
	//����
	vector<VERTEX> Vertices;
	vector<uint16_t> Indices;

	//���ݻ�����
	ComPtr<ID3D12Resource> VerticesResource;
	ComPtr<ID3D12Resource> IndicesResource;

	//����
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;

	//ͼԪ��������
	D3D_PRIMITIVE_TOPOLOGY TopologyType;

//ÿ֡������Դ
	GEOMETRY_DRAW_RESOURCE GeoDrawResourceList[3];

public:
	GEOMETRY(wstring name, GEOMETRY_TYPE type, vector<GEOMETRY_FRAME_DATA> frameDataList) :
		Name(name), Type(type), FrameDataList(move(frameDataList)) {}
	GEOMETRY(wstring name, GEOMETRY_TYPE type) : GEOMETRY(name, type, vector<GEOMETRY_FRAME_DATA>()) {}

	virtual ~GEOMETRY() = default;
	GEOMETRY(const GEOMETRY&) = delete;
	GEOMETRY& operator = (const GEOMETRY&) = delete;

	//����������
	wstring GetName()
	{
		return Name;
	}

	//������������
	GEOMETRY_TYPE GetType()
	{
		return Type;
	}

//����
	//����
	void SetVertices(vector<VERTEX>&& vertices)
	{
		Vertices = vertices;
	}
	void* GetVerticesAddr()                             //��ö������ݵ�ַ
	{
		return Vertices.data();
	}
	int GetVerticesSize()                               //��ö������ݴ�С     
	{
		return Vertices.size() * sizeof(VERTEX);
	}
	void CreateVerticesResource(ComPtr<ID3D12Resource> verticesResource)           //��ö��㻺����
	{
		VerticesResource = verticesResource;

		//��ɶ��㻺������������ʼ��
		VertexBufferView.BufferLocation = VerticesResource->GetGPUVirtualAddress();
	}

	//����
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

		//���������������������ʼ��
		IndexBufferView.BufferLocation = IndicesResource->GetGPUVirtualAddress();
	}

	//��Դ��ʼ��
	void VerAndIndResInitial()
	{
		//��ʼ�����㻺������������δȫ����ʼ����
		VertexBufferView.SizeInBytes = GetVerticesSize();
		VertexBufferView.StrideInBytes = (UINT)sizeof(VERTEX);

		//��ʼ��������������������δȫ����ʼ����
		IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = GetIndicesSize();
	}

//֡��Դ
	//������ʵ��
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
			cout << "ʵ�������Ѿ����" << endl;
		}
	}
	void PushInstance(XMFLOAT3 position, XMFLOAT3 angles, XMFLOAT3 scale, PBR_ITEM pBRItem);

	//�������ʵ������
	int GetMaxInstanceNum()
	{
		return MaxInstanceNum;
	}

//֡��Դ
	//���֡��Դ
	GEOMETRY_DRAW_RESOURCE* GetDrawResource()
	{
		return GeoDrawResourceList;
	}

	//������Դ
	virtual void UpdateFrameDataList(int drawIndex);

//����
	virtual void DrawGeometry(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex) = 0;
	virtual void DrawShadowMap(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex) = 0;
	virtual void DrawGBuffer(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex) = 0;
};

class COMMON :public GEOMETRY
{
public:
	COMMON(wstring name, vector<GEOMETRY_FRAME_DATA> frameDataList) : GEOMETRY(name, common, frameDataList)
	{
		//ָ��ͼԪ����
		TopologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	COMMON(wstring name) : GEOMETRY(name, common)
	{
		//ָ��ͼԪ����
		TopologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

	virtual ~COMMON() = default;

	virtual void DrawGeometry(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex);
	virtual void DrawShadowMap(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex);
	virtual void DrawGBuffer(ComPtr<ID3D12GraphicsCommandList> CmdList, shared_ptr<DRAW_RESOURCE> drawResource, int drawIndex);
};