//=============================================================================
// Fire.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Fire particle system.  Particles are emitted directly in world space.
//=============================================================================


//***********************************************
// GLOBALS                                      *
//***********************************************

cbuffer cbPerFrame
{
	float3 gEyePosW;
	
	// for when the emit position/direction is varying
	float3 gEmitPosW;
	//float3 gEmitDirW;
	
	float gGameTime;
	float gTimeStep;
	float4x4 gViewProj; 
	//float3 vRandom;
	float3 g_TotalVel ;
	float g_FrameRate;

};

cbuffer cbFixed
{
	// Net constant acceleration used to accerlate the particles.
	//float3 gAccelW = {0.0f, 17.8f, 0.0f};
	float3 gAccelW = {-10.0f, -119.8f, 20.0f};
	// Texture coordinates used to stretch texture over quad 
	// when we expand point particle into a quad.
	float2 gQuadTexC[4] = 
	{
		float2(0.0f, 1.0f),
		float2(1.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 0.0f)
	};
};
 
// Array of textures for texturing the particles.
//Texture2DArray gTexArray;

// Random texture used to generate random numbers in shaders.
Texture1D gRandomTex;
 
SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
};
 
DepthStencilState DisableDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
};

DepthStencilState NoDepthWrites
{
    DepthEnable = TRUE;
    DepthWriteMask = ZERO;
};

BlendState AdditiveBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = ONE;
    BlendOp = ADD;
    SrcBlendAlpha = ZERO;
    DestBlendAlpha = ZERO;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

 
//***********************************************
// STREAM-OUT TECH                              *
//***********************************************

#define PT_EMITTER 0
#define PT_FLARE 1
 
struct Particle
{
	float3 InitialPosW : POSITION;
	float3 InitialVelW : VELOCITY;
	float2 SizeW       : SIZE;
	float3 Age         : AGE;
	uint Type          : TYPE;
	
};
  

  float3 RandVec3(float offset)
{
	// Use game time plus offset to sample random texture.
	float u = (gGameTime + offset);
	
	// coordinates in [-1,1]
	float3 v = gRandomTex.SampleLevel(samLinear, u, 0).xyz;
	
	return v;
}
[maxvertexcount(1)]
void StreamOutGS( point Particle gin[1], inout PointStream<Particle> SpriteStream)
{	
	gin[0].InitialPosW += float3(2,-5,2)/g_FrameRate + g_TotalVel;//未生效
	//gin[0].InitialPosW.x += 2*gin[0].Age.x/g_FrameRate;
	//gin[0].InitialPosW.y += -62*gin[0].Age.y/g_FrameRate;
	//in[0].InitialPosW.z += 2*gin[0].Age.z/g_FrameRate;
	
	if(gin[0].InitialPosW.y <=gEyePosW.y+35*gin[0].InitialVelW.y-14)
	{                                                                                                                                                                                                          
	     gin[0].InitialPosW = gEyePosW.xyz + 35*gin[0].InitialVelW ;	
	}
		SpriteStream.Append(gin[0]);	
}


Particle StreamOutVS(Particle vin)
{
	//vin.InitialPosW += /*gin[0].Age*g_FrameRate*/float3(2,-2.2,2)/g_FrameRate;
	return vin;
}
GeometryShader gsStreamOut = ConstructGSWithSO( 
	CompileShader( gs_5_0, StreamOutGS() ), 
	"POSITION.xyz; VELOCITY.xyz; SIZE.xy; AGE.xyz; TYPE.x" );
	
technique11 StreamOutTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, StreamOutVS() ) );
        SetGeometryShader( gsStreamOut );
        SetPixelShader(NULL);
        
        // we must also disable the depth buffer for stream-out only
        SetDepthStencilState( DisableDepth, 0 );
    }
}

//***********************************************
// DRAW TECH                                    *
//***********************************************



struct GeoOut
{
	float4 PosH  : SV_Position;
	float2 Tex   : TEXTURE0;
};

void GenRainSpriteVertices(float3 worldPos, float3 velVec, float3 eyePos, out float3 outPos[4])
{
    float height = 1.0/2.0;
    float width = height/10.0;

    velVec = normalize(velVec);
    float3 eyeVec = eyePos - worldPos;
    float3 eyeOnVelVecPlane = eyePos - ((dot(eyeVec, velVec)) * velVec);
    float3 projectedEyeVec = eyeOnVelVecPlane - worldPos;
    float3 sideVec = normalize(cross(projectedEyeVec, velVec));
    
    outPos[0] =  worldPos - (sideVec * 0.5*width);
    outPos[1] = outPos[0] + (sideVec * width);
    outPos[2] = outPos[0] + (velVec * height);
    outPos[3] = outPos[2] + (sideVec * width );
}
[maxvertexcount(4)]
void GSRenderRainCheap(point Particle input[1], inout TriangleStream<GeoOut> SpriteStream)
{
   
    GeoOut output = (GeoOut)0;
    float3 pos[4];
    GenRainSpriteVertices(input[0].InitialPosW, float3(2,-5,2)/g_FrameRate+ g_TotalVel, gEyePosW, pos);
     
    output.PosH = mul( float4(pos[0].x,pos[0].z,pos[0].y,1.0), gViewProj );
    output.Tex = float2(0,1);
    SpriteStream.Append(output);
        
    output.PosH = mul( float4(pos[1].x,pos[1].z,pos[1].y,1.0), gViewProj );
    output.Tex = float2(1,1);
    SpriteStream.Append(output);
        
    output.PosH = mul( float4(pos[2].x,pos[2].z,pos[2].y,1.0), gViewProj );
    output.Tex = float2(0,0);
    SpriteStream.Append(output);
                
    output.PosH = mul( float4(pos[3].x,pos[3].z,pos[3].y,1.0), gViewProj );
    output.Tex = float2(1,0);
    SpriteStream.Append(output);
        
    SpriteStream.RestartStrip();
       
}
// The draw GS just expands points into camera facing quads.
[maxvertexcount(4)]
void DrawGS(point Particle gin[1], inout TriangleStream<GeoOut> triStream)
			
{	
	// do not draw emitter particles.

		//
		// Compute world matrix so that billboard faces the camera.
		//
		float3 look  = normalize(gEyePosW.xyz - gin[0].InitialPosW);
		float3 right = normalize(cross(float3(0,1,0), look));
		float3 up    = cross(look, right);
		
		//
		// Compute triangle strip vertices (quad) in world space.
		//
		float halfWidth  = 0.05f*gin[0].SizeW.x;
		float halfHeight = 0.5f*gin[0].SizeW.y;
	
		float4 v[4];

		v[0] = float4(gin[0].InitialPosW + halfWidth*right - halfHeight*up, 1.0f);
		v[1] = float4(gin[0].InitialPosW + halfWidth*right + halfHeight*up, 1.0f);
		v[2] = float4(gin[0].InitialPosW - halfWidth*right - halfHeight*up, 1.0f);
		v[3] = float4(gin[0].InitialPosW - halfWidth*right + halfHeight*up, 1.0f);
		
		//
		// Transform quad vertices to world space and output 
		// them as a triangle strip.
		//
		GeoOut gout;
		[unroll]
		for(int i = 0; i < 4; ++i)
		{
			gout.PosH  = mul(float4(v[i].x,v[i].z,v[i].y,1.0f), gViewProj);//本框架y,z是相反的
			gout.Tex   = gQuadTexC[i];
		
			triStream.Append(gout);
		}	
		triStream.RestartStrip();
}

float4 DrawPS(GeoOut pin) : SV_TARGET
{
	//return gTexArray.Sample(samLinear, float3(pin.Tex, 0))*pin.Color;
	return float4(1,1,1,0.27*0.9*0.1);
}
RasterizerState CullNone
{
    MultiSampleEnable = False;
    CullMode=None;
};
technique11 DrawTech
{
    pass P0
    {
		SetVertexShader(   CompileShader( vs_5_0, StreamOutVS() ) );
       //SetGeometryShader( CompileShader( gs_5_0, DrawGS() ) );
		 SetGeometryShader( CompileShader( gs_5_0, GSRenderRainCheap() ) );
        SetPixelShader(    CompileShader( ps_5_0, DrawPS() ) );
        
        SetBlendState(AdditiveBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        SetDepthStencilState( NoDepthWrites, 0 );
		SetRasterizerState( CullNone );
    }
}