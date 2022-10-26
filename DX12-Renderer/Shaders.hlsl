//�궨��--------------------------------------------------------------------------------------------------
#define PI 3.141592653
#define HBAORadius 1                  //�ڱε���԰뾶

//�ṹ�嶨��--------------------------------------------------------------------------------------------------

//ȫ��֡��Դ
struct GLOBAL_RESOURCE
{
	float4x4 VPMat;              //�ֲ���������ռ�任����

	//�������
	float3 CameraPosition;

	//ʱ��
	float RunTime;
};

//������Ϣ֡��Դ
struct LIGHT_RESOURCE
{
	//������
	float3 AmbientLight;
	//ƽ�й�ǿ
	float3 ParallelLightIntensity;
	//ƽ�йⷽ��
	float3 ParallelLightDirection;
};

//���Դ����
struct POINT_LIGHT
{
	//λ��
	float3 Position;

	//��ʼ˥������
	float DecayDistance;

	//���Դ����
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

//PBR��ز���
struct PBR_ITEM
{
	float3 F0;                      //������ϵ��
	float Roughness;                //�ֲڶ�
	float Metallicity;             //������
};

struct OBJECT_RESOURCE
{
	float4x4 WldMat;              //�ֲ�������任����

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

//����������--------------------------------------------------------------------------------------------------

//��ʵ������Դ
ConstantBuffer<OBJECT_RESOURCE> SingleObjectSource : register(b0);

//ȫ����Ϣ֡��Դ
ConstantBuffer<GLOBAL_RESOURCE> GlobalResource : register(b1);

//������Ϣ֡��Դ
ConstantBuffer<LIGHT_RESOURCE> AmbParaLight : register(b2);

//HBAO������Դ
ConstantBuffer<SSAO_RESOURCE> HBAOResource : register(b3);

//��Ӱ��ͼ������Դ
ConstantBuffer<SHADOWMAPRESOURCE> ShadowMapResource : register(b0, space1);

//�ӳ���Ⱦ������Դ
ConstantBuffer<DEFERRED_RENDER_RESOURCE> DeferredRenderResource : register(b0, space2);

//ģ������Ȩ��
ConstantBuffer<BLUR_WEIGHT> BlurWeight : register(b0, space3);

//AOͼģ��������Դ
ConstantBuffer<AO_BUFFER_BLUR_RESOURCE> AOBufferBlurResource : register(b1, space3);

//������Դ--------------------------------------------------------------------------------------------------

//�����Դ����
StructuredBuffer<POINT_LIGHT> PointLightList : register(t0);

//����ɫ��ͼ
Texture2D SingleAlbedoTexture : register(t1);

//ʵ������֡��Դ
StructuredBuffer<OBJECT_RESOURCE> InstanceResource : register(t2);

//��Ӱ��ͼ��Դ
Texture2DArray ShadowMaps : register(t3);

//���ԴVPT��������
StructuredBuffer<PointLightVPTMat> PointLightVPTMatList : register(t4);

//GBuffer0: Albedo��Roughness
Texture2D GBuffer0 : register(t0, space1);

//GBuffer1: Normal��Metallicity
Texture2D GBuffer1 : register(t1, space1);

//GBuffer2: F0
Texture2D GBuffer2 : register(t2, space1);

//��Ȼ�����
Texture2D DepthGBuffer : register(t3, space1);

//AO GBuffer
Texture2D AOGBuffer : register(t4, space1);

//���������Դ------------------------------------------------------------------------------------------------------

//Albedo
RWTexture2D<float4> AlbedoBuffer : register(u0);

//Normal
RWTexture2D<float4> NormalBuffer : register(u1);

//Depth
RWTexture2D<float4> DepthBuffer : register(u2);

//AOBuffer Vague��
RWTexture2D<float2> AOBlurBuffer : register(u4);

//RealRT
//RWTexture2D<unorm float4> RealRT : register(u4);

//������------------------------------------------------------------------------------------------------------
SamplerState Sampler0 : register(s0);
//SamplerComparisonState ShadowSampler : register(s1);
SamplerState ShadowSampler : register(s1);
SamplerState SSAOSampler : register(s2);

//��ɫ�������������----------------------------------------------------------------------------------------------------

//������ɫ������
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

//���ߺ���----------------------------------------------------------------------------------------------------

//��ͶӰ�ռ�ת����������ռ�
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

//���ַ����ģ��----------------------------------------------------------------------------------------------------

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

//����������+���淴��
float4 ComputeReflect(float3 Normal, float4 Albedo, float Roughness, float3 InLight, float3 Intensity, float3 WldPos)
{
	Normal = normalize(Normal);

	//Intensity *= max(0, dot(-InLight, Normal));

	//������
	float4 Diffuse = float4(Intensity, 0.f) * Albedo * max(0, dot(Normal, -InLight));

	//���淴��߹�
	float3 ToCam = normalize(GlobalResource.CameraPosition - WldPos);       //�������һ��㵽�����������
	float3 bisector = normalize(ToCam - InLight);         //�������
	float4  Specular = float4(Intensity, 0.f) * Albedo * pow(max(0, dot(Normal, bisector)), Roughness);

	return Diffuse + Specular;
}

/*���ַ����ģ��*/
//float4 BlinnPhong(float3 normal, float4 albedo, float3 wldPos, float roughness)
//{
//	float4 Result = { 0,0,0,0 };
//
//	//��������
//	Result += float4(AmbParaLight.AmbientLight, 1.f) * albedo;
//
//	//ƽ�й�
//	Result += ComputeReflect(normal, albedo, roughness, normalize(AmbParaLight.ParallelLightDirection), AmbParaLight.ParallelLightIntensity, wldPos);
//
//	//������Դ����
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
//		//����Դ����
//		float Distance = distance(PointPosition, wldPos);
//
//		//�ж��Ƿ�˥��
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

//PBR����ģ��----------------------------------------------------------------------------------------------------
float G_Schlick(float roughness, float cosAngel)
{
	float Kdirect = pow((roughness + 1), 2) / 8;                     //���Դ����ϵ��

	float Schlick = cosAngel / (cosAngel * (1 - Kdirect) + Kdirect);

	return Schlick;
}

float3 CookTorrance(float roughness, float3 bisector, float3 normal, float3 f0, float3 toCamera, float3 toLight, out float3 ks)
{
	float NdotC = max(0.0001, dot(toCamera, normal));                //cos(���ߺ������н�)
	float NdotL = max(0.0001, dot(toLight, normal));                 //cos(���ߺͳ����н�)

	//����D��
	float D = pow(roughness, 2) / (PI * pow((max(0.0001f, dot(bisector, normal)) * (pow(roughness, 2) - 1) + 1), 2));

	//����F��
	float3 F = f0 + (1 - f0) * pow((1 - max(0, dot(bisector, normal))), 5);
	ks = F;

	//����G��
	float GGXInput = G_Schlick(roughness, NdotC);
	float GGXOuput = G_Schlick(roughness, NdotL);
	float G = GGXInput * GGXOuput;

	//���㾵�淴��ϵ��
	float3 Specular = D * F * G / (4 * NdotC * NdotL);

	return Specular;
}

float4 PBRLight(float3 normal, float3 albedo, float3 wldPos, PBR_ITEM pBRItem)
{
	float4 Result = { 0,0,0,1 };

	//��õ��Դ����
	uint Num;
	uint Stride;
	PointLightList.GetDimensions(Num, Stride);

	for (uint LightIndex = 0; LightIndex < Num; LightIndex++)
	{
		float3 LightPosition = PointLightList[LightIndex].Position;                //���Դλ��
		float3 ToLight = normalize(LightPosition - wldPos);           //������㵽��Դ������
		float3 ToCamera = normalize(GlobalResource.CameraPosition - wldPos);         //������㵽���������
		float3 Bisector = normalize(ToCamera + ToLight);                 //�������
		float ToLightDistance = distance(LightPosition, wldPos);      //�����Դ����
		float3 LightInputLumen = PointLightList[LightIndex].Lumen / pow(ToLightDistance, 2);        //�����ǿ��

	//���㾵�淴��ϵ����
		float3 Ks;
		float3 CookTorranceItem = CookTorrance(pBRItem.Roughness, Bisector, normal, pBRItem.F0, ToCamera, ToLight, Ks);
		float3 Specular = Ks * CookTorranceItem;

	//����������ϵ��������������ϵ��*����ϵ��/PI
		float3 Kd = (1 - Ks) * (1 - pBRItem.Metallicity);
		float3 Diffuse = Kd * albedo / PI;

	//������Ӱ����
		float ShadowFactor = 0;

		//ת����NDC�ռ�
		float4 LightSpacePos = mul(float4(wldPos, 1.f), PointLightVPTMatList[LightIndex].VPTMat);
		LightSpacePos.xyz /= LightSpacePos.w;                //͸�ӳ���

		//�����Ӱ��ͼ�ߴ粢�����ѯƫ����
		float ShadowMapWidth, ShadowMapHeight, ShadowMapElements;
		ShadowMaps.GetDimensions(ShadowMapWidth, ShadowMapHeight, ShadowMapElements);
		float dx = 1.f / ShadowMapWidth;

		//PCF
		LightSpacePos.xy -= dx;
		int CoreSize = 3;                                    //�˴�С
		
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

	//�������չ��ս��
		float3 LightOut = ShadowFactor * dot((Diffuse + Specular), LightInputLumen) * max(0,dot(normal, ToLight));

		Result += float4(LightOut, 0);
	}

	return Result;
}

//������ɫ��----------------------------------------------------------------------------------------------------
VertexOut VS_Common(VertexIn vin, uint instanceID : SV_InstanceID)
{
	VertexOut vout;

	//���ߺͶ���ת��������ռ�
	vout.Normal = mul(vin.Normal, (float3x3)InstanceResource[instanceID].WldMat);
	float4 WldPos = mul(float4(vin.LocalPos, 1.f), InstanceResource[instanceID].WldMat);
	vout.WldPos = WldPos.xyz;

	//����ռ䵽�۲�ռ�任
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
	// ʹ��һ�������θ���NDC�ռ� 
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

//������ɫ��----------------------------------------------------------------------------------------------------
float4 PS_Common(VertexOut pin) : SV_Target
{
	float4 Result;

	//��ת����
	float RunSpeed = 1;
	float2x2 transmat = { cos(RunSpeed * GlobalResource.RunTime),sin(RunSpeed * GlobalResource.RunTime),-sin(RunSpeed * GlobalResource.RunTime),cos(RunSpeed * GlobalResource.RunTime) };
	float2 uv = pin.TexC;
	uv.x -= 0.5;
	uv.y -= 0.5;
	uv = mul(uv, transmat);
	uv.x += 0.5;
	uv.y += 0.5;

	//������÷���ϵ��
	float3 Albedo = SingleAlbedoTexture.Sample(Sampler0, uv).xyz;

	//��������λ��
	float3 Normal = normalize(pin.Normal);

	//���ַ����
	//Result = BlinnPhong(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	//PBR��������;��淴��
	Result = PBRLight(Normal, Albedo, pin.WldPos, InstanceResource[pin.InstanceID].PBRItem);

	//tone mapping
	Result = Result / (1 + Result); 

	return Result;
}

PixelOutDeferredShading PS_GBuffer(VertexOut pin)
{
	PixelOutDeferredShading Result;

	//��ת����
	float RunSpeed = 1;
	float2x2 transmat = { cos(RunSpeed * GlobalResource.RunTime),sin(RunSpeed * GlobalResource.RunTime),-sin(RunSpeed * GlobalResource.RunTime),cos(RunSpeed * GlobalResource.RunTime) };
	float2 uv = pin.TexC;
	//uv.x -= 0.5;
	//uv.y -= 0.5;
	//uv = mul(uv, transmat);
	//uv.x += 0.5;
	//uv.y += 0.5;

	//������÷���ϵ��
	float3 Albedo = SingleAlbedoTexture.Sample(Sampler0, uv).xyz;

	Result.Albedo_Roughness = float4(Albedo, InstanceResource[pin.InstanceID].PBRItem.Roughness);

	Result.Normal_Metallicity = float4(pin.Normal, InstanceResource[pin.InstanceID].PBRItem.Metallicity);

	Result.F0 = float4(InstanceResource[pin.InstanceID].PBRItem.F0, 1);

	return Result;
}

float2 PS_HBAO(float4 svPos : SV_Position) : SV_Target
{
	float AO = 0;

//��Ļ����
	uint ScreenX = svPos.x;
    uint ScreenY = svPos.y;

//������
	float Depth = DepthGBuffer.Load(int3(ScreenX, ScreenY, 0), 0).x;

	if(Depth < 1)
	{
	//��������ռ�����
		//ͶӰ�ռ�����
		float4 ProjPos;
		ProjPos.x = float(ScreenX) * 2 / DeferredRenderResource.ClientWidth - 1;
		ProjPos.y = -(float(ScreenY) * 2 / DeferredRenderResource.ClientHeight - 1);
		ProjPos.z = Depth;
		ProjPos.w = 1;

		//����������ռ�����
		float AspectRatio = (float)DeferredRenderResource.ClientWidth / (float)DeferredRenderResource.ClientHeight;
		float4 ViewPos;
		ViewPos.z = DeferredRenderResource.B / (Depth - DeferredRenderResource.A);
		ViewPos.x = DeferredRenderResource.TanHalfFov * ProjPos.x * ViewPos.z * AspectRatio;
		ViewPos.y = DeferredRenderResource.TanHalfFov * ProjPos.y * ViewPos.z;
		ViewPos.w = 1;

		//��������ռ�����
		float4 WldPos = mul(ViewPos, DeferredRenderResource.InverseViewMat);

	//����
		//��������
		float3 Normal = normalize(GBuffer1.Load(int3(ScreenX, ScreenY, 0), 0).xyz);

		//��14������в���
		for (int i = 0; i < 14; i++)
		{
			//������������Ƿ��ڰ���ռ��ڣ�������ת��������ռ���
			float4 Point = 0.75 * float4(normalize(HBAOResource.NormalList[i].xyz), 0);
			if (dot(Normal, Point.xyz) < 0)
			{
				Point = -Point;
			}
			Point += WldPos;

			//������ת����NDC�ռ�
			float4 PointNDC = mul(Point, GlobalResource.VPMat);
			PointNDC.xyzw /= PointNDC.w;

			//NDC�ռ䵽[0,1]
			float2 SamplingPointUV = PointNDC.xy;
			SamplingPointUV.x += 1;
			SamplingPointUV.x /= 2;
			SamplingPointUV.y += 1;
			SamplingPointUV.y /= 2;
			SamplingPointUV.y = 1 - SamplingPointUV.y;

			//�������ֵ
			PointNDC.z = DepthGBuffer.Sample(SSAOSampler, SamplingPointUV).x;

			//ת����View�ռ�
			float4 PointViewPos = ProjToView(PointNDC, (float)DeferredRenderResource.ClientWidth, (float)DeferredRenderResource.ClientHeight,
				DeferredRenderResource.A, DeferredRenderResource.B, DeferredRenderResource.TanHalfFov);
			//float AspectRatio = (float)DeferredRenderResource.ClientWidth / (float)DeferredRenderResource.ClientHeight;
			//float4 PointViewPos;
			//PointViewPos.z = DeferredRenderResource.B / (PointNDC.z - DeferredRenderResource.A);
			//PointViewPos.x = DeferredRenderResource.TanHalfFov * PointNDC.x * PointViewPos.z * AspectRatio;
			//PointViewPos.y = DeferredRenderResource.TanHalfFov * PointNDC.y * PointViewPos.z;
			//PointViewPos.w = 1;

			Point = mul(PointViewPos, DeferredRenderResource.InverseViewMat);

			//�����ڱ�����
			float4 ToPoint = normalize(Point - WldPos);
			float Factor = max(dot(ToPoint.xyz, Normal.xyz), 0);

			//�����ڱ�ֵ
			float Distance = distance(Point, WldPos);
			float OcclusionVal = max(HBAORadius - Distance, 0);

			//���ս��
			AO += Factor * OcclusionVal;
		}
		AO /= 14;
	}

	return float2(1 - AO, 0);

}

float4 PS_DeferredShading(float4 svPos : SV_Position) : SV_Target
{
	//��Ļ����
	uint ScreenX = svPos.x;
    uint ScreenY = svPos.y;
	
	//PBR������
	PBR_ITEM PBRItem;

//����GBuffer����
	//���Albedo��Roughness
	float4 Albedo_Roughness = GBuffer0.Load(int3(ScreenX, ScreenY, 0), 0);
	PBRItem.Roughness = Albedo_Roughness.w;
	
	//���Normal��Metallicity
	float4 Normal_Metallicity = GBuffer1.Load(int3(ScreenX, ScreenY, 0), 0);
	float3 Normal = normalize(Normal_Metallicity.xyz);
	PBRItem.Metallicity = Normal_Metallicity.w;

	//���F0
	PBRItem.F0 = GBuffer2.Load(int3(ScreenX, ScreenY, 0), 0).xyz;

	//������
	float Depth = DepthGBuffer.Load(int3(ScreenX, ScreenY, 0), 0).x;

	//���AOֵ
	float AO = AOGBuffer.Load(int3(ScreenX, ScreenY, 0), 0).x;

//��������ռ�����
	//ͶӰ�ռ�����
	float4 ProjPos;
	ProjPos.x = float(ScreenX) * 2 / DeferredRenderResource.ClientWidth - 1;
	ProjPos.y = -(float(ScreenY) * 2 / DeferredRenderResource.ClientHeight - 1);
	ProjPos.z = Depth;
	ProjPos.w = 1;

	//����������ռ�����
	float4 ViewPos = ProjToView(ProjPos, (float)DeferredRenderResource.ClientWidth, (float)DeferredRenderResource.ClientHeight,
		DeferredRenderResource.A, DeferredRenderResource.B, DeferredRenderResource.TanHalfFov);
	//float AspectRatio = (float)DeferredRenderResource.ClientWidth / (float)DeferredRenderResource.ClientHeight;
	//float4 ViewPos;
	//ViewPos.z = DeferredRenderResource.B / (Depth - DeferredRenderResource.A);
	//ViewPos.x = DeferredRenderResource.TanHalfFov * ProjPos.x * ViewPos.z * AspectRatio;
	//ViewPos.y = DeferredRenderResource.TanHalfFov * ProjPos.y * ViewPos.z;
	//ViewPos.w = 1;

	//��������ռ�����
	float4 WldPos = mul(ViewPos, DeferredRenderResource.InverseViewMat);

//�������
	float4 PBR = PBRLight(Normal, Albedo_Roughness.xyz, WldPos.xyz, PBRItem);       //PBR����
	float4 HBAO = float4(AO * AmbParaLight.AmbientLight * Albedo_Roughness.xyz, 1);            //�������ڱ�

	//tone mapping
	float4 Color = float4(ACESToneMapping((PBR + HBAO).xyz), 1);

	return Color;
	
}

//������ɫ��----------------------------------------------------------------------------------------------------

//�����ڴ棬�������������
groupshared float AORowBuffer[1920 * 4];

[numthreads(512,1,1)]
void CS_AOBufferBlurRow(int3 groupID : SV_GroupID, int3 gThreadID : SV_GroupThreadID)
{
	uint Width = AOBufferBlurResource.Width;                 //�п��
	uint RowIndex = groupID.x;                                //������
	uint ID = gThreadID.x;                                    //����ID

	//����Ҫѭ��������
	uint LoopNum = Width / 512 + 1;

	//���������ݲ����������ڴ���
	for (uint i = 0; i < LoopNum; i++)
	{
		uint Index = 512 * i + ID;                             //������������
		if (Index < Width)
		{
			AORowBuffer[Index] = AOBlurBuffer[int2(Index, RowIndex)].x;
		}
		else
			break;
	}
	
	//�����߳�ͬ��
	GroupMemoryBarrierWithGroupSync();

	//ģ��
	for (uint k = 0; k < LoopNum; k++)
	{
		uint Index = 512 * k + ID;                             //������������

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

//�����ڴ棬�������������
groupshared float AOColBuffer[1080 * 4];

[numthreads(512, 1, 1)]
void CS_AOBufferBlurCol(int3 groupID : SV_GroupID, int3 gThreadID : SV_GroupThreadID)
{
	uint Height = AOBufferBlurResource.Height;                 //�и߶�
	uint ColIndex = groupID.x;                                //������
	uint ID = gThreadID.x;                                    //����ID

	//����Ҫѭ��������
	uint LoopNum = Height / 512 + 1;

	//���������ݲ����������ڴ���
	for (uint i = 0; i < LoopNum; i++)
	{
		uint Index = 512 * i + ID;                             //������������
		if (Index < Height)
		{
			AOColBuffer[Index] = AOBlurBuffer[int2(ColIndex, Index)].x;
		}
		else
			break;
	}

	//�����߳�ͬ��
	GroupMemoryBarrierWithGroupSync();

	//ģ��
	for (uint k = 0; k < LoopNum; k++)
	{
		uint Index = 512 * k + ID;                             //������������

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