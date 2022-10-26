struct OBJECT_SOURCE_0
{
	//�ֲ���������ռ�任����
	float4x4 MVPMat;
	//�ֲ�������任����
	float4x4 WldMat;

	//����ֲڶ�
	float Roughness;
};

//ȫ��֡��Դ
struct GLOBAL_RESOURCE
{
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

	//��ǿ
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

//����������--------------------------------------------------------------------------------------------------

//��ʵ������Դ
ConstantBuffer<OBJECT_SOURCE_0> SingleObjectSource : register(b0);

//ȫ����Ϣ֡��Դ
ConstantBuffer<GLOBAL_RESOURCE> GlobalResource : register(b1);

//������Ϣ֡��Դ
ConstantBuffer<LIGHT_RESOURCE> AmbParaLight : register(b2);

//
ConstantBuffer<SHADOWMAPRESOURCE> ShadowMapResource : register(b0, space1);

//��ɫ����Դ--------------------------------------------------------------------------------------------------

//�����Դ����
StructuredBuffer<POINT_LIGHT> PointLightList : register(t0);

//����ɫ��ͼ
Texture2D SingleAlbedoTexture : register(t1);

//ʵ������֡��Դ
StructuredBuffer<OBJECT_SOURCE_0> InstanceResource : register(t2);

//��Ӱ��ͼ��Դ
Texture2DArray ShadowMaps : register(t3);

//���ԴVPT��������
StructuredBuffer<PointLightVPTMat> PointLightVPTMatList : register(t4);

//������------------------------------------------------------------------------------------------------------
SamplerState Sampler0 : register(s0);
//SamplerComparisonState ShadowSampler : register(s1);
SamplerState ShadowSampler : register(s1);

//����----------------------------------------------------------------------------------------------------

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
	//4����Ե��λ���
	float EdgeTess[4] : SV_TessFactor;

	//�ڲ�ϸ������������
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

//��������----------------------------------------------------------------------------------------------------

//���㷴�䣨�����䡢���淴�䣩
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

//���㻷�����ա�ƽ�й���
float4 CalcuAmbParaLight(float3 normal, float4 albedo, float3 wldPos, float roughness)
{
	//��������
	float4 AmbientLightOut = float4(AmbParaLight.AmbientLight,1.f) * albedo;

	//ƽ�й���
	float4 ParallelLightOut = ComputeReflect(normal, albedo, roughness, normalize(AmbParaLight.ParallelLightDirection), AmbParaLight.ParallelLightIntensity, wldPos);

	return AmbientLightOut + ParallelLightOut;
}

//������Դ����
float4 CalcuPointLight(float3 normal, float4 albedo, float3 wldPos, float roughness)
{
	float4 PointLightOut = { 0,0,0,0 };

	uint Num;
	uint Stride;
	PointLightList.GetDimensions(Num, Stride);

	//���Դ��Y����ת
	float3x3 RotateMat = 
	{cos(GlobalResource.RunTime),0,-sin(GlobalResource.RunTime),
		0,1,0,
	sin(GlobalResource.RunTime),0,cos(GlobalResource.RunTime)};

	for (uint i = 0; i < Num; i++)
	{
		float3 PointPosition = mul(PointLightList[i].Position, RotateMat);

		float3 Direction = normalize(wldPos - PointPosition);

		float3 Intensity = PointLightList[i].Intensity;

		//����Դ����
		float Distance = distance(PointPosition, wldPos);

		//�ж��Ƿ�˥��
		if (Distance > PointLightList[i].DecayDistance)
		{
			Intensity /= pow(Distance, 2);
		}

		PointLightOut += ComputeReflect(normal, albedo, roughness, Direction, Intensity, wldPos);
	}

	return PointLightOut;
}

//��ɫ��------------------------------------------------------------------------------------------------------
VertexOut_PC VS_0(VertexIn_PC vin)
{
	VertexOut_PC vout;

	//����ռ䵽�۲�ռ�任
	vout.ProjPos = mul(float4(vin.LocalPos, 1.0f), SingleObjectSource.MVPMat);

	//��ɫ����
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

	//����ռ䵽�۲�ռ�任
	vout.ProjPos = mul(float4(vin.LocalPos, 1.0f), SingleObjectSource.MVPMat);

	//���ߺͶ���ת��������ռ�
	vout.Normal = mul(vin.Normal, (float3x3)SingleObjectSource.WldMat);
	vout.WldPos = mul(vin.LocalPos, (float3x3)SingleObjectSource.WldMat);

	vout.TexC = vin.TexC;

	return vout;
}

float4 PS_1(VertexOut_PPNT pin) : SV_Target
{
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
	float4 Albedo = SingleAlbedoTexture.Sample(Sampler0, uv);

//��������Ԥ����
	float3 Normal = normalize(pin.Normal);

	float4 Result;

	Result = CalcuAmbParaLight(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	Result += CalcuPointLight(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);
	
	return Result;
}

//����ϸ��1----------------------------------------------------------------------------------------------

VertexIn_PC VS_2(VertexIn_PC vin)
{
	return vin;
}

//���������ɫ��
PatchTess ConstantHS(InputPatch<VertexOut_PC_1, 4>patch,    //InputPatch��ʾ����Ƭ���Ƶ���Ϊ���룬VertexOut_0���Զ���Ķ�����ɫ��������Ƶ㣬4�ǿ��Զ��������
	uint patchID : SV_PrimitiveID)                       //ϵͳͨ��SV_PrimitiveID�ṩ��Ƭ��IDֵ
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

//���Ƶ���ɫ��
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

//����ɫ��
[domain("quad")]
DSOut_P_C DS_0(PatchTess patchTess, float2 uv : SV_DomainLocation,
	const OutputPatch<HSOut_P_C, 4> quad)
{
	DSOut_P_C DSOut;

//˫���Բ�ֵ
	//λ��
	float3 v1 = lerp(quad[2].LocalPos, quad[3].LocalPos, uv.x);
	float3 v2 = lerp(quad[1].LocalPos, quad[0].LocalPos, uv.x);
	float3 p = lerp(v1, v2, uv.y);

	p.y += sin(8*(GlobalResource.RunTime + uv.y))/12;
	p.y += sin(8 * (GlobalResource.RunTime + uv.x)) / 12;

	//��ɫ
	float4 c1 = lerp(quad[2].Color, quad[3].Color, uv.x);
	float4 c2 = lerp(quad[1].Color, quad[0].Color, uv.x);
	float4 c = lerp(c1, c2, uv.y);

	DSOut.ProjPos = mul(float4(p, 1.0f), SingleObjectSource.MVPMat);
	DSOut.Color = c;

	return DSOut;
}

//����ϸ��2----------------------------------------------------------------------------------------------

VertexIn_PNT VS_3(VertexIn_PNT vin)
{
	VertexIn_PNT Out;
	Out.LocalPos = vin.LocalPos;
	Out.Normal = vin.Normal;
	Out.TexC = vin.TexC;
	return vin;
}

PatchTess ConstantHS_2(InputPatch<VertexIn_PNT, 4>patch,    //InputPatch��ʾ����Ƭ���Ƶ���Ϊ���룬VertexOut_0���Զ���Ķ�����ɫ��������Ƶ㣬4�ǿ��Զ��������
	uint patchID : SV_PrimitiveID)                       //ϵͳͨ��SV_PrimitiveID�ṩ��Ƭ��IDֵ
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

//���Ƶ���ɫ��
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

//����ɫ��
[domain("quad")]
DSOut_PPNT DS_1(PatchTess patchTess, float2 uv : SV_DomainLocation,
	const OutputPatch<VertexIn_PNT, 4> quad)
{
	DSOut_PPNT DSOut;

	//λ��
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

//ˮ----------------------------------------------------------------------------------------------
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

PatchTess ConstantHS_Water(InputPatch<VertexOut_Water, 4>patch,    //InputPatch��ʾ����Ƭ���Ƶ���Ϊ���룬VertexOut_0���Զ���Ķ�����ɫ��������Ƶ㣬4�ǿ��Զ��������
	uint patchID : SV_PrimitiveID)                       //ϵͳͨ��SV_PrimitiveID�ṩ��Ƭ��IDֵ
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

//���Ƶ���ɫ��
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

//����ɫ��
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

	//λ��
	float3 v1 = lerp(quad[0].LocalPos, quad[1].LocalPos, uv.x);
	float3 v2 = lerp(quad[2].LocalPos, quad[3].LocalPos, uv.x);
	float3 p = lerp(v1, v2, uv.y);

	p.y += sin(2 * (GlobalResource.RunTime + uv.y * 2 * 3.141592653)) / 12;
	p.y += sin(2 * (GlobalResource.RunTime + uv.x * 2 * 3.141592653)) / 12;

	//���λ�á���ɫ
	DSOut.ProjPos = mul(float4(p, 1.0f), InstanceResource[quad[0].InstanceID].MVPMat);
	DSOut.WldPos = mul(p, (float3x3)InstanceResource[quad[0].InstanceID].WldMat);
	DSOut.Color = quad[0].Color;

	//�������
	float3 normal;
	normal.x = -4 * 3.141592653 * cos(2 * (GlobalResource.RunTime + uv.y * 2 * 3.141592653));
	normal.y = 12;
	normal.z = -4 * 3.141592653 * cos(2 * (GlobalResource.RunTime + uv.x * 2 * 3.141592653));

	DSOut.Normal = mul(normalize(normal), (float3x3)InstanceResource[quad[0].InstanceID].WldMat);

	return DSOut;
}

float4 PS_Water(DSOut_PPCN pin) : SV_Target
{
	//����ϵ��
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
	//������÷���ϵ��
	float4 Albedo = SingleAlbedoTexture.Sample(Sampler0, pin.TexC);

	//��������Ԥ����
	float3 Normal = normalize(pin.Normal);

	float4 Result;

	Result = CalcuAmbParaLight(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	Result += CalcuPointLight(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	return Result;
}

//������Ӱ��ͼ-------------------------------------------------------------------------------------------------------------------------------------------------
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

	//λ��
	float3 v1 = lerp(quad[0].LocalPos, quad[1].LocalPos, uv.x);
	float3 v2 = lerp(quad[2].LocalPos, quad[3].LocalPos, uv.x);
	float3 p = lerp(v1, v2, uv.y);
	float3 position = normalize(p) * length(quad[0].LocalPos);

	//���λ��
	float4 pos = mul(float4(position, 1.0f), SingleObjectSource.WldMat);
	DSOut.Position = mul(pos, ShadowMapResource.VPMat);

	return DSOut;
}

void PS_ShadowMap(VSSHADOWOUT pin)
{

}

//��Ӱ��Ⱦ-------------------------------------------------------------------------------------------------------------------------------------------------
float4 PointLight_Shadow(float3 normal, float4 albedo, float4 wldPos, float roughness)
{
	uint PointLightNum;
	uint Stride;
	PointLightList.GetDimensions(PointLightNum, Stride);

	float4 LightOut = { 0,0,0,0 };

	for (uint LightIndex = 0; LightIndex < 1; LightIndex++)
	{
		//ת������Դ������ռ�
		float4 PointSpacePos = mul(wldPos, PointLightVPTMatList[LightIndex].VPTMat);
		PointSpacePos.xyz /= PointSpacePos.w;                                                   //͸�ӳ���

		//PCF
		float dx = 1.f / (float)4096;                   //��Ӱ��ͼÿ�����ؾ���

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
		
		//���Դ��Y����ת
		//float3x3 RotateMat =
		//{ cos(GlobalResource.RunTime),0,-sin(GlobalResource.RunTime),
		//	0,1,0,
		//sin(GlobalResource.RunTime),0,cos(GlobalResource.RunTime) };

		//float3 PointPosition = mul(PointLightList[LightIndex].Position, RotateMat);
		float3 PointPosition = PointLightList[LightIndex].Position;

		//�㵽���Դ����
		float3 Direction = normalize(wldPos.xyz - PointPosition);

		float3 Intensity = PointLightList[LightIndex].Intensity;

		//����Դ����
		float Distance = distance(PointPosition, wldPos.xyz);

		//�ж��Ƿ�˥��
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

	//����ռ䵽�۲�ռ�任
	vout.ProjPos = mul(float4(vin.LocalPos, 1.0f), SingleObjectSource.MVPMat);

	//���ߺͶ���ת��������ռ�
	vout.Normal = mul(vin.Normal, (float3x3)SingleObjectSource.WldMat);
	vout.WldPos = mul(float4(vin.LocalPos, 1.0f), SingleObjectSource.WldMat);

	vout.TexC = vin.TexC;

	return vout;
}

float4 PS_Shadow(VertexOut_PPNTS pin) : SV_Target
{
	//��ת����
	//float RunSpeed = 1;
	//float2x2 transmat = { cos(RunSpeed * GlobalResource.RunTime),sin(RunSpeed * GlobalResource.RunTime),-sin(RunSpeed * GlobalResource.RunTime),cos(RunSpeed * GlobalResource.RunTime) };
	//float2 uv = pin.TexC;
	//uv.x -= 0.5;
	//uv.y -= 0.5;
	//uv = mul(uv, transmat);
	//uv.x += 0.5;
	//uv.y += 0.5;

	//������÷���ϵ��
	float4 Albedo = SingleAlbedoTexture.Sample(Sampler0, pin.TexC);

	//��������Ԥ����
	float3 Normal = normalize(pin.Normal);

	float4 Result;

	//Result = CalcuAmbParaLight(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	Result = PointLight_Shadow(Normal, Albedo, pin.WldPos, SingleObjectSource.Roughness);

	return Result;
}







