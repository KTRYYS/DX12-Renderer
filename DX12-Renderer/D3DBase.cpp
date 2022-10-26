#include"D3DBASE.h"

using namespace std;
using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

#include"tool.h"

D3DBASE::D3DBASE(HWND Hwnd, UINT Width, UINT Height) :
	hwnd(Hwnd), ClientWidth(Width), ClientHeight(Height)
{
#ifndef NDEBUG
	OpenDebug();
#endif

	InitialDXGIandDevice();

#ifndef NDEBUG
	LogAdapter();
#endif

	InitialFence();
	GetDescriptorSize();

	MainCmdQueue = CreateCmdQueue();

	CreateSwapChain();
	CreateDS();
}

void D3DBASE::OpenDebug()
{
	//�������Բ�
	D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController));
	DebugController->EnableDebugLayer();
	//DebugController->QueryInterface(IID_PPV_ARGS(&DebugController));

	//��������̨
	AllocConsole();
	FILE* stream1;
	freopen_s(&stream1, "CONOUT$", "w", stdout);
}

void D3DBASE::LogAdapter()
{
	//��ʾ��ǰ���������ʾ���������Կ���
	UINT AdapterIndex = 0;
	IDXGIAdapter* Adapter = nullptr;
	cout << "ϵͳ��ʾ�������б�" << endl;
	while (Factory->EnumAdapters(AdapterIndex, &Adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC Desc;
		Adapter->GetDesc(&Desc);
		wcout << Desc.Description << endl;
		AdapterIndex++;
	}
}

void D3DBASE::InitialDXGIandDevice()
{
	//��������
	ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&Factory)));

	//ѡ��������
	Factory->EnumAdapters(GPUIndex, Adapter.GetAddressOf());

	//�����豸
	ThrowIfFailed(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device)));

	//���GPU����
	GPUNodeMask = Device->GetNodeCount();

#ifndef NDEBUG
	//�����ѡ��������GPU����
	DXGI_ADAPTER_DESC AdapterDesc;
	ThrowIfFailed(Adapter->GetDesc(&AdapterDesc));
	wcout << AdapterDesc.Description << endl;
	cout << "GPU���룺" << GPUNodeMask << endl;
#endif // ! NDEBUG
}

void D3DBASE::InitialFence()
{
	//����Χ��
	ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));

	//��ʼ��Χ��ֵ
	FenceNum = 0;
}

void D3DBASE::GetDescriptorSize()
{
	//�����ȾĿ�껺������������С
	RTVSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//������ģ�建������������С
	DSVSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//���CBV��SRV��UAV��С
	CSUSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

ComPtr<ID3D12CommandQueue> D3DBASE::CreateCmdQueue(D3D12_COMMAND_LIST_TYPE cmdType, LPCWSTR name)
{
	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Type = cmdType;                                              //�����б����ͣ�ֱ�������б�ָ��GPU��ִ�е������б����������̳��κ�GPU״̬
	QueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;              //����������ȼ����������ȼ�
	QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;                       //��־��Ĭ���������
	QueueDesc.NodeMask = GPUNodeMask;

	ComPtr<ID3D12CommandQueue> CmdQueue;
	ThrowIfFailed(Device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CmdQueue)));       //�����������

	if(name != nullptr)
	{
		CmdQueue->SetName(name);
	}

	return CmdQueue;
}

//ComPtr<ID3D12GraphicsCommandList> D3DBASE::CreateCmdList(ComPtr<ID3D12CommandAllocator> &cmdAllocator, D3D12_COMMAND_LIST_TYPE cmdType, LPCWSTR name)
//{
//	ComPtr<ID3D12GraphicsCommandList> CmdList;
//
//	//�������������
//	ThrowIfFailed(Device->CreateCommandAllocator(cmdType,                  //�����б����ͣ�ֱ�������б�
//		IID_PPV_ARGS(&cmdAllocator)));
//
//	//���������б�
//	ThrowIfFailed(Device->CreateCommandList(GPUNodeMask,
//		cmdType,                                                           //�����б�����:ֱ�������б�
//		cmdAllocator.Get(),                                                //�������б���������������
//		nullptr,                                                           //�����б���Ⱦ��ˮ�߳�ʼ״̬��nullptrΪ����ʼ״̬
//		IID_PPV_ARGS(CmdList.GetAddressOf())));
//
//	if (name != nullptr)
//	{
//		cmdAllocator->SetName(name);
//		CmdList->SetName(name);
//	}
//
//	return CmdList;
//}

ComPtr<ID3D12GraphicsCommandList> D3DBASE::CreateCmdList(ComPtr<ID3D12CommandAllocator>& cmdAllocator, LPCWSTR name, D3D12_COMMAND_LIST_TYPE cmdType)
{
	ComPtr<ID3D12GraphicsCommandList> CmdList;

	//�������������
	ThrowIfFailed(Device->CreateCommandAllocator(cmdType,                  //�����б����ͣ�ֱ�������б�
		IID_PPV_ARGS(&cmdAllocator)));

	//���������б�
	ThrowIfFailed(Device->CreateCommandList(GPUNodeMask,
		cmdType,                                                           //�����б�����:ֱ�������б�
		cmdAllocator.Get(),                                                //�������б���������������
		nullptr,                                                           //�����б���Ⱦ��ˮ�߳�ʼ״̬��nullptrΪ����ʼ״̬
		IID_PPV_ARGS(CmdList.GetAddressOf())));

	if (name != nullptr)
	{
		cmdAllocator->SetName(name);
		CmdList->SetName(name);
	}

	return CmdList;
}

void D3DBASE::CmdReset(ComPtr<ID3D12GraphicsCommandList> &CmdList, ComPtr<ID3D12CommandAllocator> &CmdAllocator, ID3D12PipelineState* PSO)
{
	ThrowIfFailed(CmdAllocator->Reset());
	ThrowIfFailed(CmdList->Reset(CmdAllocator.Get(), PSO));
}

void D3DBASE::ExeCmd(ComPtr<ID3D12GraphicsCommandList>& CmdList, ComPtr<ID3D12CommandQueue> &CmdQueue)
{
	ThrowIfFailed(CmdList->Close());
	ID3D12CommandList* CmdLists[] = { CmdList.Get() };
	CmdQueue->ExecuteCommandLists(_countof(CmdLists), CmdLists);
}

ComPtr<ID3D12DescriptorHeap> D3DBASE::CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags, UINT DescNum)
{
	D3D12_DESCRIPTOR_HEAP_DESC DescHeapDesc;
	DescHeapDesc.Type = Type;
	DescHeapDesc.NumDescriptors = DescNum;
	DescHeapDesc.Flags = Flags;
	DescHeapDesc.NodeMask = GPUNodeMask;

	ComPtr<ID3D12DescriptorHeap> DescHeap;

	ThrowIfFailed(Device->CreateDescriptorHeap(&DescHeapDesc, IID_PPV_ARGS(DescHeap.GetAddressOf())));

	return DescHeap;
}

void D3DBASE::CreateSwapChain()
{
//����������
	//��������д�����������ṹ��
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};

	SwapChainDesc.BufferDesc.Width = ClientWidth;                            //�ֱ��ʿ��
	SwapChainDesc.BufferDesc.Height = ClientHeight;                          //�ֱ��ʸ߶�
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 0;                    //ˢ���ʷ�ĸ
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 0;                      //ˢ���ʷ���
	SwapChainDesc.BufferDesc.Format = RTFormat;                              //��������ʾ��ʽ
	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;       //δָ��ɨ����˳��
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;        //δָ������
	SwapChainDesc.SampleDesc.Count = 1;                                      //�������ظ���
	SwapChainDesc.SampleDesc.Quality = 0;                                    //������������
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = SwapChainBufferNumber;                        //����������
	SwapChainDesc.OutputWindow = hwnd;                                       //����Ȼ�������������ݰ󶨵�������
	SwapChainDesc.Windowed = true;                                           //����ģʽ��ʾ
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;            //���л�����ȫ��ģʽ

	//����������
	ThrowIfFailed(Factory->CreateSwapChain(MainCmdQueue.Get(), &SwapChainDesc, SwapChain.GetAddressOf()));

//��ȾĿ�껺��������
	//������������
	RTVHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, SwapChainBufferNumber);

	//��������RT�������Ͷ�Ӧ��RTV
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(                             //���RTV���׾��
		RTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < SwapChainBufferNumber; i++)                         //���δ�������RT�����������Ѷ�ӦRTV��������������
	{
		SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffer[i]));          //����RTB

		Device->CreateRenderTargetView(                                      //ΪRT����������������
			SwapChainBuffer[i].Get(),                                        //RT��Դ
			nullptr,                                                         //��Դ��Ԫ���������ͣ���ָ����ʽ����Ϊnullptr
			RTVHeapHandle);                                                  //Ҫ������RTV�����ڴ������CPU���������

		RTVHeapHandle.Offset(                                                //ƫ�ƾ����������һ���������ڴ�����Ŀ�ʼ
			1,                                                               //ƫ������ 
			RTVSize);                                                        //ƫ������С
	}
}

void D3DBASE::CreateDS()
{
	//��������дDS��������Դ�����ṹ��
	D3D12_RESOURCE_DESC DSDesc;
	DSDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;                   //��Դ����:2D����
	DSDesc.Alignment = 0;                                                    //���뷽ʽ
	DSDesc.Width = ClientWidth;                                              //��Դ���
	DSDesc.Height = ClientHeight;                                            //��Դ�߶�
	DSDesc.DepthOrArraySize = 1;                                             //��Դ���
	DSDesc.MipLevels = 1;                                                    //mipmap�㼶
	DSDesc.Format = DSFormat;                                                //��Դ��ʽ
	DSDesc.SampleDesc.Count = 1;
	DSDesc.SampleDesc.Quality = 0;
	DSDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;                            //������ѡ�����δ֪������������������
	DSDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;                  //����Ϊ��Դ�������ģ����ͼ

	//����DS������
	D3D12_CLEAR_VALUE ClearValue;                                            //���������Դ���Ż�ֵ
	ClearValue.Format = DSFormat;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES DSProperties(D3D12_HEAP_TYPE_DEFAULT);           //������Ҫ�����Ķ����ԣ�Ĭ�϶�

	ThrowIfFailed(Device->CreateCommittedResource(                           //����DS��������һ��Ĭ�϶ѣ�����ǰ��ӳ�䵽������
		&DSProperties,                                                       //�����ԣ�Ĭ�϶�
		D3D12_HEAP_FLAG_NONE,                                                //��־��Ĭ��ֵ
		&DSDesc,                                                             //������е���Դ��DS��������������
		D3D12_RESOURCE_STATE_COMMON,                                         //��Դ��ʼ״̬
		&ClearValue,                                                         //�����Դ�����ֵ
		IID_PPV_ARGS(DSBuffer.GetAddressOf())));

	//����DSV��
	DSVHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	//����DSV
	Device->CreateDepthStencilView(DSBuffer.Get(),                           //DS��Դ
		nullptr,                                                             //������Դ��ʽ����������Ϊ��ָ��
		DSVHeap->GetCPUDescriptorHandleForHeapStart());                      //DSV���׾��

	//����DS������״̬
	CD3DX12_RESOURCE_BARRIER DSBufferBarrier =                               //����DS��Դ����
		CD3DX12_RESOURCE_BARRIER::Transition(
			DSBuffer.Get(),                                                  //DS��Դ
			D3D12_RESOURCE_STATE_COMMON,                                     //��Դת��ǰ״̬
			D3D12_RESOURCE_STATE_DEPTH_WRITE);                               //��Դת����״̬

	ComPtr<ID3D12CommandAllocator> CmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(CmdAllocator);

	CmdList->ResourceBarrier(                                                //֪ͨ����ͬ����Դ����
		1,                                                                   //��������
		&DSBufferBarrier);                                                   //���������ṹ������

	ExeCmd(CmdList, MainCmdQueue);

	FluCmdQueue(MainCmdQueue);

	CmdList.~ComPtr();
	CmdAllocator.~ComPtr();
}

void D3DBASE::SetViewPortAndSciRect(ComPtr<ID3D12GraphicsCommandList>CmdList)
{
	//�����ӿ�
	ViewPort.TopLeftX = 0;                                                   //�ӿ����϶����ں�̨��������x����
	ViewPort.TopLeftY = 0;                                                   //���ϣ�y����
	ViewPort.Width = static_cast<float>(ClientWidth);                        //�ӿڿ��
	ViewPort.Height = static_cast<float>(ClientHeight);                      //�ӿڸ߶�
	ViewPort.MinDepth = 0.0f;                                                //���ֵӳ��������˵�ֵ
	ViewPort.MaxDepth = 1.0f;                                                //���ֵӳ��������Ҷ˵�ֵ

	CmdList->RSSetViewports(                                                 //�ӿ�����
		1, &ViewPort);                                                       //�ӿ������ṹ������

	//���òü�����
	SciRect.left = 0;                                                        //�ü����������Ͻ�x����
	SciRect.top = 0;                                                         //���½�y����
	SciRect.right = ClientWidth;                                             //���½�x����
	SciRect.bottom = ClientHeight;                                           //���½�y����

	CmdList->RSSetScissorRects(                                              //���òü�����
		1, &SciRect);                                                        //�ü������������ü����������ṹ������
}

ComPtr<ID3D12RootSignature> D3DBASE::CreateRootSig(D3D12_ROOT_SIGNATURE_DESC RootSigDesc)
{
	//���л���ǩ��
	ComPtr<ID3DBlob> Error = nullptr;                                      //��Ÿ�ǩ�����л�ʧ�ܵ���Ϣ
	ComPtr<ID3DBlob> SerializedRootSig = nullptr;                              //������л���ĸ�ǩ������

	ThrowIfFailed(D3D12SerializeRootSignature(                                               //���л���ǩ��
		&RootSigDesc,                                                          //��ǩ�������ṹ���ַ
		D3D_ROOT_SIGNATURE_VERSION_1,                                          //��ǩ���汾
		SerializedRootSig.GetAddressOf(), Error.GetAddressOf()));           //�������л���ĸ�ǩ��Դ���ݣ��ʹ�����Ϣ

	if (Error.Get())
	{
		cout << "��ǩ�����л����棺" << (char*)Error->GetBufferPointer() << endl;
	}

	//������ǩ��
	ComPtr<ID3D12RootSignature> RootSignature;

	ThrowIfFailed(Device->CreateRootSignature(GPUNodeMask,
		SerializedRootSig->GetBufferPointer(),                                 //���л����ǩ�����ݵ�ַ
		SerializedRootSig->GetBufferSize(),                                    //��ǩ��Դ���ݴ�С
		IID_PPV_ARGS(&RootSignature)));

	return RootSignature;
}

ComPtr<ID3D12PipelineState> D3DBASE::CreateBasicPSO(ComPtr<ID3D12RootSignature> RootSignature, D3D12_SHADER_BYTECODE VSByteCodeDesc, D3D12_SHADER_BYTECODE PSByteCodeDesc, D3D12_INPUT_LAYOUT_DESC IAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
	ComPtr<ID3D12PipelineState> PSO;

	//����PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

	PSODesc.pRootSignature = RootSignature.Get();                              //�󶨸�ǩ��
	PSODesc.VS = VSByteCodeDesc;                                               //�󶨶�����ɫ��
	PSODesc.PS = PSByteCodeDesc;                                               //��������ɫ��
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);          //ʹ��Ĭ�Ϲ�դ��
	//PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //�߿�ģʽ
	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //����ȫ��������
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                    //���״̬��Ĭ��
	PSODesc.SampleMask = UINT_MAX;                                             //���ز�����أ������в����㶼���в���
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);     //��ȡ�ģ��״̬��Ĭ��
	PSODesc.PrimitiveTopologyType = TopologyType;                              //ͼԪ�������ͣ�������
	PSODesc.RTVFormats[0] = RTFormat;                                          //��ȾĿ���ʽ������ֵ����
	PSODesc.NumRenderTargets = 1;                                              //��ȾĿ���ʽ����Ԫ������
	PSODesc.SampleDesc.Count = 1;                                              //ÿ�����ض��ز�����
	PSODesc.SampleDesc.Quality = 0;                                            //���ز�������
	PSODesc.DSVFormat = DSFormat;                                              //���ģ�建������ʽ
	PSODesc.InputLayout = IAInputDesc;                                         //���벼������

	//����PSO
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	return PSO;
}

ComPtr<ID3D12PipelineState> D3DBASE::CreateComputePSO(ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE cSByteCodeDesc,
	D3D12_CACHED_PIPELINE_STATE cachedPSO)
{
	ComPtr<ID3D12PipelineState> PSO;

	D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};
	PSODesc.pRootSignature = rootSignature.Get();
	PSODesc.CS = cSByteCodeDesc;
	PSODesc.CachedPSO = cachedPSO;
	PSODesc.NodeMask = GPUNodeMask;

#ifndef NDEBUG
	PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#endif

	ThrowIfFailed(Device->CreateComputePipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	return PSO;
}

//ComPtr<ID3D12PipelineState> D3DBASE::CreateTessPSO(ComPtr<ID3D12RootSignature> RootSignature, D3D12_INPUT_LAYOUT_DESC IAInputDesc,
//	D3D12_SHADER_BYTECODE VSByteCodeDesc, D3D12_SHADER_BYTECODE HSByteCodeDesc, D3D12_SHADER_BYTECODE DSByteCodeDesc,
//	D3D12_SHADER_BYTECODE PSByteCodeDesc, D3D12_SHADER_BYTECODE GSByteCodeDesc)
//{
//	ComPtr<ID3D12PipelineState> PSO;
//
//	//����PSO
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
//
//	PSODesc.pRootSignature = RootSignature.Get();
//	PSODesc.VS = VSByteCodeDesc;
//	PSODesc.PS = PSByteCodeDesc;
//	PSODesc.HS = HSByteCodeDesc;
//	PSODesc.DS = DSByteCodeDesc;
//	PSODesc.GS = GSByteCodeDesc;
//	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	//PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //�߿�ģʽ
//	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //����ȫ��������
//	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//	PSODesc.SampleMask = UINT_MAX;
//	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;       //ͼԪ�������ͣ�ƬԪ
//	PSODesc.RTVFormats[0] = RTFormat;
//	PSODesc.NumRenderTargets = 1;
//	PSODesc.SampleDesc.Count = 1;
//	PSODesc.SampleDesc.Quality = 0;
//	PSODesc.DSVFormat = DSFormat;
//	PSODesc.InputLayout = IAInputDesc;
//
//	//����PSO
//	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));
//
//	return PSO;
//}
//
//ComPtr<ID3D12PipelineState> D3DBASE::CreateWaterPSO(ComPtr<ID3D12RootSignature> RootSignature, D3D12_INPUT_LAYOUT_DESC IAInputDesc,
//	D3D12_SHADER_BYTECODE VSByteCodeDesc, D3D12_SHADER_BYTECODE HSByteCodeDesc, D3D12_SHADER_BYTECODE DSByteCodeDesc,
//	D3D12_SHADER_BYTECODE PSByteCodeDesc, D3D12_SHADER_BYTECODE GSByteCodeDesc)
//{
//	ComPtr<ID3D12PipelineState> PSO;
//
//	D3D12_RENDER_TARGET_BLEND_DESC BlendTargetDesc; 
//	BlendTargetDesc.BlendEnable = true;                                      //�����Ϲ��ܣ����ã��ͳ��߼��Ϲ��ܲ���ͬʱture��false
//	BlendTargetDesc.LogicOpEnable = false;                                   //�߼�������㹦�ܣ����ã��ͳ����Ϲ��ܲ���ͬʱture��false
//	BlendTargetDesc.SrcBlend = D3D12_BLEND_BLEND_FACTOR;                     //ָ��RGBԴ�Ļ������
//	BlendTargetDesc.DestBlend = D3D12_BLEND_INV_BLEND_FACTOR;                //ָ��RGBĿ��������
//	BlendTargetDesc.BlendOp = D3D12_BLEND_OP_ADD;                            //ָ����ϲ���
//	BlendTargetDesc.SrcBlendAlpha = D3D12_BLEND_BLEND_FACTOR;                //ָ��alphaԴ�������
//	BlendTargetDesc.DestBlendAlpha = D3D12_BLEND_INV_BLEND_FACTOR;           //ָ��alphaĿ��������
//	BlendTargetDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
//	BlendTargetDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
//	BlendTargetDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;          //���ƻ�Ϻ����ݿɱ�д���̨��������Щͨ��
//
//	D3D12_BLEND_DESC BlendDesc;
//	BlendDesc.AlphaToCoverageEnable = false;       //alpha-to-coverage���ܣ���ȾҶƬ���ŵ��������õĶ��ز������������ر�
//	BlendDesc.IndependentBlendEnable = false;      //false��������ȾĿ��ʹ��RenderTarget��һ��Ԫ�ؽ��л�ϣ�true��ÿ����ȾĿ��ʹ�ò�ͬ��ϣ����֧�ְ˸���
//	BlendDesc.RenderTarget[0] = BlendTargetDesc;   //��Ϸ�ʽ�����֧�ְ˸������IndependentBlendEnableΪfalse����ֻ�õ�һ��
//
//	//����PSO
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
//
//	PSODesc.pRootSignature = RootSignature.Get();
//	PSODesc.VS = VSByteCodeDesc;
//	PSODesc.PS = PSByteCodeDesc;
//	PSODesc.HS = HSByteCodeDesc;
//	PSODesc.DS = DSByteCodeDesc;
//	PSODesc.GS = GSByteCodeDesc;
//	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //�߿�ģʽ
//	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //����ȫ��������
//	PSODesc.BlendState = BlendDesc;
//	PSODesc.SampleMask = UINT_MAX;
//	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;       //ͼԪ�������ͣ�ƬԪ
//	PSODesc.RTVFormats[0] = RTFormat;
//	PSODesc.NumRenderTargets = 1;
//	PSODesc.SampleDesc.Count = 1;
//	PSODesc.SampleDesc.Quality = 0;
//	PSODesc.DSVFormat = DSFormat;
//	PSODesc.InputLayout = IAInputDesc;
//
//	//����PSO
//	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));
//
//	return PSO;
//}

void D3DBASE::ResizeRTandDS()
{

//�ؽ���ȾĿ�껺����
	//�ͷ�RTB
	for (int i = 0; i < SwapChainBufferNumber; i++)
	{
		SwapChainBuffer[i].Reset();
	}

	//���ý�����
	ThrowIfFailed(SwapChain->ResizeBuffers(SwapChainBufferNumber, ClientWidth, ClientHeight, RTFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	BackBufferIndex = 0;

	//����������Ѿ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());

	//������������������
	for (UINT i = 0; i < SwapChainBufferNumber; i++)
	{
		ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffer[i])));

		Device->CreateRenderTargetView(SwapChainBuffer[i].Get(), nullptr, RTVHeapHandle);

		RTVHeapHandle.Offset(1, RTVSize);
	}

//�ؽ����ģ�建����

	//�ͷ�DSB
	DSBuffer.Reset();

	//�������ģ�建������Դ
	D3D12_RESOURCE_DESC DSDesc;

	DSDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;                   //��Դ����Ϊ2D����
	DSDesc.Alignment = 0;                                                    //���뷽ʽ��0��
	DSDesc.Width = ClientWidth;                                              //��Դ���
	DSDesc.Height = ClientHeight;                                            //��Դ�߶�
	DSDesc.DepthOrArraySize = 1;                                             //��Դ���
	DSDesc.MipLevels = 1;                                                    //mipmap�㼶
	DSDesc.Format = DSFormat;                                                //��Դ��ʽ
	DSDesc.SampleDesc.Count = 1;
	DSDesc.SampleDesc.Quality = 0;
	DSDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;                            //������ѡ�����δ֪������������������
	DSDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;                  //����Ϊ��Դ�������ģ����ͼ

	//������Դ���ֵ
	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DSFormat;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;

	//������Ҫ�����Ķ����ԣ�Ĭ�϶�
	CD3DX12_HEAP_PROPERTIES DSProperties(D3D12_HEAP_TYPE_DEFAULT);

	//������
	ThrowIfFailed(Device->CreateCommittedResource(&DSProperties, D3D12_HEAP_FLAG_NONE, &DSDesc, D3D12_RESOURCE_STATE_COMMON, &ClearValue, IID_PPV_ARGS(DSBuffer.GetAddressOf())));

	//����������
	Device->CreateDepthStencilView(DSBuffer.Get(),nullptr,DSVHeap->GetCPUDescriptorHandleForHeapStart());

//�ı�DS��Դ����
	//��������
	CD3DX12_RESOURCE_BARRIER DSBufferBarrier =
		CD3DX12_RESOURCE_BARRIER::Transition(DSBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	//�ı�����
	ComPtr<ID3D12CommandQueue> CmdQueue = CreateCmdQueue();
	ComPtr<ID3D12CommandAllocator> CmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(CmdAllocator);

	CmdList->ResourceBarrier(1, &DSBufferBarrier);

	ExeCmd(CmdList, CmdQueue);

	FluCmdQueue(CmdQueue);
}

void D3DBASE::FluCmdQueue(ComPtr<ID3D12CommandQueue> CmdQueue, HANDLE Event)
{
	FenceNum++;

	ThrowIfFailed(CmdQueue->Signal(Fence.Get(), FenceNum));

	if (!Event)
	{
		Event = CreateEvent(NULL, FALSE, FALSE, NULL);
		ThrowIfFailed(Fence->SetEventOnCompletion(FenceNum, Event));
		WaitForSingleObject(Event, INFINITE);
	}
	else
	{
		ThrowIfFailed(Fence->SetEventOnCompletion(FenceNum, Event));
	}
}

D3D12_SHADER_BYTECODE D3DBASE::CompileShader(ComPtr<ID3DBlob>& ShaderByteCode, wchar_t* FileName, char* FunctionName, LPCSTR ShaderVersion, D3D_SHADER_MACRO* ShaderMacro, ID3DInclude* HandleIncludeMacro, UINT Flag1) 
{
	ComPtr<ID3DBlob> Error;

	HRESULT hresult = S_OK;

	hresult = D3DCompileFromFile(                                              //������ɫ��
		FileName,                                                              //��ɫ���ļ���
		ShaderMacro,                                                           //�궨�����飬��������ΪNULL
		HandleIncludeMacro,                                                    //ָ��������ļ���ID3DInclude�ӿڵ�ָ�룬��������ΪNULL
		FunctionName,                                                          //��ɫ��������
		ShaderVersion,                                                         //��ɫ�����Ͱ汾
		Flag1,                                                                 //����ѡ�����
		0,
		&ShaderByteCode,                                                       //�洢����õ���ɫ���ֽ���
		&Error);                                                              //�洢������Ϣ

	if (Error.Get())
	{
		cout << "��ɫ�����棺";
		wcout << FileName << "  " << endl;
		cout << (char*)Error->GetBufferPointer() << endl;
	}

	ThrowIfFailed(hresult);

	D3D12_SHADER_BYTECODE ShaderByteCodeDesc;
	ShaderByteCodeDesc.BytecodeLength = ShaderByteCode->GetBufferSize();
	ShaderByteCodeDesc.pShaderBytecode = ShaderByteCode->GetBufferPointer();

	return ShaderByteCodeDesc;
}

ComPtr<ID3D12Resource> D3DBASE::CreateUploadBuffer(UINT64 resourceSize)
{
	ComPtr<ID3D12Resource> UploadBuffer;

	CD3DX12_RESOURCE_DESC ResourceDesc =                                       //������Դ�����ṹ��
		CD3DX12_RESOURCE_DESC::Buffer(resourceSize);

	CD3DX12_HEAP_PROPERTIES CBHeapProperties(D3D12_HEAP_TYPE_UPLOAD);          //�ϴ�������

	ThrowIfFailed(Device->CreateCommittedResource(&CBHeapProperties,           //���������ϴ��ѣ�������)
		D3D12_HEAP_FLAG_NONE, &ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(UploadBuffer.GetAddressOf())));

	return UploadBuffer;
}

D3D12_CONSTANT_BUFFER_VIEW_DESC D3DBASE::CreateConstantBuffer(ComPtr<ID3D12Resource>& ConstantUploadBuffer, UINT DataSize, D3D12_HEAP_TYPE heapType)
{
	UINT CBSize = DataSize + 255 & ~255;                         //����Ԫ�ش�С����Ϊ256��������

	CD3DX12_RESOURCE_DESC ResourceDesc =                                       //������Դ�����ṹ��
		CD3DX12_RESOURCE_DESC::Buffer(CBSize);

	CD3DX12_HEAP_PROPERTIES CBHeapProperties(heapType);                        //�����ԣ�Ĭ���ϴ���

	ThrowIfFailed(Device->CreateCommittedResource(&CBHeapProperties,           //��������������
		D3D12_HEAP_FLAG_NONE, &ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(ConstantUploadBuffer.GetAddressOf())));

	//����������������
	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = ConstantUploadBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = CBSize;

	return CBVDesc;
}

ComPtr<ID3D12DescriptorHeap> D3DBASE::CreateCSUHeap(vector<D3D12_CONSTANT_BUFFER_VIEW_DESC>& CBVDescList, vector<D3D12_SHADER_RESOURCE_VIEW_DESC>& SRVDescList,
	vector< D3D12_UNORDERED_ACCESS_VIEW_DESC>& UAVDescList, vector<ID3D12Resource*>& ShaderResource,vector<ID3D12Resource*>& UAResource1, vector<ID3D12Resource*>& UAResource2)
{
	UINT CBVNum = CBVDescList.size();
	UINT SRVNum = SRVDescList.size();
	UINT UAVNum = UAVDescList.size();

	//�����������������������գ���
	ComPtr<ID3D12DescriptorHeap> DescHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, CBVNum + SRVNum + UAVNum);

	//����������
	CD3DX12_CPU_DESCRIPTOR_HANDLE DescHeapHandle(DescHeap->GetCPUDescriptorHandleForHeapStart());

	//����CBV������
	for (int i = 0; i < CBVNum; i++)
	{
		Device->CreateConstantBufferView(&CBVDescList[i], DescHeapHandle);
		DescHeapHandle.Offset(1, CSUSize);
	}
	//����SRV������
	for (int i = 0; i < SRVNum; i++)
	{
		Device->CreateShaderResourceView(ShaderResource[i], &SRVDescList[i], DescHeapHandle);
		DescHeapHandle.Offset(1, CSUSize);
	}
	//����UAV������
	for (int i = 0; i < UAVNum; i++)
	{
		Device->CreateUnorderedAccessView(UAResource1[i], UAResource2[i], &UAVDescList[i], DescHeapHandle);
		DescHeapHandle.Offset(1, CSUSize);
	}

	return DescHeap;
}

//void D3DBASE::UpdateUB(void* DataSrc, UINT CBSize, ComPtr<ID3D12Resource>& ConstantBuffer, UINT Subresource, D3D12_RANGE* ReadRange)
//{
//	//��ø���Ŀ������ָ��
//	void* DataDst;
//	ThrowIfFailed(ConstantBuffer->Map(Subresource, ReadRange, &DataDst));
//
//	//����
//	memcpy(DataDst, DataSrc, CBSize);
//
//	//�ͷ�ӳ��
//	ConstantBuffer->Unmap(Subresource, ReadRange);
//}

ComPtr<ID3D12Resource> D3DBASE::UploadTexture(wstring File)
{
	ComPtr<ID3D12Resource> Texture;

	auto CmdQueue = CreateCmdQueue();

	ResourceUploadBatch upload(Device.Get());

	upload.Begin();
	ThrowIfFailed(CreateDDSTextureFromFile(Device.Get(), upload, File.data(), Texture.GetAddressOf()));
	upload.End(CmdQueue.Get());

	FluCmdQueue(CmdQueue);

	Texture->SetName(File.data());

	return Texture;
}

ComPtr<ID3D12Resource> D3DBASE::CreateDefaultBuffer(UINT64 ResourceSize, const void* ResourceData, LPCWSTR name)
{
//����������Դ
	CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(ResourceSize);

//�����н��ϴ���
	ComPtr<ID3D12Resource> UploadBuffer;

	//�����ϴ�������
	CD3DX12_HEAP_PROPERTIES UploadBufferProperties(D3D12_HEAP_TYPE_UPLOAD);                //�����ϴ�������

	//������
	ThrowIfFailed(Device->CreateCommittedResource(&UploadBufferProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(UploadBuffer.GetAddressOf())));

//����Ŀ��Ĭ�϶�
	ComPtr<ID3D12Resource> DefaultBuffer;

	CD3DX12_HEAP_PROPERTIES DefaultBufferProperties(D3D12_HEAP_TYPE_DEFAULT);             //����Ĭ�϶�����

	//������
	ThrowIfFailed(Device->CreateCommittedResource(&DefaultBufferProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(DefaultBuffer.GetAddressOf())));

//����Ĭ�϶�
	//������ʱ�����
	ComPtr<ID3D12CommandAllocator> CmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(CmdAllocator);
	ComPtr<ID3D12CommandQueue> CmdQueue = CreateCmdQueue();

	//����Ҫ���Ƶ�Ĭ�϶ѵ�����
	D3D12_SUBRESOURCE_DATA SubResourceDesc = {};
	SubResourceDesc.pData = ResourceData;                                    //ָ����Դ��ָ��
	SubResourceDesc.RowPitch = ResourceSize;                                 //��Դ�ֽ���
	SubResourceDesc.SlicePitch = SubResourceDesc.RowPitch;                   //��Դ�ֽ������Ⱦ���ô�㣩

	//��Ĭ�϶ѵ���Դ״̬����Ĭ��״̬תΪ���Ʋ�����Ŀ��״̬
	CD3DX12_RESOURCE_BARRIER DefaultBufferBarrier =                          //��������Դ���ϵ�����
		CD3DX12_RESOURCE_BARRIER::Transition(                                //ת����Դ��״̬
			DefaultBuffer.Get(),                                             //��Դ��ַ
			D3D12_RESOURCE_STATE_COMMON,                                     //��Դת��ǰ״̬
			D3D12_RESOURCE_STATE_COPY_DEST);                                 //��Դת����״̬

	CmdList->ResourceBarrier(1,                                              //�ύ����������������
		&DefaultBufferBarrier);                                              //ָ���������������ָ��

	//��������Դ
	UpdateSubresources<1>                                                    //�������Դ����Ϊ1
		(CmdList.Get(), DefaultBuffer.Get(), UploadBuffer.Get(),                   //�����б�����Ŀ����Դ������Դ��Դ
			0, 0, 1,                                                         //���м���Դƫ��������һ������Դ��������Դ������Դ����
			&SubResourceDesc);                                               //����������Դ�����ṹ�ĵ�ַ������
	
	//��Ĭ�϶ѵ���Դ״̬���Ӹ��Ʋ�����Ŀ��״̬תΪͨ�ö�ȡ״̬
	CD3DX12_RESOURCE_BARRIER DefaultBufferBarrier2 =                         //��������Դ���ϵ�����
		CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	CmdList->ResourceBarrier(1,     //�ύ����������������
		&DefaultBufferBarrier2);    //ָ���������������ָ��

	ExeCmd(CmdList, CmdQueue);

	//�ȴ�GPU�˴�����Դ
	FluCmdQueue(CmdQueue);

	if (name != nullptr)
	{
		DefaultBuffer->SetName(name);
	}

	return DefaultBuffer;
}

void D3DBASE::UpdateDB(ComPtr<ID3D12Resource> defaultBuffer, void* sourceData, UINT sourceSize, D3D12_RESOURCE_STATES beforeBarrier)
{
//�����н��ϴ���
	CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sourceSize);
	ComPtr<ID3D12Resource> UploadBuffer;
	CD3DX12_HEAP_PROPERTIES UploadBufferProperties(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(Device->CreateCommittedResource(&UploadBufferProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(UploadBuffer.GetAddressOf())));

//����Ĭ�϶�
	ComPtr<ID3D12CommandAllocator> CmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(CmdAllocator, L"UpdateDefaultBuffer");
	ComPtr<ID3D12CommandQueue> CmdQueue = CreateCmdQueue();

	//����Ҫ���Ƶ�Ĭ�϶ѵ�����
	D3D12_SUBRESOURCE_DATA SubResourceDesc = {};
	SubResourceDesc.pData = sourceData;
	SubResourceDesc.RowPitch = sourceSize;
	SubResourceDesc.SlicePitch = SubResourceDesc.RowPitch;

	//��Դ״̬תΪ����Ŀ��̬
	CD3DX12_RESOURCE_BARRIER DefaultBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), beforeBarrier, D3D12_RESOURCE_STATE_COPY_DEST);
	CmdList->ResourceBarrier(1, &DefaultBufferBarrier);

	//������Դ
	UpdateSubresources<1>(CmdList.Get(), defaultBuffer.Get(), UploadBuffer.Get(), 0, 0, 1, &SubResourceDesc);

	//��Դ״̬תΪGENERIC_READ
	CD3DX12_RESOURCE_BARRIER DefaultBufferBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	CmdList->ResourceBarrier(1, &DefaultBufferBarrier2);

	ExeCmd(CmdList, CmdQueue);

	//�ȴ�GPU�˴�����Դ
	FluCmdQueue(CmdQueue);
}