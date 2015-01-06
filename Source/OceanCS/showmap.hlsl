//-----------------------------------------------------------------------------
//gaidong
//-----------------------------------------------------------------------------
Texture2D gShadowMap: register(t0);
matrix g_mWorldViewProj:register(b0);
// A small sky cubemap for reflection
SamplerState g_samplerCube		: register(s0);

struct VS_IN
{
	float3 posL : POSITION;
	float2 texC : TEXCOORD;
};

struct VS_OUT
{
	float4 posH : SV_POSITION;
	float2 texC : TEXCOORD;
};
 
VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;

//	vOut.posH = float4(vIn.posL.x,vIn.posL.y,vIn.posL.z, 1.0f);
	vOut.posH = mul(float4(vIn.posL.x,vIn.posL.z,vIn.posL.y, 1.0f), g_mWorldViewProj);
	vOut.texC = vIn.texC;
	
	return vOut;
}

float4 PS(VS_OUT pIn) : SV_Target
{
	float3 rgb = gShadowMap.Sample(g_samplerCube, pIn.texC).rgb;
	
	// draw as grayscale
	return float4(rgb,1);
}