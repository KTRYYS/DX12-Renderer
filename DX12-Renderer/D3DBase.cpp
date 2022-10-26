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
	//开启调试层
	D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController));
	DebugController->EnableDebugLayer();
	//DebugController->QueryInterface(IID_PPV_ARGS(&DebugController));

	//开启控制台
	AllocConsole();
	FILE* stream1;
	freopen_s(&stream1, "CONOUT$", "w", stdout);
}

void D3DBASE::LogAdapter()
{
	//显示当前计算机的显示适配器（显卡）
	UINT AdapterIndex = 0;
	IDXGIAdapter* Adapter = nullptr;
	cout << "系统显示适配器列表：" << endl;
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
	//创建工厂
	ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&Factory)));

	//选择适配器
	Factory->EnumAdapters(GPUIndex, Adapter.GetAddressOf());

	//创建设备
	ThrowIfFailed(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device)));

	//获得GPU掩码
	GPUNodeMask = Device->GetNodeCount();

#ifndef NDEBUG
	//输出所选适配器和GPU掩码
	DXGI_ADAPTER_DESC AdapterDesc;
	ThrowIfFailed(Adapter->GetDesc(&AdapterDesc));
	wcout << AdapterDesc.Description << endl;
	cout << "GPU掩码：" << GPUNodeMask << endl;
#endif // ! NDEBUG
}

void D3DBASE::InitialFence()
{
	//创建围栏
	ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));

	//初始化围栏值
	FenceNum = 0;
}

void D3DBASE::GetDescriptorSize()
{
	//获得渲染目标缓冲区描述符大小
	RTVSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//获得深度模板缓冲区描述符大小
	DSVSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//获得CBV、SRV、UAV大小
	CSUSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

ComPtr<ID3D12CommandQueue> D3DBASE::CreateCmdQueue(D3D12_COMMAND_LIST_TYPE cmdType, LPCWSTR name)
{
	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Type = cmdType;                                              //命令列表类型：直接命令列表，指定GPU可执行的命令列表缓冲区，不继承任何GPU状态
	QueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;              //命令队列优先级：正常优先级
	QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;                       //标志：默认命令队列
	QueueDesc.NodeMask = GPUNodeMask;

	ComPtr<ID3D12CommandQueue> CmdQueue;
	ThrowIfFailed(Device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CmdQueue)));       //创建命令队列

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
//	//创建命令分配器
//	ThrowIfFailed(Device->CreateCommandAllocator(cmdType,                  //命令列表类型：直接命令列表
//		IID_PPV_ARGS(&cmdAllocator)));
//
//	//创建命令列表
//	ThrowIfFailed(Device->CreateCommandList(GPUNodeMask,
//		cmdType,                                                           //命令列表类型:直接命令列表
//		cmdAllocator.Get(),                                                //与命令列表关联的命令分配器
//		nullptr,                                                           //命令列表渲染流水线初始状态，nullptr为纯初始状态
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

	//创建命令分配器
	ThrowIfFailed(Device->CreateCommandAllocator(cmdType,                  //命令列表类型：直接命令列表
		IID_PPV_ARGS(&cmdAllocator)));

	//创建命令列表
	ThrowIfFailed(Device->CreateCommandList(GPUNodeMask,
		cmdType,                                                           //命令列表类型:直接命令列表
		cmdAllocator.Get(),                                                //与命令列表关联的命令分配器
		nullptr,                                                           //命令列表渲染流水线初始状态，nullptr为纯初始状态
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
//交换链部分
	//创建并填写交换链描述结构体
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};

	SwapChainDesc.BufferDesc.Width = ClientWidth;                            //分辨率宽度
	SwapChainDesc.BufferDesc.Height = ClientHeight;                          //分辨率高度
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 0;                    //刷新率分母
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 0;                      //刷新率分子
	SwapChainDesc.BufferDesc.Format = RTFormat;                              //缓冲区显示格式
	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;       //未指定扫描线顺序
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;        //未指定缩放
	SwapChainDesc.SampleDesc.Count = 1;                                      //采样像素个数
	SwapChainDesc.SampleDesc.Quality = 0;                                    //采样质量级别
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = SwapChainBufferNumber;                        //交换链数量
	SwapChainDesc.OutputWindow = hwnd;                                       //将深度缓冲区输出的内容绑定到窗口上
	SwapChainDesc.Windowed = true;                                           //窗口模式显示
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;            //可切换窗口全屏模式

	//创建交换链
	ThrowIfFailed(Factory->CreateSwapChain(MainCmdQueue.Get(), &SwapChainDesc, SwapChain.GetAddressOf()));

//渲染目标缓冲区部分
	//创建描述符堆
	RTVHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, SwapChainBufferNumber);

	//创建两个RT缓冲区和对应的RTV
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(                             //获得RTV堆首句柄
		RTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < SwapChainBufferNumber; i++)                         //依次创建两个RT缓冲区，并把对应RTV填入描述符堆中
	{
		SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffer[i]));          //建立RTB

		Device->CreateRenderTargetView(                                      //为RT缓冲区创建描述符
			SwapChainBuffer[i].Get(),                                        //RT资源
			nullptr,                                                         //资源中元素数据类型，已指定格式，设为nullptr
			RTVHeapHandle);                                                  //要创建的RTV所在内存区域的CPU描述符句柄

		RTVHeapHandle.Offset(                                                //偏移句柄到堆中下一个描述符内存区域的开始
			1,                                                               //偏移数量 
			RTVSize);                                                        //偏移量大小
	}
}

void D3DBASE::CreateDS()
{
	//创建和填写DS缓冲区资源描述结构体
	D3D12_RESOURCE_DESC DSDesc;
	DSDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;                   //资源类型:2D纹理
	DSDesc.Alignment = 0;                                                    //对齐方式
	DSDesc.Width = ClientWidth;                                              //资源宽度
	DSDesc.Height = ClientHeight;                                            //资源高度
	DSDesc.DepthOrArraySize = 1;                                             //资源深度
	DSDesc.MipLevels = 1;                                                    //mipmap层级
	DSDesc.Format = DSFormat;                                                //资源格式
	DSDesc.SampleDesc.Count = 1;
	DSDesc.SampleDesc.Quality = 0;
	DSDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;                            //纹理布局选项：布局未知，可能依赖于适配器
	DSDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;                  //允许为资源创建深度模板视图

	//创建DS缓冲区
	D3D12_CLEAR_VALUE ClearValue;                                            //用于清除资源的优化值
	ClearValue.Format = DSFormat;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES DSProperties(D3D12_HEAP_TYPE_DEFAULT);           //描述将要创建的堆属性：默认堆

	ThrowIfFailed(Device->CreateCommittedResource(                           //创建DS缓冲区和一个默认堆，并把前者映射到后者上
		&DSProperties,                                                       //堆属性：默认堆
		D3D12_HEAP_FLAG_NONE,                                                //标志：默认值
		&DSDesc,                                                             //放入堆中的资源（DS缓冲区）描述符
		D3D12_RESOURCE_STATE_COMMON,                                         //资源初始状态
		&ClearValue,                                                         //清除资源区域的值
		IID_PPV_ARGS(DSBuffer.GetAddressOf())));

	//创建DSV堆
	DSVHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	//创建DSV
	Device->CreateDepthStencilView(DSBuffer.Get(),                           //DS资源
		nullptr,                                                             //描述资源格式，已描述则为空指针
		DSVHeap->GetCPUDescriptorHandleForHeapStart());                      //DSV堆首句柄

	//更改DS缓冲区状态
	CD3DX12_RESOURCE_BARRIER DSBufferBarrier =                               //描述DS资源屏障
		CD3DX12_RESOURCE_BARRIER::Transition(
			DSBuffer.Get(),                                                  //DS资源
			D3D12_RESOURCE_STATE_COMMON,                                     //资源转换前状态
			D3D12_RESOURCE_STATE_DEPTH_WRITE);                               //资源转换后状态

	ComPtr<ID3D12CommandAllocator> CmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(CmdAllocator);

	CmdList->ResourceBarrier(                                                //通知驱动同步资源访问
		1,                                                                   //屏障数量
		&DSBufferBarrier);                                                   //屏障描述结构体数组

	ExeCmd(CmdList, MainCmdQueue);

	FluCmdQueue(MainCmdQueue);

	CmdList.~ComPtr();
	CmdAllocator.~ComPtr();
}

void D3DBASE::SetViewPortAndSciRect(ComPtr<ID3D12GraphicsCommandList>CmdList)
{
	//设置视口
	ViewPort.TopLeftX = 0;                                                   //视口左上顶点在后台缓冲区的x坐标
	ViewPort.TopLeftY = 0;                                                   //类上，y坐标
	ViewPort.Width = static_cast<float>(ClientWidth);                        //视口宽度
	ViewPort.Height = static_cast<float>(ClientHeight);                      //视口高度
	ViewPort.MinDepth = 0.0f;                                                //深度值映射区间左端点值
	ViewPort.MaxDepth = 1.0f;                                                //深度值映射区间的右端点值

	CmdList->RSSetViewports(                                                 //视口数量
		1, &ViewPort);                                                       //视口描述结构体数组

	//设置裁剪矩形
	SciRect.left = 0;                                                        //裁剪矩形区左上角x坐标
	SciRect.top = 0;                                                         //左下角y坐标
	SciRect.right = ClientWidth;                                             //右下角x坐标
	SciRect.bottom = ClientHeight;                                           //右下角y坐标

	CmdList->RSSetScissorRects(                                              //设置裁剪矩形
		1, &SciRect);                                                        //裁剪矩形数量；裁剪矩形描述结构体数组
}

ComPtr<ID3D12RootSignature> D3DBASE::CreateRootSig(D3D12_ROOT_SIGNATURE_DESC RootSigDesc)
{
	//序列化根签名
	ComPtr<ID3DBlob> Error = nullptr;                                      //存放根签名序列化失败的信息
	ComPtr<ID3DBlob> SerializedRootSig = nullptr;                              //存放序列化后的根签名数据

	ThrowIfFailed(D3D12SerializeRootSignature(                                               //序列化根签名
		&RootSigDesc,                                                          //根签名描述结构体地址
		D3D_ROOT_SIGNATURE_VERSION_1,                                          //根签名版本
		SerializedRootSig.GetAddressOf(), Error.GetAddressOf()));           //返回序列化后的根签名源数据，和错误信息

	if (Error.Get())
	{
		cout << "根签名序列化警告：" << (char*)Error->GetBufferPointer() << endl;
	}

	//创建根签名
	ComPtr<ID3D12RootSignature> RootSignature;

	ThrowIfFailed(Device->CreateRootSignature(GPUNodeMask,
		SerializedRootSig->GetBufferPointer(),                                 //序列化后根签名数据地址
		SerializedRootSig->GetBufferSize(),                                    //根签名源数据大小
		IID_PPV_ARGS(&RootSignature)));

	return RootSignature;
}

ComPtr<ID3D12PipelineState> D3DBASE::CreateBasicPSO(ComPtr<ID3D12RootSignature> RootSignature, D3D12_SHADER_BYTECODE VSByteCodeDesc, D3D12_SHADER_BYTECODE PSByteCodeDesc, D3D12_INPUT_LAYOUT_DESC IAInputDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
	ComPtr<ID3D12PipelineState> PSO;

	//描述PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

	PSODesc.pRootSignature = RootSignature.Get();                              //绑定根签名
	PSODesc.VS = VSByteCodeDesc;                                               //绑定顶点着色器
	PSODesc.PS = PSByteCodeDesc;                                               //绑定像素着色器
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);          //使用默认光栅化
	//PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //线框模式
	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //绘制全部三角形
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                    //混合状态：默认
	PSODesc.SampleMask = UINT_MAX;                                             //多重采样相关，对所有采样点都进行采样
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);     //深度、模板状态：默认
	PSODesc.PrimitiveTopologyType = TopologyType;                              //图元拓扑类型：三角形
	PSODesc.RTVFormats[0] = RTFormat;                                          //渲染目标格式的类型值数组
	PSODesc.NumRenderTargets = 1;                                              //渲染目标格式数组元素数量
	PSODesc.SampleDesc.Count = 1;                                              //每个像素多重采样数
	PSODesc.SampleDesc.Quality = 0;                                            //多重采样质量
	PSODesc.DSVFormat = DSFormat;                                              //深度模板缓冲区格式
	PSODesc.InputLayout = IAInputDesc;                                         //输入布局描述

	//创建PSO
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
//	//描述PSO
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
//
//	PSODesc.pRootSignature = RootSignature.Get();
//	PSODesc.VS = VSByteCodeDesc;
//	PSODesc.PS = PSByteCodeDesc;
//	PSODesc.HS = HSByteCodeDesc;
//	PSODesc.DS = DSByteCodeDesc;
//	PSODesc.GS = GSByteCodeDesc;
//	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	//PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //线框模式
//	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //绘制全部三角形
//	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//	PSODesc.SampleMask = UINT_MAX;
//	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;       //图元拓扑类型：片元
//	PSODesc.RTVFormats[0] = RTFormat;
//	PSODesc.NumRenderTargets = 1;
//	PSODesc.SampleDesc.Count = 1;
//	PSODesc.SampleDesc.Quality = 0;
//	PSODesc.DSVFormat = DSFormat;
//	PSODesc.InputLayout = IAInputDesc;
//
//	//创建PSO
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
//	BlendTargetDesc.BlendEnable = true;                                      //常规混合功能：启用，和常逻辑合功能不能同时ture或false
//	BlendTargetDesc.LogicOpEnable = false;                                   //逻辑混合运算功能：禁用，和常规混合功能不能同时ture或false
//	BlendTargetDesc.SrcBlend = D3D12_BLEND_BLEND_FACTOR;                     //指定RGB源的混合因子
//	BlendTargetDesc.DestBlend = D3D12_BLEND_INV_BLEND_FACTOR;                //指定RGB目标混合因子
//	BlendTargetDesc.BlendOp = D3D12_BLEND_OP_ADD;                            //指定混合操作
//	BlendTargetDesc.SrcBlendAlpha = D3D12_BLEND_BLEND_FACTOR;                //指定alpha源混合因子
//	BlendTargetDesc.DestBlendAlpha = D3D12_BLEND_INV_BLEND_FACTOR;           //指定alpha目标混合因子
//	BlendTargetDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
//	BlendTargetDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
//	BlendTargetDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;          //控制混合后数据可被写入后台缓冲区哪些通道
//
//	D3D12_BLEND_DESC BlendDesc;
//	BlendDesc.AlphaToCoverageEnable = false;       //alpha-to-coverage功能（渲染叶片或门等纹理所用的多重采样技术）：关闭
//	BlendDesc.IndependentBlendEnable = false;      //false：所有渲染目标使用RenderTarget第一个元素进行混合，true：每个渲染目标使用不同混合（最多支持八个）
//	BlendDesc.RenderTarget[0] = BlendTargetDesc;   //混合方式，最多支持八个，如果IndependentBlendEnable为false，则只用第一个
//
//	//描述PSO
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
//
//	PSODesc.pRootSignature = RootSignature.Get();
//	PSODesc.VS = VSByteCodeDesc;
//	PSODesc.PS = PSByteCodeDesc;
//	PSODesc.HS = HSByteCodeDesc;
//	PSODesc.DS = DSByteCodeDesc;
//	PSODesc.GS = GSByteCodeDesc;
//	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;            //线框模式
//	//PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                 //绘制全部三角形
//	PSODesc.BlendState = BlendDesc;
//	PSODesc.SampleMask = UINT_MAX;
//	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;       //图元拓扑类型：片元
//	PSODesc.RTVFormats[0] = RTFormat;
//	PSODesc.NumRenderTargets = 1;
//	PSODesc.SampleDesc.Count = 1;
//	PSODesc.SampleDesc.Quality = 0;
//	PSODesc.DSVFormat = DSFormat;
//	PSODesc.InputLayout = IAInputDesc;
//
//	//创建PSO
//	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));
//
//	return PSO;
//}

void D3DBASE::ResizeRTandDS()
{

//重建渲染目标缓冲区
	//释放RTB
	for (int i = 0; i < SwapChainBufferNumber; i++)
	{
		SwapChainBuffer[i].Reset();
	}

	//重置交换链
	ThrowIfFailed(SwapChain->ResizeBuffers(SwapChainBufferNumber, ClientWidth, ClientHeight, RTFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	BackBufferIndex = 0;

	//获得描述符堆句柄
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());

	//创建缓冲区和描述符
	for (UINT i = 0; i < SwapChainBufferNumber; i++)
	{
		ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffer[i])));

		Device->CreateRenderTargetView(SwapChainBuffer[i].Get(), nullptr, RTVHeapHandle);

		RTVHeapHandle.Offset(1, RTVSize);
	}

//重建深度模板缓冲区

	//释放DSB
	DSBuffer.Reset();

	//描述深度模板缓冲区资源
	D3D12_RESOURCE_DESC DSDesc;

	DSDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;                   //资源类型为2D纹理
	DSDesc.Alignment = 0;                                                    //对齐方式：0？
	DSDesc.Width = ClientWidth;                                              //资源宽度
	DSDesc.Height = ClientHeight;                                            //资源高度
	DSDesc.DepthOrArraySize = 1;                                             //资源深度
	DSDesc.MipLevels = 1;                                                    //mipmap层级
	DSDesc.Format = DSFormat;                                                //资源格式
	DSDesc.SampleDesc.Count = 1;
	DSDesc.SampleDesc.Quality = 0;
	DSDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;                            //纹理布局选项：布局未知，可能依赖于适配器
	DSDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;                  //允许为资源创建深度模板视图

	//描述资源清除值
	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DSFormat;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;

	//描述将要创建的堆属性：默认堆
	CD3DX12_HEAP_PROPERTIES DSProperties(D3D12_HEAP_TYPE_DEFAULT);

	//创建堆
	ThrowIfFailed(Device->CreateCommittedResource(&DSProperties, D3D12_HEAP_FLAG_NONE, &DSDesc, D3D12_RESOURCE_STATE_COMMON, &ClearValue, IID_PPV_ARGS(DSBuffer.GetAddressOf())));

	//创建描述符
	Device->CreateDepthStencilView(DSBuffer.Get(),nullptr,DSVHeap->GetCPUDescriptorHandleForHeapStart());

//改变DS资源屏障
	//描述屏障
	CD3DX12_RESOURCE_BARRIER DSBufferBarrier =
		CD3DX12_RESOURCE_BARRIER::Transition(DSBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	//改变屏障
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

	hresult = D3DCompileFromFile(                                              //编译着色器
		FileName,                                                              //着色器文件名
		ShaderMacro,                                                           //宏定义数组，不用设置为NULL
		HandleIncludeMacro,                                                    //指向处理包含文件的ID3DInclude接口的指针，不用设置为NULL
		FunctionName,                                                          //着色器函数名
		ShaderVersion,                                                         //着色器类型版本
		Flag1,                                                                 //编译选项组合
		0,
		&ShaderByteCode,                                                       //存储编译好的着色器字节码
		&Error);                                                              //存储错误信息

	if (Error.Get())
	{
		cout << "着色器警告：";
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

	CD3DX12_RESOURCE_DESC ResourceDesc =                                       //常量资源描述结构体
		CD3DX12_RESOURCE_DESC::Buffer(resourceSize);

	CD3DX12_HEAP_PROPERTIES CBHeapProperties(D3D12_HEAP_TYPE_UPLOAD);          //上传堆属性

	ThrowIfFailed(Device->CreateCommittedResource(&CBHeapProperties,           //创建常量上传堆（缓冲区)
		D3D12_HEAP_FLAG_NONE, &ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(UploadBuffer.GetAddressOf())));

	return UploadBuffer;
}

D3D12_CONSTANT_BUFFER_VIEW_DESC D3DBASE::CreateConstantBuffer(ComPtr<ID3D12Resource>& ConstantUploadBuffer, UINT DataSize, D3D12_HEAP_TYPE heapType)
{
	UINT CBSize = DataSize + 255 & ~255;                         //常量元素大小必须为256的整数倍

	CD3DX12_RESOURCE_DESC ResourceDesc =                                       //常量资源描述结构体
		CD3DX12_RESOURCE_DESC::Buffer(CBSize);

	CD3DX12_HEAP_PROPERTIES CBHeapProperties(heapType);                        //堆属性，默认上传堆

	ThrowIfFailed(Device->CreateCommittedResource(&CBHeapProperties,           //创建常量缓冲区
		D3D12_HEAP_FLAG_NONE, &ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(ConstantUploadBuffer.GetAddressOf())));

	//返回描述符的描述
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

	//创建常量缓冲区描述符（空）堆
	ComPtr<ID3D12DescriptorHeap> DescHeap = CreateDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, CBVNum + SRVNum + UAVNum);

	//创建描述符
	CD3DX12_CPU_DESCRIPTOR_HANDLE DescHeapHandle(DescHeap->GetCPUDescriptorHandleForHeapStart());

	//创建CBV描述符
	for (int i = 0; i < CBVNum; i++)
	{
		Device->CreateConstantBufferView(&CBVDescList[i], DescHeapHandle);
		DescHeapHandle.Offset(1, CSUSize);
	}
	//创建SRV描述符
	for (int i = 0; i < SRVNum; i++)
	{
		Device->CreateShaderResourceView(ShaderResource[i], &SRVDescList[i], DescHeapHandle);
		DescHeapHandle.Offset(1, CSUSize);
	}
	//创建UAV描述符
	for (int i = 0; i < UAVNum; i++)
	{
		Device->CreateUnorderedAccessView(UAResource1[i], UAResource2[i], &UAVDescList[i], DescHeapHandle);
		DescHeapHandle.Offset(1, CSUSize);
	}

	return DescHeap;
}

//void D3DBASE::UpdateUB(void* DataSrc, UINT CBSize, ComPtr<ID3D12Resource>& ConstantBuffer, UINT Subresource, D3D12_RANGE* ReadRange)
//{
//	//获得复制目标区域指针
//	void* DataDst;
//	ThrowIfFailed(ConstantBuffer->Map(Subresource, ReadRange, &DataDst));
//
//	//复制
//	memcpy(DataDst, DataSrc, CBSize);
//
//	//释放映射
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
//描述堆中资源
	CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(ResourceSize);

//创建中介上传堆
	ComPtr<ID3D12Resource> UploadBuffer;

	//描述上传堆属性
	CD3DX12_HEAP_PROPERTIES UploadBufferProperties(D3D12_HEAP_TYPE_UPLOAD);                //描述上传堆属性

	//创建堆
	ThrowIfFailed(Device->CreateCommittedResource(&UploadBufferProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(UploadBuffer.GetAddressOf())));

//创建目标默认堆
	ComPtr<ID3D12Resource> DefaultBuffer;

	CD3DX12_HEAP_PROPERTIES DefaultBufferProperties(D3D12_HEAP_TYPE_DEFAULT);             //描述默认堆属性

	//创建堆
	ThrowIfFailed(Device->CreateCommittedResource(&DefaultBufferProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(DefaultBuffer.GetAddressOf())));

//更新默认堆
	//创建临时命令部分
	ComPtr<ID3D12CommandAllocator> CmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(CmdAllocator);
	ComPtr<ID3D12CommandQueue> CmdQueue = CreateCmdQueue();

	//描述要复制到默认堆的数据
	D3D12_SUBRESOURCE_DATA SubResourceDesc = {};
	SubResourceDesc.pData = ResourceData;                                    //指向资源的指针
	SubResourceDesc.RowPitch = ResourceSize;                                 //资源字节数
	SubResourceDesc.SlicePitch = SubResourceDesc.RowPitch;                   //资源字节数（先就这么算）

	//将默认堆的资源状态，从默认状态转为复制操作的目标状态
	CD3DX12_RESOURCE_BARRIER DefaultBufferBarrier =                          //返回屏资源屏障的描述
		CD3DX12_RESOURCE_BARRIER::Transition(                                //转换资源的状态
			DefaultBuffer.Get(),                                             //资源地址
			D3D12_RESOURCE_STATE_COMMON,                                     //资源转换前状态
			D3D12_RESOURCE_STATE_COPY_DEST);                                 //资源转换后状态

	CmdList->ResourceBarrier(1,                                              //提交的屏障描述的数量
		&DefaultBufferBarrier);                                              //指向屏障描述数组的指针

	//更新子资源
	UpdateSubresources<1>                                                    //最大子资源数量为1
		(CmdList.Get(), DefaultBuffer.Get(), UploadBuffer.Get(),                   //命令列表；复制目标资源；复制源资源
			0, 0, 1,                                                         //到中间资源偏移量；第一个子资源索引，资源中子资源数量
			&SubResourceDesc);                                               //待更新子资源描述结构的地址的数组
	
	//将默认堆的资源状态，从复制操作的目标状态转为通用读取状态
	CD3DX12_RESOURCE_BARRIER DefaultBufferBarrier2 =                         //返回屏资源屏障的描述
		CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	CmdList->ResourceBarrier(1,     //提交的屏障描述的数量
		&DefaultBufferBarrier2);    //指向屏障描述数组的指针

	ExeCmd(CmdList, CmdQueue);

	//等待GPU端创建资源
	FluCmdQueue(CmdQueue);

	if (name != nullptr)
	{
		DefaultBuffer->SetName(name);
	}

	return DefaultBuffer;
}

void D3DBASE::UpdateDB(ComPtr<ID3D12Resource> defaultBuffer, void* sourceData, UINT sourceSize, D3D12_RESOURCE_STATES beforeBarrier)
{
//创建中介上传堆
	CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sourceSize);
	ComPtr<ID3D12Resource> UploadBuffer;
	CD3DX12_HEAP_PROPERTIES UploadBufferProperties(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(Device->CreateCommittedResource(&UploadBufferProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(UploadBuffer.GetAddressOf())));

//更新默认堆
	ComPtr<ID3D12CommandAllocator> CmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> CmdList = CreateCmdList(CmdAllocator, L"UpdateDefaultBuffer");
	ComPtr<ID3D12CommandQueue> CmdQueue = CreateCmdQueue();

	//描述要复制到默认堆的数据
	D3D12_SUBRESOURCE_DATA SubResourceDesc = {};
	SubResourceDesc.pData = sourceData;
	SubResourceDesc.RowPitch = sourceSize;
	SubResourceDesc.SlicePitch = SubResourceDesc.RowPitch;

	//资源状态转为复制目标态
	CD3DX12_RESOURCE_BARRIER DefaultBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), beforeBarrier, D3D12_RESOURCE_STATE_COPY_DEST);
	CmdList->ResourceBarrier(1, &DefaultBufferBarrier);

	//更新资源
	UpdateSubresources<1>(CmdList.Get(), defaultBuffer.Get(), UploadBuffer.Get(), 0, 0, 1, &SubResourceDesc);

	//资源状态转为GENERIC_READ
	CD3DX12_RESOURCE_BARRIER DefaultBufferBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	CmdList->ResourceBarrier(1, &DefaultBufferBarrier2);

	ExeCmd(CmdList, CmdQueue);

	//等待GPU端创建资源
	FluCmdQueue(CmdQueue);
}