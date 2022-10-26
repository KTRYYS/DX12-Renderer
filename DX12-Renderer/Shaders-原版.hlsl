struct OBJECT_SOURCE_0
{
	//局部到摄像机空间变换矩阵
	float4x4 MVPMat;
	//局部到世界变换矩阵
	float4x4 WldMat;

	//物体粗糙度
	float Roughness;
};

//全局帧资源
struct GLOBAL_RESOURCE
{
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

	//光强
	float3 Intensity;
};

struct PointLightVPTMat
{
	float4x4 VPTMat;
};

struct SHADOWMAPRESOURCE
{
	float4x4 VPMat;
};

//常量缓冲区--------------------------------------------------------------------------------------------------

//单实例物资源
ConstantBuffer<OBJECT_SOURCE_0> SingleObjectSource : register(b0);

//全局信息帧资源
ConstantBuffer<GLOBAL_RESOURCE> GlobalResource : register(b1);

//光照信息帧资源
ConstantBuffer<LIGHT_RESOURCE> AmbParaLight : register(b2);

//
ConstantBuffer<SHADOWMAPRESOURCE> ShadowMapResource : register(b0, space1);

//着色器资源--------------------------------------------------------------------------------------------------

//顶点光源数组
StructuredBuffer<POINT_LIGHT> PointLightList : register(t0);

//单颜色贴图
Texture2D SingleAlbedoTexture : register(t1);

//实例物体帧资源
StructuredBuffer<OBJECT_SOURCE_0> InstanceResource : register(t2);

//阴影贴图资源
Texture2DArray ShadowMaps : register(t3);

//点光源VPT矩阵数组
StructuredBuffer<PointLightVPTMat> PointLightVPTMatList : register(t4);

//采样器------------------------------------------------------------------------------------------------------
SamplerState Sampler0 : register(s0);
//SamplerComparisonState ShadowSampler : register(s1);
SamplerState ShadowSampler : register(s1);

//定义----------------------------------------------------------------------------------------------------

struct VertexIn
{
	float3 LocalPos : POSITION;
	float3 Normal : NORMAL;
	float2 TexC : TEXCOORD;
	float4 Color : COLOR;
};

struct VertexIn_PC
{
	float3 LocalPos : POSITION;
	float4 Color : COLOR;
};

struct VertexOut_PC
{
	float4 ProjPos  : SV_POSITION;
	float4 Color:COLOR;
};

struct VertexOut_PC_1
{
	float3 LocalPos : POSITION;
	float4 Color : COLOR;
};

struct VertexIn_PNT
{
	float3 LocalPos : POSITION;
	float3 Normal : NORMAL;
	float2 TexC : TEXCOORD;
};

struct VertexOut_PPNT
{
	float4 ProjPos : SV_POSITION;
	float3 WldPos : POSITION;
	float3 Normal : NORMAL;
	float2 TexC : TEXCOORD;
};

struct PatchTess
{
	//4个边缘如何划分
	float EdgeTess[4] : SV_TessFactor;

	//内部细分列数和行数
	float InsideTess[2] : SV_InsideTessFactor;
};

struct HSOut_P_C
{
	float3 LocalPos : POSITION;
	float4 Color : COLOR;
};

struct DSOut_P_C
{
	float4 ProjPos  : SV_POSITION;
	float4 Color:COLOR;
};

struct DSOut_PPNT
{
	float4 ProjPos : SV_POSITION;
	float3 WldPos : POSITION;
	float3 Normal : NORMAL;
	float2 TexC : TEXCOORD;
};

//辅助函数----------------------------------------------------------------------------------------------------

//计算反射（漫反射、镜面反射）
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

//计算环境光照、平行光照
float4 CalcuAmbParaLight(float3 normal, float4 albedo, float3 wldPos, float roughness)
{
	//环境光照
	float4 AmbientLightOut = float4(AmbParaLight.AmbientLight,1.f) * albedo;

	//平行光照
	float4 ParallelLightOut = ComputeReflect(normal, albedo, roughness, normalize(AmbParaLight.ParallelLightDirection), AmbParaLight.ParallelLightIntensity, wldPos);

	return AmbientLightOut + ParallelLightOut;
}

//计算点光源光照
float4 CalcuPointLight(float3 normal, float4 albedo, float3 wldPos, float roughness)
{
	float4 PointLightOut = { 0,0,0,0 };

	uint Num;
	uint Stride;
	PointLightList.GetDimensions(Num, Stride);

	//点光源绕Y轴旋转
	float3x3 RotateMat = 
	{cos(GlobalResource.RunTime),0,-sin(GlobalResource.RunTime),
		0,1,0,
	sin(GlobalResource.RunTime),0,cos(GlobalResource.RunTime)};

	for (uint i = 0; i < Num; i++)
	{
		float3 PointPosition = mul(PointLightList[i].Position, RotateMat);

		float3 Direction = normalize(wldPos - PointPosition);

		float3 Intensity = PointLightList[i].Intensity;

		//距点光源距离
		float Distance = distance(PointPosition, wldPos);

		//判断是否衰减
		if (Distance > PointLightList[i].DecayDistance)
		{
			Intensity /= pow(Distance, 2);
		}

		PointLightOut += ComputeReflect(normal, albedo, roughness, Direction, Intensity, wldPos);
	}

	return PointLightOut;
}

//着色器------------------------------------------------------------------------------------------------------
VertexOut_PC VS_0(VertexIn_PC vin)
{
	VertexOut_PC vout;

	//物体空间到观察空间变换
	vout.ProjPos = mul(float4(vin.LocalPos, 1.0f), SingleObjectSource.MVPMat);

	//颜色不变
	vout.Color = vin.Color;

	return vout;
}

float4 PS_0(VertexOut_PC pin) : SV_Target
{
	return pin.Color;
}

VertexOut_PPNT VS_1(VertexIn_PNT vin)
{
	VertexOut_PPNT vout;

	//物体空间到观察空间变换
	vout.ProjPos = mul(float4(vin.LocalPos, 1.0f), SingleObjectSource.MVPMat);

	//法线和顶点转换到世界空间
	vout.Normal = mul(vin.Normal, (float3x3)SingleObjectSource.WldMat);
	vout.WldPos = mul(vin.LocalPos, (float3x3)SingleObjectSource.WldMat);

	vout.TexC = vin.TexC;

	return vout;
}

float4 PS_1(VertexOut_PPNT pin) : SV_Target
{
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
	float4 Albedo = SingleAlbedoTexture.Sample(Sampler0, uv);

//像素数据预处理
	float3 Normal = normalize(pin.Normal);

	float4 Result;

	Result = CalcuAmbParaLight(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	Result += CalcuPointLight(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);
	
	return Result;
}

//曲面细分1----------------------------------------------------------------------------------------------

VertexIn_PC VS_2(VertexIn_PC vin)
{
	return vin;
}

//常量外壳着色器
PatchTess ConstantHS(InputPatch<VertexOut_PC_1, 4>patch,    //InputPatch表示以面片控制点作为输入，VertexOut_0是自定义的顶点着色器输出控制点，4是可自定义的数量
	uint patchID : SV_PrimitiveID)                       //系统通过SV_PrimitiveID提供面片的ID值
{
	PatchTess pt;

	//float i = 4 * GlobalResource.RunTime % 64;

	//if (i > 32)
	//{
	//	i = 64 - i;
	//}

	float i = 64;

	pt.EdgeTess[0] = i;
	pt.EdgeTess[1] = i;
	pt.EdgeTess[2] = i;
	pt.EdgeTess[3] = i;

	pt.InsideTess[0] = i;
	pt.InsideTess[1] = i;

	return pt;
}

//控制点着色器
[domain("quad")][partitioning("fractional_even")][outputtopology("triangle_cw")]
[outputcontrolpoints(4)][patchconstantfunc("ConstantHS")][maxtessfactor(64.0f)]
HSOut_P_C HS_0(InputPatch<VertexOut_PC_1, 4> ControlPoint, uint i : SV_OutputControlPointID,
	uint patchId : SV_PrimitiveID)
{
	HSOut_P_C Out;
	Out.LocalPos = ControlPoint[i].LocalPos;
	Out.Color = ControlPoint[i].Color;

	return Out;
}

//域着色器
[domain("quad")]
DSOut_P_C DS_0(PatchTess patchTess, float2 uv : SV_DomainLocation,
	const OutputPatch<HSOut_P_C, 4> quad)
{
	DSOut_P_C DSOut;

//双线性插值
	//位置
	float3 v1 = lerp(quad[2].LocalPos, quad[3].LocalPos, uv.x);
	float3 v2 = lerp(quad[1].LocalPos, quad[0].LocalPos, uv.x);
	float3 p = lerp(v1, v2, uv.y);

	p.y += sin(8*(GlobalResource.RunTime + uv.y))/12;
	p.y += sin(8 * (GlobalResource.RunTime + uv.x)) / 12;

	//颜色
	float4 c1 = lerp(quad[2].Color, quad[3].Color, uv.x);
	float4 c2 = lerp(quad[1].Color, quad[0].Color, uv.x);
	float4 c = lerp(c1, c2, uv.y);

	DSOut.ProjPos = mul(float4(p, 1.0f), SingleObjectSource.MVPMat);
	DSOut.Color = c;

	return DSOut;
}

//曲面细分2----------------------------------------------------------------------------------------------

VertexIn_PNT VS_3(VertexIn_PNT vin)
{
	VertexIn_PNT Out;
	Out.LocalPos = vin.LocalPos;
	Out.Normal = vin.Normal;
	Out.TexC = vin.TexC;
	return vin;
}

PatchTess ConstantHS_2(InputPatch<VertexIn_PNT, 4>patch,    //InputPatch表示以面片控制点作为输入，VertexOut_0是自定义的顶点着色器输出控制点，4是可自定义的数量
	uint patchID : SV_PrimitiveID)                       //系统通过SV_PrimitiveID提供面片的ID值
{
	PatchTess pt;

	float i = 4 * GlobalResource.RunTime % 128;

	if (i > 64)
	{
		i = 128 - i;
	}

	pt.EdgeTess[0] = i;
	pt.EdgeTess[1] = i;
	pt.EdgeTess[2] = i;
	pt.EdgeTess[3] = i;

	pt.InsideTess[0] = i;
	pt.InsideTess[1] = i;

	return pt;
}

//控制点着色器
[domain("quad")][partitioning("fractional_even")][outputtopology("triangle_cw")]
[outputcontrolpoints(4)] [patchconstantfunc("ConstantHS_2")][maxtessfactor(64.0f)]
VertexIn_PNT HS_1(InputPatch<VertexIn_PNT, 4> ControlPoint, uint i : SV_OutputControlPointID,
	uint patchId : SV_PrimitiveID)
{
	VertexIn_PNT Out;
	Out.LocalPos = ControlPoint[i].LocalPos;
	Out.Normal = ControlPoint[i].Normal;
	Out.TexC = ControlPoint[i].TexC;

	return Out;
}

//域着色器
[domain("quad")]
DSOut_PPNT DS_1(PatchTess patchTess, float2 uv : SV_DomainLocation,
	const OutputPatch<VertexIn_PNT, 4> quad)
{
	DSOut_PPNT DSOut;

	//位置
	float3 v1 = lerp(quad[0].LocalPos, quad[1].LocalPos, uv.x);
	float3 v2 = lerp(quad[2].LocalPos, quad[3].LocalPos, uv.x);
	float3 p = lerp(v1, v2, uv.y);
	float3 position = normalize(p) * length(quad[0].LocalPos);



	//TexC
	float2 t1 = lerp(quad[0].TexC, quad[1].TexC, uv.x);
	float2 t2 = lerp(quad[2].TexC, quad[3].TexC, uv.x);
	float2 texc = lerp(t1, t2, uv.y);

	DSOut.ProjPos = mul(float4(position, 1.0f), SingleObjectSource.MVPMat);
	DSOut.WldPos = mul(position, (float3x3)SingleObjectSource.WldMat);
	DSOut.Normal = DSOut.WldPos;
	DSOut.TexC = texc;

	return DSOut;
}

//水----------------------------------------------------------------------------------------------
struct VertexOut_Water
{
	float3 LocalPos : POSITION;
	float4 Color:COLOR;
	uint InstanceID : SV_InstanceID;
};

VertexOut_Water VS_Water(VertexIn_PC vin, uint instanceId : SV_InstanceID)
{
	VertexOut_Water Vout;

	Vout.LocalPos = vin.LocalPos;
	Vout.Color = vin.Color;
	Vout.InstanceID = instanceId;

	return Vout;
}

PatchTess ConstantHS_Water(InputPatch<VertexOut_Water, 4>patch,    //InputPatch表示以面片控制点作为输入，VertexOut_0是自定义的顶点着色器输出控制点，4是可自定义的数量
	uint patchID : SV_PrimitiveID)                       //系统通过SV_PrimitiveID提供面片的ID值
{
	PatchTess pt;

	//float i = 4 * GlobalResource.RunTime % 128;
	//if (i > 64)
	//{
	//	i = 128 - i;
	//}
	float i = 64;

	pt.EdgeTess[0] = i;
	pt.EdgeTess[1] = i;
	pt.EdgeTess[2] = i;
	pt.EdgeTess[3] = i;

	pt.InsideTess[0] = i;
	pt.InsideTess[1] = i;

	return pt;
}

//控制点着色器
[domain("quad")] [partitioning("fractional_even")] [outputtopology("triangle_cw")]
[outputcontrolpoints(4)] [patchconstantfunc("ConstantHS_Water")] [maxtessfactor(64.0f)]
VertexOut_Water HS_Water(InputPatch<VertexOut_Water, 4> ControlPoint, uint i : SV_OutputControlPointID,
	uint patchId : SV_PrimitiveID)
{
	VertexOut_Water Out;
	Out.LocalPos = ControlPoint[i].LocalPos;
	Out.Color = ControlPoint[i].Color;
	Out.InstanceID = ControlPoint[i].InstanceID;

	return Out;
}

//域着色器
struct DSOut_PPCN
{
	float4 ProjPos  : SV_POSITION;
	float3 WldPos  : POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
};

[domain("quad")]
DSOut_PPCN DS_Water(PatchTess patchTess, float2 uv : SV_DomainLocation,
	const OutputPatch<VertexOut_Water, 4> quad)
{
	DSOut_PPCN DSOut;

	//位置
	float3 v1 = lerp(quad[0].LocalPos, quad[1].LocalPos, uv.x);
	float3 v2 = lerp(quad[2].LocalPos, quad[3].LocalPos, uv.x);
	float3 p = lerp(v1, v2, uv.y);

	p.y += sin(2 * (GlobalResource.RunTime + uv.y * 2 * 3.141592653)) / 12;
	p.y += sin(2 * (GlobalResource.RunTime + uv.x * 2 * 3.141592653)) / 12;

	//输出位置、颜色
	DSOut.ProjPos = mul(float4(p, 1.0f), InstanceResource[quad[0].InstanceID].MVPMat);
	DSOut.WldPos = mul(p, (float3x3)InstanceResource[quad[0].InstanceID].WldMat);
	DSOut.Color = quad[0].Color;

	//输出向量
	float3 normal;
	normal.x = -4 * 3.141592653 * cos(2 * (GlobalResource.RunTime + uv.y * 2 * 3.141592653));
	normal.y = 12;
	normal.z = -4 * 3.141592653 * cos(2 * (GlobalResource.RunTime + uv.x * 2 * 3.141592653));

	DSOut.Normal = mul(normalize(normal), (float3x3)InstanceResource[quad[0].InstanceID].WldMat);

	return DSOut;
}

float4 PS_Water(DSOut_PPCN pin) : SV_Target
{
	//反射系数
	float4 Albedo = pin.Color;

	float3 Normal = normalize(pin.Normal);

	float4 Result;
	
	Result = CalcuAmbParaLight(Normal, Albedo, pin.WldPos, 64);
	Result += CalcuPointLight(Normal, Albedo, pin.WldPos, 64);

	return Result;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
float4 PS_2(VertexOut_PPNT pin) : SV_Target
{
	//采样获得反射系数
	float4 Albedo = SingleAlbedoTexture.Sample(Sampler0, pin.TexC);

	//像素数据预处理
	float3 Normal = normalize(pin.Normal);

	float4 Result;

	Result = CalcuAmbParaLight(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	Result += CalcuPointLight(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	return Result;
}

//制作阴影贴图-------------------------------------------------------------------------------------------------------------------------------------------------
struct VSSHADOWOUT
{
	float4 Position : SV_Position;
};

VSSHADOWOUT VS_ShadowMap(VertexIn_PNT vin)
{
	VSSHADOWOUT Out;

	float4 re = mul(float4(vin.LocalPos, 1.f), SingleObjectSource.WldMat);
	Out.Position = mul(re, ShadowMapResource.VPMat);

	return Out;
}

[domain("quad")]
VSSHADOWOUT DS_Shadow(PatchTess patchTess, float2 uv : SV_DomainLocation,
	const OutputPatch<VertexIn_PNT, 4> quad)
{
	VSSHADOWOUT DSOut;

	//位置
	float3 v1 = lerp(quad[0].LocalPos, quad[1].LocalPos, uv.x);
	float3 v2 = lerp(quad[2].LocalPos, quad[3].LocalPos, uv.x);
	float3 p = lerp(v1, v2, uv.y);
	float3 position = normalize(p) * length(quad[0].LocalPos);

	//输出位置
	float4 pos = mul(float4(position, 1.0f), SingleObjectSource.WldMat);
	DSOut.Position = mul(pos, ShadowMapResource.VPMat);

	return DSOut;
}

void PS_ShadowMap(VSSHADOWOUT pin)
{

}

//阴影渲染-------------------------------------------------------------------------------------------------------------------------------------------------
float4 PointLight_Shadow(float3 normal, float4 albedo, float4 wldPos, float roughness)
{
	uint PointLightNum;
	uint Stride;
	PointLightList.GetDimensions(PointLightNum, Stride);

	float4 LightOut = { 0,0,0,0 };

	for (uint LightIndex = 0; LightIndex < 1; LightIndex++)
	{
		//转换到光源摄像机空间
		float4 PointSpacePos = mul(wldPos, PointLightVPTMatList[LightIndex].VPTMat);
		PointSpacePos.xyz /= PointSpacePos.w;                                                   //透视除法

		//PCF
		float dx = 1.f / (float)4096;                   //阴影贴图每个像素距离

		float ShadowFactor = 0;

		PointSpacePos.x -= dx;
		PointSpacePos.y -= dx;
		
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				//float3 m;
				//m.xy = PointSpacePos.xy;
				//m.z = LightIndex;
				//float f = ShadowMaps.SampleCmpLevelZero(
				//	ShadowSampler, m, PointSpacePos.z);
				//uint f;
				//ShadowMaps.SampleCmpLevelZero(
				//	ShadowSampler, PointSpacePos, PointSpacePos.z, LightIndex, f);
				//if (f != 0)
				//{
				//	ShadowFactor += 1;
				//}
				//ShadowFactor += 0;
				//PointSpacePos.x += dx;
		
				float4 k = ShadowMaps.Sample(ShadowSampler,
					float3(PointSpacePos.x, PointSpacePos.y, LightIndex));
				if (k.r >= PointSpacePos.z)
				{
					ShadowFactor += 1;
				}
		
			}
			PointSpacePos.y += dx;
		}
		
		ShadowFactor /= 9;
		
		//点光源绕Y轴旋转
		//float3x3 RotateMat =
		//{ cos(GlobalResource.RunTime),0,-sin(GlobalResource.RunTime),
		//	0,1,0,
		//sin(GlobalResource.RunTime),0,cos(GlobalResource.RunTime) };

		//float3 PointPosition = mul(PointLightList[LightIndex].Position, RotateMat);
		float3 PointPosition = PointLightList[LightIndex].Position;

		//点到点光源距离
		float3 Direction = normalize(wldPos.xyz - PointPosition);

		float3 Intensity = PointLightList[LightIndex].Intensity;

		//距点光源距离
		float Distance = distance(PointPosition, wldPos.xyz);

		//判断是否衰减
		if (Distance > PointLightList[LightIndex].DecayDistance)
		{
			Intensity /= pow(Distance, 2);
		}

		LightOut += ShadowFactor * ComputeReflect(normal, albedo, roughness, Direction, Intensity, wldPos.xyz);
	}

	return LightOut;
}

struct VertexOut_PPNTS
{
	float4 ProjPos : SV_POSITION;
	float4 WldPos : POSITION;
	float3 Normal : NORMAL;
	float2 TexC : TEXCOORD;
};

VertexOut_PPNTS VS_Shadow(VertexIn_PNT vin)
{
	VertexOut_PPNTS vout;

	//物体空间到观察空间变换
	vout.ProjPos = mul(float4(vin.LocalPos, 1.0f), SingleObjectSource.MVPMat);

	//法线和顶点转换到世界空间
	vout.Normal = mul(vin.Normal, (float3x3)SingleObjectSource.WldMat);
	vout.WldPos = mul(float4(vin.LocalPos, 1.0f), SingleObjectSource.WldMat);

	vout.TexC = vin.TexC;

	return vout;
}

float4 PS_Shadow(VertexOut_PPNTS pin) : SV_Target
{
	//旋转功能
	//float RunSpeed = 1;
	//float2x2 transmat = { cos(RunSpeed * GlobalResource.RunTime),sin(RunSpeed * GlobalResource.RunTime),-sin(RunSpeed * GlobalResource.RunTime),cos(RunSpeed * GlobalResource.RunTime) };
	//float2 uv = pin.TexC;
	//uv.x -= 0.5;
	//uv.y -= 0.5;
	//uv = mul(uv, transmat);
	//uv.x += 0.5;
	//uv.y += 0.5;

	//采样获得反射系数
	float4 Albedo = SingleAlbedoTexture.Sample(Sampler0, pin.TexC);

	//像素数据预处理
	float3 Normal = normalize(pin.Normal);

	float4 Result;

	//Result = CalcuAmbParaLight(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	Result = PointLight_Shadow(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	return Result;
}







