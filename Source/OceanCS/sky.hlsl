
#define PI 3.14159265
float4x4	WorldViewProj;
float3 gEyePosW;
float4x4 gWorld;
Texture2D Ftable;
cbuffer cbImmutable
{
    float4 pointLightColor = float4(1.0,1.0,1.0,1.0);
    float3 g_PointLightPos = float3(  -3.7,5.8,3.15);     
	//float3 g_PointLightPos = float3(  262,0,425);     
    float3 g_PointLightPos2 = float3(-3.7,5.8,3.15);
	float3 g_beta = float3(0.04,0.04,0.04);
	float g_PointLightIntensity = 0.2;
	float dirLightIntensity = 2;
    float g_fXOffset = 0; 
    float g_fXScale = 0.6366198; //1/(PI/2)
    float g_fYOffset = 0;        
    float g_fYScale = 0.5;
    
    float g_20XOffset = 0; 
    float g_20XScale = 0.6366198; //1/(PI/2) 
    float g_20YOffset = 0;
    float g_20YScale = 0.5;

    float g_diffXOffset = 0; 
    float g_diffXScale = 0.5;
    float g_diffYOffset = 0;        
    float g_diffYScale = 0.3183099;  //1/PI   
}
SamplerState samLinearClamp
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};
struct IN
{
	float3 posL : POSITION;
	float4 color: COLOR;
};
struct OUT
{
	float4 posH : SV_POSITION;
	float4 color: COLOR;
	float4 posW : POSITION;
};
OUT SkyVS(IN vIn)
{
	OUT vOut;

	vOut.posH  = mul(float4(vIn.posL,1.0f),WorldViewProj);
	vOut.color = vIn.color;
	vOut.posW = mul(float4(vIn.posL,1.0f),gWorld);
	return vOut;
}
float4 SkyPS(OUT In):SV_Target
{
	return float4(In.color);
}
RasterizerState NoCull
{
    CullMode = None;
};

DepthStencilState LessEqualDSS
{
	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
    DepthFunc = LESS_EQUAL;
};
cbuffer cbPerFrame
{
	float4x4 gWVP;
	float4 eyePos;
	float4 sunDirection;
	float4 betaRayleighMie;
	float4 reflectance;
	float4 HGg;
	float4 betaDashRay;
	float4 betaDashMie;
	float4 oneOverBetaRM;																																																																																																																																																																																																																																																																																																																																																																																																																								
	float4 sunColourIntensity;
	float4 termMultipliers;
};
cbuffer  cbFixed
{
	float4 constants = {1.0f, 1.4426950f,0.5f, 0.0f};
}
// Nonnumeric values cannot be added to a cbuffer.
TextureCube gCubeMap;

SamplerState gTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

struct VS_IN
{
	float3 posL : POSITION;
};

struct VS_OUT
{
	float4 posH : SV_POSITION;
	float4 inscattering : COLOR0;
	float4 extinction : COLOR1;
    float3 texC : TEXCOORD;
};
 
VS_OUT VS(IN vIn)
{
	VS_OUT vOut;
	
	// set z = w so that z/w = 1 (i.e., skydome always on far plane).
	vOut.posH = mul(float4(vIn.posL, 1.0f), gWVP).xyww;
	
	// use local vertex position as cubemap lookup vector.
	vOut.texC = vIn.posL;
	
	float4 worldPos = float4(vIn.posL, 1.0f);
	float3 viewDir = normalize(eyePos.xyz - worldPos.xyz); 	
	float cosTheta = dot(viewDir, sunDirection.xyz);	
	float thetaRay = (cosTheta * cosTheta) + 1.0f;		
	float dist = distance(eyePos, worldPos);
	float3 extinction = exp(-betaRayleighMie.xyz * dist * constants.y);		
	float3 totalExtinction = extinction * reflectance.xyz;		
	float thetaMie = HGg.x / pow(abs((HGg.y + HGg.z * cosTheta)), 1.5f);		
	float3 inscattering = ((betaDashRay.xyz * thetaRay) + (betaDashMie.xyz * thetaMie)) * (1.0f - extinction) * oneOverBetaRM.xyz;		
	inscattering *= termMultipliers.y;	
	inscattering *= sunColourIntensity.xyz;	
	inscattering *= sunColourIntensity.w;		
	totalExtinction *= sunColourIntensity.xyz;	
	totalExtinction *= sunColourIntensity.w;	 	
	vOut.inscattering = float4(inscattering, 1.0f); 	
	vOut.extinction = float4(totalExtinction, 1.0f);



	return vOut;
}

float4 PS(VS_OUT pIn) : SV_Target
{
	return pIn.inscattering;
}



technique11 SkyScatterTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS() ) );
        
        SetRasterizerState(NoCull);
        SetDepthStencilState(LessEqualDSS, 0);
    }
}
float3 calculateAirLightPointLight(float Dvp,float Dsv,float3 S,float3 V)
{
    float gamma = acos(dot(S, V));
    gamma = clamp(gamma,0.01,PI-0.01);
    float sinGamma = sin(gamma);
    float cosGamma = cos(gamma);
    float u = g_beta.x * Dsv * sinGamma;
    float v1 = 0.25*PI+0.5*atan((Dvp-Dsv*cosGamma)/(Dsv*sinGamma)); 
    float v2 = 0.5*gamma;
            
    float lightIntensity = g_PointLightIntensity * 40;        
            
    float f1= Ftable.SampleLevel(samLinearClamp, float2((v1-g_fXOffset)*g_fXScale, (u-g_fYOffset)*g_fYScale), 0).r;
    float f2= Ftable.SampleLevel(samLinearClamp, float2((v2-g_fXOffset)*g_fXScale, (u-g_fYOffset)*g_fYScale), 0).r;
    float airlight = (g_beta.x*lightIntensity*exp(-g_beta.x*Dsv*cosGamma))/(2*PI*Dsv*sinGamma)*(f1-f2);
    
    return airlight.xxx;
}
float3 phaseFunctionSchlick(float cosTheta)
{
   float k = -0.2; 
   float p = (1-k*k)/(pow(1+k*cosTheta,2) );
   return float3(p,p,p);
}
float4 SkyRenderPS(OUT pIn) : SV_Target
{
	
	float4 outputColor = float4(0,0,0,0);    
    float4 sceneColor =  float4(0,0,0,0);

    float3 viewVec = pIn.posW.xyz - gEyePosW;
   // float Dvp = length(viewVec);//视点到天空的距离
	float Dvp = 50;
    float3 V =  normalize(viewVec);//视点到天空的方向
    float3 exDir = float3( exp(-g_beta.x*Dvp),  exp(-g_beta.y*Dvp),  exp(-g_beta.z*Dvp)  );

    // air light
	float3 lightEyeVec = g_PointLightPos - gEyePosW;//光源到视点的
	float Dsv = length(lightEyeVec);//光源到视点的距离
	float3 S = normalize(lightEyeVec);//光源到视点的方向
    float3 airlightColor = calculateAirLightPointLight(Dvp,Dsv,S,V);

	//directional light
    //float3 SDir = normalize( g_eyePos - g_lightPos);
    float cosGammaDir = dot(-S, -V);
    float3 diffuseDirLight = dirLightIntensity*exDir;
    float3 dirAirLight = phaseFunctionSchlick(cosGammaDir)*  
                         dirLightIntensity*float3(1-exDir.x,1-exDir.y,1-exDir.z);
    outputColor = float4( airlightColor.xyz , 1); 

    
    return outputColor;
}
technique11 SkyTech
{
    pass P0
    {
		SetVertexShader(   CompileShader( vs_5_0, SkyVS() ) );
		SetGeometryShader( NULL  );
        SetPixelShader(    CompileShader( ps_5_0, SkyPS() ) );
        
    //    SetBlendState(AdditiveBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        SetRasterizerState(NoCull);
    //    SetDepthStencilState(LessEqualDSS, 0); //去掉湖面与天空交界就平坦了
    }
}