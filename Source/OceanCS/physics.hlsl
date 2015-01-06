Texture2D	gPrevStepMap;
Texture2D	gCurrStepMap;
SamplerState g_samplergStepMap
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
};
cbuffer cbPerFrame
{
float gK1;
float gK2;
float gK3;
float gTexelSize;
};
RasterizerState NoCull
{
    CullMode = None;
};
struct VS_QUAD_OUTPUT
{
    float4 Position		: SV_POSITION;	// vertex position
    float2 TexCoord		: TEXCOORD0;	// vertex texture coords 
};

struct VS_OUT
{
	float4 posH : SV_POSITION;
	float2 oUV : TEXCOORD0;
};

VS_QUAD_OUTPUT QuadVS(float4 vPos : POSITION)
{
	VS_QUAD_OUTPUT Output;

	Output.Position = vPos;
	Output.TexCoord.x = 0.5f + vPos.x * 0.5f;
	Output.TexCoord.y = 0.5f - vPos.y * 0.5f;

	return Output;
}
float4 PhysicsPS(VS_OUT vIn) : SV_Target
{		
	// Sample grid points from previous and current time steps.
		float c0 = gPrevStepMap.SampleLevel(g_samplergStepMap, vIn.oUV, 0).r;
		float c1 = gPrevStepMap.SampleLevel(g_samplergStepMap, vIn.oUV, 0).g;
		float t = gPrevStepMap.SampleLevel(g_samplergStepMap, float2(vIn.oUV.x, vIn.oUV.y - gTexelSize), 0).g;//(i,j-1)
		float b = gPrevStepMap.SampleLevel(g_samplergStepMap, float2(vIn.oUV.x, vIn.oUV.y + gTexelSize), 0).g;//(i,j+1)
		float l = gPrevStepMap.SampleLevel(g_samplergStepMap, float2(vIn.oUV.x - gTexelSize, vIn.oUV.y ), 0).g;//(i-1,j)
		float r = gPrevStepMap.SampleLevel(g_samplergStepMap, float2(vIn.oUV.x + gTexelSize, vIn.oUV.y ), 0).g;//(i+1,j)
	// Return next solution at this grid point.
		float c2 = gK1*c0 + gK2*c1 + gK3*(t+b+l+r);
		
		return float4( c1,c2,c0,0.0f);
		//return float4( c2,0,0,0.0f);
}
/*float4 PhysicsPS(VS_OUT vIn) : SV_Target
{		
	// Sample grid points from previous and current time steps.
		float c0 = gPrevStepMap.SampleLevel(g_samplergStepMap, vIn.oUV, 0).r;
		float c1 = gCurrStepMap.SampleLevel(g_samplergStepMap, vIn.oUV, 0).r;
		float t = gCurrStepMap.SampleLevel(g_samplergStepMap, float2(vIn.oUV.x, vIn.oUV.y - gTexelSize), 0).r;//(i,j-1)
		float b = gCurrStepMap.SampleLevel(g_samplergStepMap, float2(vIn.oUV.x, vIn.oUV.y + gTexelSize), 0).r;//(i,j+1)
		float l = gCurrStepMap.SampleLevel(g_samplergStepMap, float2(vIn.oUV.x - gTexelSize, vIn.oUV.y ), 0).r;//(i-1,j)
		float r = gCurrStepMap.SampleLevel(g_samplergStepMap, float2(vIn.oUV.x + gTexelSize, vIn.oUV.y ), 0).r;//(i+1,j)
	// Return next solution at this grid point.
		float c2 = gK1*c0 + gK2*c1 + gK3*(t+b+l+r);
		
		//return float4( c1,c2,c0,0.0f);
		return float4( gK1*c0 + gK2*c1 + gK3*(t+b+l+r),0,0,0.0f);
}*/
float4 CopyPS(VS_OUT vIn) : SV_Target
{
		float c0 = gCurrStepMap.SampleLevel(g_samplergStepMap, vIn.oUV, 0).r;
		float c1 = gCurrStepMap.SampleLevel(g_samplergStepMap, vIn.oUV, 0).g;
		float c2 = gCurrStepMap.SampleLevel(g_samplergStepMap, vIn.oUV, 0).b;
		return float4( c0,c1,c2,0.0f);
}
technique11 PhysicsTech
{
    pass P0
    {
		SetVertexShader(   CompileShader( vs_5_0, QuadVS() ) );
		SetGeometryShader( NULL  );
        SetPixelShader(    CompileShader( ps_5_0, PhysicsPS() ) );
        
    //    SetBlendState(AdditiveBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
     //   SetRasterizerState(NoCull);
    //    SetDepthStencilState(LessEqualDSS, 0); //去掉湖面与天空交界就平坦了
    }
}
technique11 CopyTech
{
    pass P0
    {
		SetVertexShader(   CompileShader( vs_5_0, QuadVS() ) );
		SetGeometryShader( NULL  );
        SetPixelShader(    CompileShader( ps_5_0, CopyPS() ) );
        
    //    SetBlendState(AdditiveBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
    //    SetRasterizerState(NoCull);
    //    SetDepthStencilState(LessEqualDSS, 0); //去掉湖面与天空交界就平坦了
    }
}
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
struct IN
{
	float3 posL : POSITION;
};
struct OUT
{
	float4 posH : SV_POSITION;
	//float2 oUV : TEXCOORD0;
	float height : TEXCOORD0;
};
OUT DisturbVS(IN vIn)
{
	OUT vOut;
	vOut.posH  = float4(vIn.posL.x,vIn.posL.y,0,1.0f);
	vOut.height = vIn.posL.z;
	return vOut;
}
float4 DisturbPS(OUT vOut):SV_Target
{
	return float4(vOut.height,vOut.height,0,1);
}

technique11 DisturbTech
{
    pass P0
    {
		SetVertexShader(   CompileShader( vs_5_0, DisturbVS() ) );
		SetGeometryShader( NULL  );
        SetPixelShader(    CompileShader( ps_5_0, DisturbPS() ) );
        
    //    SetBlendState(AdditiveBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
    //    SetRasterizerState(NoCull);
    //    SetDepthStencilState(LessEqualDSS, 0); //去掉湖面与天空交界就平坦了
    }
}