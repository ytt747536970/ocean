#include <DXUT.h>
#include "rain.h"
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "ParticleSystem.h"
#define PAD16(n) (((n)+15)/16*16)
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );
float random()
{
    return (float(   (double)rand() / ((double)(RAND_MAX)+(double)(1)) ));
}
void Rain::init()
{
	buildFX();
	WorldViewProj = g_pEffect->GetVariableByName("g_mWorldViewProj")->AsMatrix();
	g_eyePos = g_pEffect->GetVariableByName("g_eyePos")->AsVector();
	g_FrameRate = g_pEffect->GetVariableByName("g_FrameRate")->AsScalar();
	g_TotalVel = g_pEffect->GetVariableByName("g_TotalVel")->AsVector ();
	g_ResponseDirLight = g_pEffect->GetVariableByName("g_ResponseDirLight")->AsScalar();
	g_ResponsePointLight = g_pEffect->GetVariableByName("g_ResponsePointLight")->AsScalar();
	g_dirLightIntensity = g_pEffect->GetVariableByName("dirLightIntensity")->AsScalar();
	g_SpriteSize = g_pEffect->GetVariableByName("g_SpriteSize")->AsScalar();
	g_heightRange = g_pEffect->GetVariableByName("g_heightRange")->AsScalar();
	RainVertex* vertices = new RainVertex[g_numRainVertices];
    if(vertices==NULL) 
        exit(0);

    for(int i=0;i<g_numRainVertices;i++)
    {
        RainVertex raindrop;
        //use rejection sampling to generate random points inside a circle of radius 1 centered at 0,0
        float SeedX;
        float SeedZ;
        bool pointIsInside = false;
        while(!pointIsInside)
        { 
           SeedX = random() - 0.5f;
           SeedZ = random() - 0.5f;
           if( sqrt( SeedX*SeedX + SeedZ*SeedZ ) <= 0.5f )
               pointIsInside = true;
        }
        //save these random locations for reinitializing rain particles that have fallen out of bounds
        SeedX *= radiusRange;
        SeedZ *= radiusRange;
        float SeedY = random()*heightRange;
        raindrop.seed = D3DXVECTOR3(SeedX,SeedY,SeedZ)/10000000; 
        
        //add some random speed to the particles, to prevent all the particles from following exactly the same trajectory
        //additionally, random speeds in the vertical direction ensure that temporal aliasing is minimized
        float SpeedX = 40.0f*(random()/20.0f);
        float SpeedZ = 40.0f*(random()/20.0f);
        float SpeedY = -40.0f*(random()/10.0f); 
        raindrop.speed = D3DXVECTOR3(SpeedX,SpeedY,SpeedZ);

        //move the rain particles to a random positions in a cylinder above the camera
        float x = SeedX + vecEye.x;
        float z = SeedZ + vecEye.z;
        float y = SeedY + vecEye.y;
       // raindrop.pos = D3DXVECTOR3(x,y,z); 
		raindrop.pos = 15*D3DXVECTOR3(RandF(-1, 1),RandF(-1, 1),RandF(-1, 1));
        //get an integer between 1 and 8 inclusive to decide which of the 8 types of rain textures the particle will use
        raindrop.Type = int(floor(  random()*8 + 1 ));

        //this number is used to randomly increase the brightness of some rain particles
        raindrop.random = 1;
        float randomIncrease = random();
        if( randomIncrease > 0.8)
            raindrop.random += randomIncrease;

        vertices[i] = raindrop;
    }


    //create vertex buffers for the rain, two will be used to pingpong between during animation
    D3D11_BUFFER_DESC bd;
    bd.ByteWidth = sizeof( RainVertex ) * g_numRainVertices;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
    InitData.pSysMem = vertices;
    InitData.SysMemPitch = sizeof(RainVertex);
    pd3dDevice->CreateBuffer( &bd, &InitData, &g_pParticleStart    ) ;
    bd.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
    pd3dDevice->CreateBuffer( &bd, NULL,      &g_pParticleDrawFrom ) ;
     pd3dDevice->CreateBuffer( &bd, NULL,      &g_pParticleStreamTo ) ;
    delete[] vertices;
	const D3D11_INPUT_ELEMENT_DESC rain[5] = 
	{
	 { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
     { "SEED",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
     { "SPEED",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
     { "RAND",     0, DXGI_FORMAT_R32_FLOAT,       0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
     { "TYPE",     0, DXGI_FORMAT_R8_UINT,         0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	D3DX11_PASS_DESC passDesc;
	RenderCheapTech = g_pEffect->GetTechniqueByName("RenderParticlesCheap");
	AdvanceTech = g_pEffect->GetTechniqueByName("AdvanceParticles");

	RenderCheapTech  ->GetPassByIndex(0)->GetDesc(&passDesc);
	pd3dDevice->CreateInputLayout(rain, 5, passDesc.pIAInputSignature, 	passDesc.IAInputSignatureSize, &rainlayout);

	heightRange = 40.0;
	vecEye = D3DXVECTOR3( 15.5f, 5.0f, 0.0f )/*(0.0f, 280.0f, 0.0f)*/;
	radiusRange = 45.0f; 
	g_pParticleStart = NULL;
	g_pParticleDrawFrom = NULL;
	g_pParticleStreamTo = NULL;
	//g_numRainVertices = 15000;
	 WindAnimation.clear();
    int time = 0; //time in seconds between each key
    WindAnimation.push_back( WindValue( D3DXVECTOR3(-0.1,-0.5,0),   time ) );
    WindAnimation.push_back( WindValue( D3DXVECTOR3(-0.4,-0.5,0.04), time += 10 ) ); 
    WindAnimation.push_back( WindValue( D3DXVECTOR3(-0.2,-0.5,-0.4),   time += 5 ) );   
    WindAnimation.push_back( WindValue( D3DXVECTOR3(0.0,-0.5,-0.02), time += 10 ) );
    WindAnimation.push_back( WindValue( D3DXVECTOR3(0.0,-0.5,-0.02), time += 10 ) );
    WindAnimation.push_back( WindValue( D3DXVECTOR3(0.1,-0.5,0.4),  time += 6) );
    WindAnimation.push_back( WindValue( D3DXVECTOR3(-0.1,-0.5,0),   time += 5 ) );
	g_TotalVel->SetFloatVector((float*)WindAnimation.at(0).windAmount);
	firstFrame = true;
	g_eyePos->SetFloatVector((float*)&vecEye);
}

void Rain::Draw(D3DXMATRIX WVP,D3DXVECTOR3 eyePos)
{
	/*ID3D11RenderTargetView* pOrigRT = DXUTGetD3D11RenderTargetView(); 
    ID3D11DepthStencilView* pOrigDS = DXUTGetD3D11DepthStencilView();
	//pd3dContex->OMGetRenderTargets(1,&pOrigRT,&pOrigDS);
    //clear the render target and depth stencil view
 //   float ClearColor[4] = {0.0,0.0,0.0,1.0};
 //   pd3dContex->ClearRenderTargetView( pOrigRT, ClearColor );
 //   pd3dContex->ClearDepthStencilView( pOrigDS, D3D11_CLEAR_DEPTH, 1.0, 0 );*/
/*	D3D11_VIEWPORT old_viewport;
	UINT num_viewport = 1;
	pd3dContex->RSGetViewports(&num_viewport, &old_viewport);
    //set the viewport and render target
   // pd3dDevice->RSSetViewports( 1, &g_SceneVP );
    pd3dContex->OMSetRenderTargets( 1,&pOrigRT,pOrigDS);*/
//	D3DXVECTOR3 D3DXVECTOR3 = *cam.GetEyePt();
	/*D3DXMATRIX T ;
	//D3DXMatrixTranslation(&T,0,0,0);
	D3DXMatrixIdentity(&T);
	D3DXMATRIX matProj = *cam.GetProjMatrix();
	D3DXMATRIX matView = *cam.GetViewMatrix();
	D3DXMATRIX WVP =  T * matProj * matView;*/

	float dirLightIntensity = 1.0;
	float ResponseDirLight = 1.0;
	//D3DXVECTOR3 TotalVel = D3DXVECTOR3(0,-0.25,0);
	float SpriteSize = 0.8;
	float ResponsePointLight = 1.0;
	float fFPS = DXUTGetFPS();
	WorldViewProj->SetMatrix (WVP);
	g_eyePos->SetFloatVector(eyePos);
	g_FrameRate->SetFloat(fFPS);
	//g_TotalVel->SetFloatVector(TotalVel);
	g_ResponseDirLight ->SetFloat(ResponseDirLight);
	g_ResponsePointLight ->SetFloat(ResponsePointLight);
	g_dirLightIntensity ->SetFloat(dirLightIntensity);
	g_SpriteSize->SetFloat(SpriteSize);
	g_heightRange->SetFloat(heightRange);
	static float time = 0;
    int lastTime = WindAnimation.back().time;
    int upperFrame = 1;

    float framesPerSecond = fFPS;
	float g_WindAmount = 1.0f;
    while( time > WindAnimation.at(upperFrame).time )
            upperFrame++;
        
    int lowerFrame = upperFrame - 1;
    float amount = float(time - WindAnimation.at(lowerFrame).time)/(WindAnimation.at(upperFrame).time - WindAnimation.at(lowerFrame).time);
	D3DXVECTOR3 interpolatedWind = WindAnimation.at(lowerFrame).windAmount + amount*( WindAnimation.at(upperFrame).windAmount - WindAnimation.at(lowerFrame).windAmount);
    //adjust the wind based on the current frame rate; the values were estimated for 40FPS
    interpolatedWind *= 40.0f/framesPerSecond;
   //lerp between the wind vector and just the vector pointing down depending on the amount of user chosen wind
    interpolatedWind = g_WindAmount*interpolatedWind + (1-g_WindAmount)*(D3DXVECTOR3(0,-0.5,0));
	g_TotalVel->SetFloatVector((float*)interpolatedWind);
    time += 1.0f/framesPerSecond;
   if(time>lastTime)
          time = 0;
	

	pd3dContex->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
    pd3dContex->IASetInputLayout(rainlayout);
    ID3D11Buffer *pBuffers[1];
    if(firstFrame)
         pBuffers[0] = g_pParticleStart;
    else
         pBuffers[0] = g_pParticleDrawFrom;

	UINT stride[1] = {sizeof(RainVertex)};
	UINT offset[1] = { 0 };
    pd3dContex->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
 
    // Point to the correct output buffer
    pBuffers[0] = g_pParticleStreamTo;
    pd3dContex->SOSetTargets( 1, pBuffers, offset );
	// draw
    D3DX11_TECHNIQUE_DESC techDesc;
	
	AdvanceTech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = AdvanceTech->GetPassByIndex(p);
		
		pass->Apply(0, pd3dContex);
	//	pd3dContex->DrawAuto();
		pd3dContex->Draw(g_numRainVertices , 0 );
	}
    // Get back to normal
    pBuffers[0] = NULL;
    pd3dContex->SOSetTargets( 1, pBuffers, offset );
	// Swap buffers
    ID3D11Buffer* pTemp = g_pParticleDrawFrom;
    g_pParticleDrawFrom = g_pParticleStreamTo;
    g_pParticleStreamTo = pTemp;
    
    firstFrame = false;
	//
	// Draw the updated particle system we just streamed-out. 
	//

	pd3dContex->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
    pd3dContex->IASetInputLayout(rainlayout);
	pd3dContex->IASetVertexBuffers(0, 1, &g_pParticleDrawFrom, stride, offset);
	//RenderCheapTech = g_pEffect->GetTechniqueByName("RenderParticlesCheap");
	RenderCheapTech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        RenderCheapTech->GetPassByIndex( p )->Apply(0, pd3dContex);    
	//	pd3dContex->DrawAuto();
		pd3dContex->Draw(g_numRainVertices, 0 );
    }

	// ID3D11RenderTargetView* pNULLRT = {NULL};
   // pd3dContex->OMSetRenderTargets( 1, &(pNULLRT), NULL );
}



 void Rain::draw (D3DXMATRIX g_mWorldViewProj,float g_timeCycle, D3DXVECTOR3 g_eyePos)
 {
 }
 Rain::Rain (ID3D11Device* mpd3dDevice)   //eye in world space)
{
	pd3dDevice = mpd3dDevice;
	assert(pd3dDevice);
	//pd3dContex = mpd3dContex;
	g_pRainVS = NULL;
	g_pRainPS = NULL;
	g_pRainGS = NULL;
	RainLayout = NULL;
	
	pd3dDevice->GetImmediateContext(&pd3dContex);
	assert(pd3dContex);
	g_numRainVertices =159 ;
	rainoften.g_TotalVel = D3DXVECTOR3(0,-0.25,0);
	rainoften.dirLightIntensity = 1.0f;
	rainoften.g_ResponsePointLight = 1.0f;
}
 Rain::~Rain()
 {
	 delete pd3dDevice;
	 delete g_pRainVS;
	 delete g_pRainPS;
	 delete g_pRainGS;
	 delete RainLayout;
	delete pd3dContex;
	delete RainLayout;
	delete  g_pParticleDrawFrom  ;
	delete g_pParticleStart;
	delete g_pParticleStreamTo;
	delete g_pRainPerCB ;
	delete g_pRainOftenCB ;
	delete RenderCheapTech;
	delete AdvanceTech;
	delete rainlayout;
  
	
	

 }

HRESULT Rain::buildFX()
{
	DWORD flag = 0;  
	#if defined(DEBUG) || defined(_DEBUG)   
    flag |= D3D10_SHADER_DEBUG;   
    flag |= D3D10_SHADER_SKIP_OPTIMIZATION;   
	#endif   
	//两个ID3D10Blob用来存放编译好的shader及错误消息     
	ID3D10Blob  *compiledShader(NULL);    
	ID3D10Blob  *errMsg(NULL);    
	//编译effect     
	HRESULT hr = D3DX11CompileFromFile(L"fire.fx",0,0,0,"fx_5_0",flag,0,0,&compiledShader,&errMsg,0);    
	//如果有编译错误，显示之     
	if(errMsg)    
	{    
	   MessageBoxA(NULL,(char*)errMsg->GetBufferPointer(),"ShaderCompileError",MB_OK);    
	  errMsg->Release();    
	  return FALSE;    
	}    
	if(FAILED(hr))    
	{    
	  MessageBox(NULL,L"CompileShader错误!",L"错误",MB_OK);    
	 return FALSE;    
	}    
  
	hr = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(),   
	 0, pd3dDevice, &g_pEffect);  
	if(FAILED(hr))  
	{  
	  MessageBox(NULL,L"CreateEffect错误!",L"错误",MB_OK);    
	  return FALSE;    
	}  
  
	
	 

	//std::wstring& filename = "rain.hlsl";
	
	/*std::ifstream fin("FX\rainny.fxo", std::ios::binary);
	
	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compiledShader(size);

	fin.read(&compiledShader[0], size);
	fin.close();
	
	D3DX11CreateEffectFromMemory(&compiledShader[0], size, 
		0, pd3dDevice, &g_pEffect);*/

	
}

  



