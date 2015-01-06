/***********************************************
// GLOBALS                                      *
//***********************************************


cbuffer cbPerFrame
{
	float g_FrameRate;
	float3 g_eyePos;   //eye in world space
	matrix g_mWorldViewProj;
	float3 g_TotalVel ;
	float g_ResponseDirLight ;
	float g_ResponsePointLight ;
	float dirLightIntensity ;
	float g_SpriteSize;
	float g_heightRange ;
};


DepthStencilState DisableDepth
{
        DepthEnable = false;
        DepthWriteMask = ZERO;
        DepthFunc = Less;
    
        //Stencil
        StencilEnable = false;
        StencilReadMask = 0xFF;
        StencilWriteMask = 0x00;
};

BlendState CorrectBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = ONE;
    DestBlendAlpha = ONE;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
}; 

DepthStencilState EnableDepthTestingOnly
{
        DepthEnable = true;
        DepthWriteMask = 0x00;
        DepthFunc = Less;
    
        StencilEnable = false;
};

RasterizerState CullNone
{
    MultiSampleEnable = False;
    CullMode=None;
};

struct VSParticleIn
{   
    float3 pos              : POSITION;         //position of the particle
    float3 seed             : SEED;
    float3 speed            : SPEED;
    float random            : RAND;
    uint   Type             : TYPE;             //particle type
};
//--------------------------------------------------------------------------------------------
// advance rain
//--------------------------------------------------------------------------------------------
VSParticleIn VSPassThroughRain(VSParticleIn input )
{
    return input;
}
VSParticleIn VSAdvanceRain(VSParticleIn input)
{
//     if(moveParticles)
 //    {
         //move forward
         input.pos.xyz += input.speed.xyz/g_FrameRate + g_TotalVel.xyz;

         //if the particle is outside the bounds, move it to random position near the eye         
         if(input.pos.y <=  g_eyePos.y-g_heightRange )
         {
            float x = input.seed.x + g_eyePos.x;
            float z = input.seed.z + g_eyePos.z;
            float y = input.seed.y + g_eyePos.y;
            input.pos = float3(x,y,z);
         }
//    }

    return input;
    
}

GeometryShader gsStreamOut = ConstructGSWithSO( CompileShader( vs_5_0, VSAdvanceRain() ), "POSITION.xyz; SEED.xyz; SPEED.xyz; RAND.x; TYPE.x" );
	
technique11 AdvanceParticles
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VSPassThroughRain() ) );
        SetGeometryShader( gsStreamOut );
        
        // disable pixel shader for stream-out only
        SetPixelShader(NULL);
        
        // we must also disable the depth buffer for stream-out only
        SetDepthStencilState( DisableDepth, 0 );
    }
}




//--------------------------------------------------------------------------------------------
// cheap pixel shader for rendering rain
//--------------------------------------------------------------------------------------------


struct PSRainIn
{
    float4 pos : SV_Position;
    float2 tex : TEXCOORD0;
};


void GenRainSpriteVertices(float3 worldPos, float3 velVec, float3 eyePos, out float3 outPos[4])
{
    float height = g_SpriteSize/2.0;
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

// GS for rendering rain as point sprites.  Takes a point and turns it into 2 tris.
[maxvertexcount(4)]
void GSRenderRainCheap(point VSParticleIn input[1], inout TriangleStream<PSRainIn> SpriteStream)
{
   
    PSRainIn output = (PSRainIn)0;
    
    float3 pos[4];
    GenRainSpriteVertices(input[0].pos, input[0].speed.xyz/g_FrameRate + g_TotalVel, g_eyePos, pos);
        
    output.pos = mul( float4(pos[0],1.0), g_mWorldViewProj );
    output.tex = float2(0,1);
    SpriteStream.Append(output);
        
    output.pos = mul( float4(pos[1],1.0), g_mWorldViewProj );
    output.tex = float2(1,1);
    SpriteStream.Append(output);
        
    output.pos = mul( float4(pos[2],1.0), g_mWorldViewProj );
    output.tex = float2(0.0f, 0.0f);
    SpriteStream.Append(output);
                
    output.pos = mul( float4(pos[3],1.0), g_mWorldViewProj );
    output.tex = float2(1.0f, 0.0f);
    SpriteStream.Append(output);
        
    SpriteStream.RestartStrip();
       
}

float4 PSRenderRainCheap(PSRainIn input) : SV_Target
{     
    return float4(1,1,1,1);
}


technique11 RenderParticlesCheap
{
    pass p0
    {
        SetVertexShader( CompileShader(   vs_5_0, VSPassThroughRain() ) );
        SetGeometryShader( CompileShader( gs_5_0, GSRenderRainCheap() ) );
        SetPixelShader( CompileShader(    ps_5_0, PSRenderRainCheap() ) );
        
        SetBlendState( CorrectBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepthTestingOnly, 0 );
        SetRasterizerState( CullNone );
    }  
}