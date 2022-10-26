//宏定义--------------------------------------------------------------------------------------------------
#define PI 3.141592653
#define HBAORadius 1                  //遮蔽点测试半径

//结构体定义--------------------------------------------------------------------------------------------------

//全局帧资源
struct GLOBAL_RESOURCE
{
	float4x4 VPMat;              //局部到摄像机空间变换矩阵

	//相机坐标
	float3 CameraPosition;

	//时间
	float RunTime;
};

//光照信息帧资源
struct LIGHT_RESOURCE
{
	//环境光
	float3 AmbientLight;
	//平行光强
	float3 ParallelLightIntensity;
	//平行光方向
	float3 ParallelLightDirection;
};

//点光源光照
struct POINT_LIGHT
{
	//位置
	float3 Position;

	//开始衰减距离
	float DecayDistance;

	//点光源功率
	float3 Lumen;
};

struct PointLightVPTMat
{
	float4x4 VPTMat;
};

struct SHADOWMAPRESOURCE
{
	float4x4 VPMat;
};

//PBR相关参数
struct PBR_ITEM
{
	float3 F0;                      //菲涅尔系数
	float Roughness;                //粗糙度
	float Metallicity;             //金属度
};

struct OBJECT_RESOURCE
{
	float4x4 WldMat;              //局部到世界变换矩阵

	PBR_ITEM PBRItem;

};

struct DEFERRED_RENDER_RESOURCE
{
	float4x4 InverseViewMat;
	float ClientWidth;
	float ClientHeight;
	float A;
	float B;
	float TanHalfFov;
};

struct SSAO_RESOURCE
{
	float4 NormalList[14];
};

struct AO_BUFFER_BLUR_RESOURCE
{
	int Width;
	int Height;
};

struct BLUR_WEIGHT
{
	float Weight[7];
};

//常量缓冲区--------------------------------------------------------------------------------------------------

//单实例物资源
ConstantBuffer<OBJECT_RESOURCE> SingleObjectSource : register(b0);

//全局信息帧资源
ConstantBuffer<GLOBAL_RESOURCE> GlobalResource : register(b1);

//光照信息帧资源
ConstantBuffer<LIGHT_RESOURCE> AmbParaLight : register(b2);

//HBAO计算资源
ConstantBuffer<SSAO_RESOURCE> HBAOResource : register(b3);

//阴影贴图计算资源
ConstantBuffer<SHADOWMAPRESOURCE> ShadowMapResource : register(b0, space1);

//延迟渲染计算资源
ConstantBuffer<DEFERRED_RENDER_RESOURCE> DeferredRenderResource : register(b0, space2);

//模糊计算权重
ConstantBuffer<BLUR_WEIGHT> BlurWeight : register(b0, space3);

//AO图模糊计算资源
ConstantBuffer<AO_BUFFER_BLUR_RESOURCE> AOBufferBlurResource : register(b1, space3);

//纹理资源--------------------------------------------------------------------------------------------------

//顶点光源数组
StructuredBuffer<POINT_LIGHT> PointLightList : register(t0);

//单颜色贴图
Texture2D SingleAlbedoTexture : register(t1);

//实例物体帧资源
StructuredBuffer<OBJECT_RESOURCE> InstanceResource : register(t2);

//阴影贴图资源
Texture2DArray ShadowMaps : register(t3);

//点光源VPT矩阵数组
StructuredBuffer<PointLightVPTMat> PointLightVPTMatList : register(t4);

//GBuffer0: Albedo、Roughness
Texture2D GBuffer0 : register(t0, space1);

//GBuffer1: Normal、Metallicity
Texture2D GBuffer1 : register(t1, space1);

//GBuffer2: F0
Texture2D GBuffer2 : register(t2, space1);

//深度缓冲区
Texture2D DepthGBuffer : register(t3, space1);

//AO GBuffer
Texture2D AOGBuffer : register(t4, space1);

//无序访问资源------------------------------------------------------------------------------------------------------

//Albedo
RWTexture2D<float4> AlbedoBuffer : register(u0);

//Normal
RWTexture2D<float4> NormalBuffer : register(u1);

//Depth
RWTexture2D<float4> DepthBuffer : register(u2);

//AOBuffer Vague版
RWTexture2D<float2> AOBlurBuffer : register(u4);

//RealRT
//RWTexture2D<unorm float4> RealRT : register(u4);

//采样器------------------------------------------------------------------------------------------------------
SamplerState Sampler0 : register(s0);
//SamplerComparisonState ShadowSampler : register(s1);
SamplerState ShadowSampler : register(s1);
SamplerState SSAOSampler : register(s2);

//着色器输入输出定义----------------------------------------------------------------------------------------------------

//顶点着色器输入
struct VertexIn
{
	float3 LocalPos : POSITION;
	float3 Normal : NORMAL;
	float2 TexC : TEXCOORD;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 ProjPos : SV_POSITION;
	float3 WldPos : POSITION;
	float3 Normal : NORMAL;
	float2 TexC : TEXCOORD;
	float4 Color : COLOR;
	uint  InstanceID : SV_InstanceID;
};

struct PixelOutDeferredShading
{
	float4 Albedo_Roughness : SV_Target0;
	float4 Normal_Metallicity : SV_Target1;
	float4 F0 : SV_Target2;
};

//工具函数----------------------------------------------------------------------------------------------------

//从投影空间转换到摄像机空间
float4 ProjToView(float4 projPos,float width,float height,float a,float b,float tanHalfFov)
{
	float4 ViewPos;

	float AspectRatio = width / height;
	ViewPos.z = b / (projPos.z - a);
	ViewPos.x = tanHalfFov * projPos.x * ViewPos.z * AspectRatio;
	ViewPos.y = tanHalfFov * projPos.y * ViewPos.z;
	ViewPos.w = 1;

	return ViewPos;
}


//ACES Tonemapping
float3 ACESToneMapping(float3 Color)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;

	return saturate((Color * (a * Color + b)) / (Color * (c * Color + d) + e));
}

//布林冯光照模型----------------------------------------------------------------------------------------------------

float3 ACESToneMapping(float3 color, float adapted_lum)
{
	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;

	color *= adapted_lum;
	return (color * (A * color + B)) / (color * (C * color + D) + E);
}

//计算漫反射+镜面反射
float4 ComputeReflect(float3 Normal, float4 Albedo, float Roughness, float3 InLight, float3 Intensity, float3 WldPos)
{
	Normal = normalize(Normal);

	//Intensity *= max(0, dot(-InLight, Normal));

	//漫反射
	float4 Diffuse = float4(Intensity, 0.f) * Albedo * max(0, dot(Normal, -InLight));

	//镜面反射高光
	float3 ToCam = normalize(GlobalResource.CameraPosition - WldPos);       //从物表面一点点到摄像机的向量
	float3 bisector = normalize(ToCam - InLight);         //半程向量
	float4  Specular = float4(Intensity, 0.f) * Albedo * pow(max(0, dot(Normal, bisector)), Roughness);

	return Diffuse + Specular;
}

/*布林冯光照模型*/
//float4 BlinnPhong(float3 normal, float4 albedo, float3 wldPos, float roughness)
//{
//	float4 Result = { 0,0,0,0 };
//
//	//环境光照
//	Result += float4(AmbParaLight.AmbientLight, 1.f) * albedo;
//
//	//平行光
//	Result += ComputeReflect(normal, albedo, roughness, normalize(AmbParaLight.ParallelLightDirection), AmbParaLight.ParallelLightIntensity, wldPos);
//
//	//计算点光源光照
//	uint Num;
//	uint Stride;
//	PointLightList.GetDimensions(Num, Stride);
//
//	for (uint i = 0; i < Num; i++)
//	{
//		float3 PointPosition = PointLightList[i].Position;
//
//		float3 Direction = normalize(wldPos - PointPosition);
//
//		float3 Intensity = PointLightList[i].Intensity;
//
//		//距点光源距离
//		float Distance = distance(PointPosition, wldPos);
//
//		//判断是否衰减
//		if (Distance > PointLightList[i].DecayDistance)
//		{
//			Intensity /= pow(Distance, 2);
//		}
//
//		Result += ComputeReflect(normal, albedo, roughness, Direction, Intensity, wldPos);
//	}
//
//	return Result;
//}

//PBR光照模型----------------------------------------------------------------------------------------------------
float G_Schlick(float roughness, float cosAngel)
{
	float Kdirect = pow((roughness + 1), 2) / 8;                     //点光源换算系数

	float Schlick = cosAngel / (cosAngel * (1 - Kdirect) + Kdirect);

	return Schlick;
}

float3 CookTorrance(float roughness, float3 bisector, float3 normal, float3 f0, float3 toCamera, float3 toLight, out float3 ks)
{
	float NdotC = max(0.0001, dot(toCamera, normal));                //cos(法线和入射光夹角)
	float NdotL = max(0.0001, dot(toLight, normal));                 //cos(法线和出射光夹角)

	//计算D项
	float D = pow(roughness, 2) / (PI * pow((max(0.0001f, dot(bisector, normal)) * (pow(roughness, 2) - 1) + 1), 2));

	//计算F项
	float3 F = f0 + (1 - f0) * pow((1 - max(0, dot(bisector, normal))), 5);
	ks = F;

	//计算G项
	float GGXInput = G_Schlick(roughness, NdotC);
	float GGXOuput = G_Schlick(roughness, NdotL);
	float G = GGXInput * GGXOuput;

	//计算镜面反射系数
	float3 Specular = D * F * G / (4 * NdotC * NdotL);

	return Specular;
}

float4 PBRLight(float3 normal, float3 albedo, float3 wldPos, PBR_ITEM pBRItem)
{
	float4 Result = { 0,0,0,1 };

	//获得点光源数量
	uint Num;
	uint Stride;
	PointLightList.GetDimensions(Num, Stride);

	for (uint LightIndex = 0; LightIndex < Num; LightIndex++)
	{
		float3 LightPosition = PointLightList[LightIndex].Position;                //点光源位置
		float3 ToLight = normalize(LightPosition - wldPos);           //被照射点到光源法向量
		float3 ToCamera = normalize(GlobalResource.CameraPosition - wldPos);         //被照射点到相机法向量
		float3 Bisector = normalize(ToCamera + ToLight);                 //半程向量
		float ToLightDistance = distance(LightPosition, wldPos);      //到点光源距离
		float3 LightInputLumen = PointLightList[LightIndex].Lumen / pow(ToLightDistance, 2);        //入射光强度

	//计算镜面反射系数：
		float3 Ks;
		float3 CookTorranceItem = CookTorrance(pBRItem.Roughness, Bisector, normal, pBRItem.F0, ToCamera, ToLight, Ks);
		float3 Specular = Ks * CookTorranceItem;

	//计算漫反射系数：漫反贡献射系数*反射系数/PI
		float3 Kd = (1 - Ks) * (1 - pBRItem.Metallicity);
		float3 Diffuse = Kd * albedo / PI;

	//计算阴影因子
		float ShadowFactor = 0;

		//转换到NDC空间
		float4 LightSpacePos = mul(float4(wldPos, 1.f), PointLightVPTMatList[LightIndex].VPTMat);
		LightSpacePos.xyz /= LightSpacePos.w;                //透视除法

		//获得阴影贴图尺寸并计算查询偏移量
		float ShadowMapWidth, ShadowMapHeight, ShadowMapElements;
		ShadowMaps.GetDimensions(ShadowMapWidth, ShadowMapHeight, ShadowMapElements);
		float dx = 1.f / ShadowMapWidth;

		//PCF
		LightSpacePos.xy -= dx;
		int CoreSize = 3;                                    //核大小
		
		[unroll]
		for (int i = 0; i < CoreSize; i++)
		{
			for (int j = 0; j < CoreSize; j++)
			{
				//ShadowFactor += ShadowMaps.SampleCmpLevelZero(ShadowSampler, 
				//	float3(LightSpacePos.xy,float(LightIndex)), LightSpacePos.z, int2(0,0)).x;

				float k = ShadowMaps.Sample(ShadowSampler, float3(LightSpacePos.x, LightSpacePos.y, float(LightIndex))).r;

				if (k.r >= LightSpacePos.z)
				{
					ShadowFactor += 1;
				}

				LightSpacePos.x += dx;
			}
			LightSpacePos.y += dx;
		}

		ShadowFactor /= pow(CoreSize, 2);

	//计算最终光照结果
		float3 LightOut = ShadowFactor * dot((Diffuse + Specular), LightInputLumen) * max(0,dot(normal, ToLight));

		Result += float4(LightOut, 0);
	}

	return Result;
}

//顶点着色器----------------------------------------------------------------------------------------------------
VertexOut VS_Common(VertexIn vin, uint instanceID : SV_InstanceID)
{
	VertexOut vout;

	//法线和顶点转换到世界空间
	vout.Normal = mul(vin.Normal, (float3x3)InstanceResource[instanceID].WldMat);
	float4 WldPos = mul(float4(vin.LocalPos, 1.f), InstanceResource[instanceID].WldMat);
	vout.WldPos = WldPos.xyz;

	//世界空间到观察空间变换
	vout.ProjPos = mul(WldPos, GlobalResource.VPMat);

	vout.TexC = vin.TexC;
	vout.Color = vin.Color;
	vout.InstanceID = instanceID;

	return vout;
}

float4 VS_CommonShadowMap(VertexIn vin, uint instanceID : SV_InstanceID) : SV_POSITION
{
	float4 WldPos = mul(float4(vin.LocalPos, 1.f), InstanceResource[instanceID].WldMat);
	float4 ProjPos = mul(WldPos, ShadowMapResource.VPMat);

	return ProjPos;
}

float4 VS_DeferredRender(uint vertexID : SV_VertexID) : SV_Position
{
	float4 Result = {-1,1,1,1};
	// 使用一个三角形覆盖NDC空间 
	// (-1, 1)________ (3, 1)
	// (-1,-1)|___|/ (1, -1)
	// (-1,-3)|/ 


	if(vertexID == 1)
	{
		Result.xy = float2(3, 1);
	}
	if(vertexID == 2)
	{
		Result.xy = float2(-1, -3);
	}

	return Result;
}

//像素着色器----------------------------------------------------------------------------------------------------
float4 PS_Common(VertexOut pin) : SV_Target
{
	float4 Result;

	//旋转功能
	float RunSpeed = 1;
	float2x2 transmat = { cos(RunSpeed * GlobalResource.RunTime),sin(RunSpeed * GlobalResource.RunTime),-sin(RunSpeed * GlobalResource.RunTime),cos(RunSpeed * GlobalResource.RunTime) };
	float2 uv = pin.TexC;
	uv.x -= 0.5;
	uv.y -= 0.5;
	uv = mul(uv, transmat);
	uv.x += 0.5;
	uv.y += 0.5;

	//采样获得反射系数
	float3 Albedo = SingleAlbedoTexture.Sample(Sampler0, uv).xyz;

	//法向量单位化
	float3 Normal = normalize(pin.Normal);

	//布林冯光照
	//Result = BlinnPhong(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	//PBR：漫反射和镜面反射
	Result = PBRLight(Normal, Albedo, pin.WldPos, InstanceResource[pin.InstanceID].PBRItem);

	//tone mapping
	Result = Result / (1 + Result); 

	return Result;
}

PixelOutDeferredShading PS_GBuffer(VertexOut pin)
{
	PixelOutDeferredShading Result;

	//旋转功能
	float RunSpeed = 1;
	float2x2 transmat = { cos(RunSpeed * GlobalResource.RunTime),sin(RunSpeed * GlobalResource.RunTime),-sin(RunSpeed * GlobalResource.RunTime),cos(RunSpeed * GlobalResource.RunTime) };
	float2 uv = pin.TexC;
	//uv.x -= 0.5;
	//uv.y -= 0.5;
	//uv = mul(uv, transmat);
	//uv.x += 0.5;
	//uv.y += 0.5;

	//采样获得反射系数
	float3 Albedo = SingleAlbedoTexture.Sample(Sampler0, uv).xyz;

	Result.Albedo_Roughness = float4(Albedo, InstanceResource[pin.InstanceID].PBRItem.Roughness);

	Result.Normal_Metallicity = float4(pin.Normal, InstanceResource[pin.InstanceID].PBRItem.Metallicity);

	Result.F0 = float4(InstanceResource[pin.InstanceID].PBRItem.F0, 1);

	return Result;
}

float2 PS_HBAO(float4 svPos : SV_Position) : SV_Target
{
	float AO = 0;

//屏幕坐标
	uint ScreenX = svPos.x;
    uint ScreenY = svPos.y;

//获得深度
	float Depth = DepthGBuffer.Load(int3(ScreenX, ScreenY, 0), 0).x;

	if(Depth < 1)
	{
	//计算世界空间坐标
		//投影空间坐标
		float4 ProjPos;
		ProjPos.x = float(ScreenX) * 2 / DeferredRenderResource.ClientWidth - 1;
		ProjPos.y = -(float(ScreenY) * 2 / DeferredRenderResource.ClientHeight - 1);
		ProjPos.z = Depth;
		ProjPos.w = 1;

		//计算摄像机空间坐标
		float AspectRatio = (float)DeferredRenderResource.ClientWidth / (float)DeferredRenderResource.ClientHeight;
		float4 ViewPos;
		ViewPos.z = DeferredRenderResource.B / (Depth - DeferredRenderResource.A);
		ViewPos.x = DeferredRenderResource.TanHalfFov * ProjPos.x * ViewPos.z * AspectRatio;
		ViewPos.y = DeferredRenderResource.TanHalfFov * ProjPos.y * ViewPos.z;
		ViewPos.w = 1;

		//计算世界空间坐标
		float4 WldPos = mul(ViewPos, DeferredRenderResource.InverseViewMat);

	//采样
		//半球向量
		float3 Normal = normalize(GBuffer1.Load(int3(ScreenX, ScreenY, 0), 0).xyz);

		//对14个点进行采样
		for (int i = 0; i < 14; i++)
		{
			//计算采样向量是否在半球空间内，不在则转换到半球空间中
			float4 Point = 0.75 * float4(normalize(HBAOResource.NormalList[i].xyz), 0);
			if (dot(Normal, Point.xyz) < 0)
			{
				Point = -Point;
			}
			Point += WldPos;

			//采样点转换到NDC空间
			float4 PointNDC = mul(Point, GlobalResource.VPMat);
			PointNDC.xyzw /= PointNDC.w;

			//NDC空间到[0,1]
			float2 SamplingPointUV = PointNDC.xy;
			SamplingPointUV.x += 1;
			SamplingPointUV.x /= 2;
			SamplingPointUV.y += 1;
			SamplingPointUV.y /= 2;
			SamplingPointUV.y = 1 - SamplingPointUV.y;

			//采样深度值
			PointNDC.z = DepthGBuffer.Sample(SSAOSampler, SamplingPointUV).x;

			//转换到View空间
			float4 PointViewPos = ProjToView(PointNDC, (float)DeferredRenderResource.ClientWidth, (float)DeferredRenderResource.ClientHeight,
				DeferredRenderResource.A, DeferredRenderResource.B, DeferredRenderResource.TanHalfFov);
			//float AspectRatio = (float)DeferredRenderResource.ClientWidth / (float)DeferredRenderResource.ClientHeight;
			//float4 PointViewPos;
			//PointViewPos.z = DeferredRenderResource.B / (PointNDC.z - DeferredRenderResource.A);
			//PointViewPos.x = DeferredRenderResource.TanHalfFov * PointNDC.x * PointViewPos.z * AspectRatio;
			//PointViewPos.y = DeferredRenderResource.TanHalfFov * PointNDC.y * PointViewPos.z;
			//PointViewPos.w = 1;

			Point = mul(PointViewPos, DeferredRenderResource.InverseViewMat);

			//计算遮蔽因子
			float4 ToPoint = normalize(Point - WldPos);
			float Factor = max(dot(ToPoint.xyz, Normal.xyz), 0);

			//计算遮蔽值
			float Distance = distance(Point, WldPos);
			float OcclusionVal = max(HBAORadius - Distance, 0);

			//最终结果
			AO += Factor * OcclusionVal;
		}
		AO /= 14;
	}

	return float2(1 - AO, 0);

}

float4 PS_DeferredShading(float4 svPos : SV_Position) : SV_Target
{
	//屏幕坐标
	uint ScreenX = svPos.x;
    uint ScreenY = svPos.y;
	
	//PBR项数据
	PBR_ITEM PBRItem;

//解析GBuffer数据
	//获得Albedo和Roughness
	float4 Albedo_Roughness = GBuffer0.Load(int3(ScreenX, ScreenY, 0), 0);
	PBRItem.Roughness = Albedo_Roughness.w;
	
	//获得Normal和Metallicity
	float4 Normal_Metallicity = GBuffer1.Load(int3(ScreenX, ScreenY, 0), 0);
	float3 Normal = normalize(Normal_Metallicity.xyz);
	PBRItem.Metallicity = Normal_Metallicity.w;

	//获得F0
	PBRItem.F0 = GBuffer2.Load(int3(ScreenX, ScreenY, 0), 0).xyz;

	//获得深度
	float Depth = DepthGBuffer.Load(int3(ScreenX, ScreenY, 0), 0).x;

	//获得AO值
	float AO = AOGBuffer.Load(int3(ScreenX, ScreenY, 0), 0).x;

//计算世界空间坐标
	//投影空间坐标
	float4 ProjPos;
	ProjPos.x = float(ScreenX) * 2 / DeferredRenderResource.ClientWidth - 1;
	ProjPos.y = -(float(ScreenY) * 2 / DeferredRenderResource.ClientHeight - 1);
	ProjPos.z = Depth;
	ProjPos.w = 1;

	//计算摄像机空间坐标
	float4 ViewPos = ProjToView(ProjPos, (float)DeferredRenderResource.ClientWidth, (float)DeferredRenderResource.ClientHeight,
		DeferredRenderResource.A, DeferredRenderResource.B, DeferredRenderResource.TanHalfFov);
	//float AspectRatio = (float)DeferredRenderResource.ClientWidth / (float)DeferredRenderResource.ClientHeight;
	//float4 ViewPos;
	//ViewPos.z = DeferredRenderResource.B / (Depth - DeferredRenderResource.A);
	//ViewPos.x = DeferredRenderResource.TanHalfFov * ProjPos.x * ViewPos.z * AspectRatio;
	//ViewPos.y = DeferredRenderResource.TanHalfFov * ProjPos.y * ViewPos.z;
	//ViewPos.w = 1;

	//计算世界空间坐标
	float4 WldPos = mul(ViewPos, DeferredRenderResource.InverseViewMat);

//计算光照
	float4 PBR = PBRLight(Normal, Albedo_Roughness.xyz, WldPos.xyz, PBRItem);       //PBR光照
	float4 HBAO = float4(AO * AmbParaLight.AmbientLight * Albedo_Roughness.xyz, 1);            //环境光遮蔽

	//tone mapping
	float4 Color = float4(ACESToneMapping((PBR + HBAO).xyz), 1);

	return Color;
	
}

//计算着色器----------------------------------------------------------------------------------------------------

//共享内存，存放行像素数据
groupshared float AORowBuffer[1920 * 4];

[numthreads(512,1,1)]
void CS_AOBufferBlurRow(int3 groupID : SV_GroupID, int3 gThreadID : SV_GroupThreadID)
{
	uint Width = AOBufferBlurResource.Width;                 //行宽度
	uint RowIndex = groupID.x;                                //行序数
	uint ID = gThreadID.x;                                    //组内ID

	//计算要循环多少组
	uint LoopNum = Width / 512 + 1;

	//将本行数据采样进共享内存中
	for (uint i = 0; i < LoopNum; i++)
	{
		uint Index = 512 * i + ID;                             //处理像素序数
		if (Index < Width)
		{
			AORowBuffer[Index] = AOBlurBuffer[int2(Index, RowIndex)].x;
		}
		else
			break;
	}
	
	//组内线程同步
	GroupMemoryBarrierWithGroupSync();

	//模糊
	for (uint k = 0; k < LoopNum; k++)
	{
		uint Index = 512 * k + ID;                             //处理像素序数

		if (Index < Width)
		{
			float Val = 0;
			for (int j = 0; j < 7; j++)
			{
				int ClampIndex = clamp(Index + j - 3, 0, Width);
				Val += AORowBuffer[ClampIndex] * BlurWeight.Weight[j];
			}
			AOBlurBuffer[int2(Index, RowIndex)] = float2(Val,0);
		}
		else
			break;
	}
}

//共享内存，存放列像素数据
groupshared float AOColBuffer[1080 * 4];

[numthreads(512, 1, 1)]
void CS_AOBufferBlurCol(int3 groupID : SV_GroupID, int3 gThreadID : SV_GroupThreadID)
{
	uint Height = AOBufferBlurResource.Height;                 //列高度
	uint ColIndex = groupID.x;                                //列序数
	uint ID = gThreadID.x;                                    //组内ID

	//计算要循环多少组
	uint LoopNum = Height / 512 + 1;

	//将本行数据采样进共享内存中
	for (uint i = 0; i < LoopNum; i++)
	{
		uint Index = 512 * i + ID;                             //处理像素序数
		if (Index < Height)
		{
			AOColBuffer[Index] = AOBlurBuffer[int2(ColIndex, Index)].x;
		}
		else
			break;
	}

	//组内线程同步
	GroupMemoryBarrierWithGroupSync();

	//模糊
	for (uint k = 0; k < LoopNum; k++)
	{
		uint Index = 512 * k + ID;                             //处理像素序数

		if (Index < Height)
		{
			float Val = 0;
			for (int j = 0; j < 7; j++)
			{
				int ClampIndex = clamp(Index + j - 3, 0, Height);
				Val += AOColBuffer[ClampIndex] * BlurWeight.Weight[j];
			}
			AOBlurBuffer[int2(ColIndex, Index)] = float2(Val, 0);
		}
		else
			break;
	}
}