//***************************************************************************************
// ParticleSystem.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************
#include "DXUT.h"
#include "ParticleSystem.h"
//#include "TextureMgr.h"
//#include "Vertex.h"
#include "Effects.h"
//#include "Camera.h"

ParticleSystem::ParticleSystem(ID3D11Device* mpd3dDevice)
: mInitVB(0), mDrawVB(0), mStreamOutVB(0)
{
	pd3dDevice = mpd3dDevice;
	assert(pd3dDevice);
	pd3dDevice->GetImmediateContext(&pd3dContex);
	assert(pd3dContex);
	mFirstRun = true;
	mGameTime = 0.0f;
	mTimeStep = 0.0f;
	mAge      = 0.0f;
	g_numRainVertices = 13000;
	mEyePosW  = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	mEmitPosW = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	mEmitDirW = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
}

ParticleSystem::~ParticleSystem()
{
	delete mInitVB;
	delete mDrawVB;
	delete mStreamOutVB;
}

float ParticleSystem::GetAge()const
{
	return mAge;
}

void ParticleSystem::SetEyePos(const D3DXVECTOR3& eyePosW)
{
	mEyePosW = eyePosW;
}

void ParticleSystem::SetEmitPos(const D3DXVECTOR3& emitPosW)
{
	mEmitPosW = emitPosW;
}

void ParticleSystem::SetEmitDir(const D3DXVECTOR3& emitDirW)
{
	mEmitDirW = emitDirW;
}

void ParticleSystem::Init()
{
	
	buildFX();
	StreamOutTech = g_pEffect->GetTechniqueByName("StreamOutTech");
	DrawTech = g_pEffect->GetTechniqueByName("DrawTech");
	
	ViewProj = g_pEffect->GetVariableByName("gViewProj")->AsMatrix();
	GameTime = g_pEffect->GetVariableByName("gGameTime")->AsScalar();

	TimeStep = g_pEffect->GetVariableByName("gTimeStep")->AsScalar();
	EyePosW = g_pEffect->GetVariableByName("gEyePosW")->AsVector();
	EmitPosW = g_pEffect->GetVariableByName("gEmitPosW")->AsVector();
	//EmitDirW = g_pEffect->GetVariableByName("gEmitDirW")->AsVector();
	//TexArray = g_pEffect->GetVariableByName("gTexArray")->AsShaderResource();
	RandomTex = g_pEffect->GetVariableByName("gRandomTex")->AsShaderResource();
	g_TotalVel = g_pEffect->GetVariableByName("g_TotalVel")->AsVector();
	g_FrameRate = g_pEffect->GetVariableByName("g_FrameRate")->AsScalar();
	//mMaxParticles = maxParticles;
	const D3D11_INPUT_ELEMENT_DESC Particle[5] = 
	{
		
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		
		{"SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	//	{"SPEED",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//	{"AGE",      0, DXGI_FORMAT_R32_FLOAT,       0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"AGE",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TYPE",     0, DXGI_FORMAT_R32_UINT,        0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}
		
	};
	D3DX11_PASS_DESC passDesc;
	StreamOutTech->GetPassByIndex(0)->GetDesc(&passDesc);
	pd3dDevice->CreateInputLayout(Particle, 5, passDesc.pIAInputSignature, 
		passDesc.IAInputSignatureSize, &Particlelayout);
	//

//	mTexArraySRV  = texArraySRV;
//	mRandomTexSRV = randomTexSRV; 

	BuildVB();
	WindAnimation.clear();
    int time = 0; //time in seconds between each key
    WindAnimation.push_back( WindValue( D3DXVECTOR3(-0.1,-0.5,0),   time ) );
    WindAnimation.push_back( WindValue( D3DXVECTOR3(-0.4,-0.5,0.04), time += 10 ) ); 
    WindAnimation.push_back( WindValue( D3DXVECTOR3(-0.2,-0.5,-0.4),   time += 5 ) );   
    WindAnimation.push_back( WindValue( D3DXVECTOR3(0.0,-0.5,-0.02), time += 10 ) );
    WindAnimation.push_back( WindValue( D3DXVECTOR3(0.0,-0.5,-0.02), time += 10 ) );
    WindAnimation.push_back( WindValue( D3DXVECTOR3(0.1,-0.5,0.4),  time += 6) );
    WindAnimation.push_back( WindValue( D3DXVECTOR3(-0.1,-0.5,0),   time += 5 ) );
	g_TotalVel->SetFloatVector(WindAnimation.at(0).windAmount);
	 //populate the wind animation
    
}

void ParticleSystem::Reset()
{
	mFirstRun = true;
	mAge      = 0.0f;
}

void ParticleSystem::Update(float dt, float gameTime)
{
	mGameTime = gameTime;
	mTimeStep = dt;

	mAge += dt;
}

void ParticleSystem::Draw( D3DXVECTOR3 a,D3DXMATRIX WVP,float dt)
{
	//D3DXMATRIX WVP = *cam.GetProjMatrix()**cam.GetViewMatrix();
	//D3DXVECTOR3 a = *cam.GetEyePt();
	RandomSRV = CreateRandomTexture1DSRV();
	//float fFPS = DXUTGetFPS();
	
	static float time = 0;
    int lastTime = WindAnimation.back().time;
    int upperFrame = 1;
	float fFPS = DXUTGetFPS();
    float framesPerSecond = fFPS;
	float g_WindAmount = 1.0f;
	//double elapsetime = double(1/fFPS);
	//g_FrameRate->SetFloat(dt);
	g_FrameRate->SetFloat(framesPerSecond);
    while( time > WindAnimation.at(upperFrame).time )
            upperFrame++;
        
    int lowerFrame = upperFrame - 1;
    float amount = float(time - WindAnimation.at(lowerFrame).time)/(WindAnimation.at(upperFrame).time - WindAnimation.at(lowerFrame).time); 
	D3DXVECTOR3 interpolatedWind = WindAnimation.at(lowerFrame).windAmount + amount*( WindAnimation.at(upperFrame).windAmount - WindAnimation.at(lowerFrame).windAmount);
    //adjust the wind based on the current frame rate; the values were estimated for 40FPS
    interpolatedWind *= 40.0f/framesPerSecond;
   //lerp between the wind vector and just the vector pointing down depending on the amount of user chosen wind
    interpolatedWind = g_WindAmount*interpolatedWind + (1-g_WindAmount)*(D3DXVECTOR3(0,-0.5,0));
	g_TotalVel->SetFloatVector(interpolatedWind);
    time += 1.0f/framesPerSecond;
    if(time>lastTime)
          time = 0;

	GameTime->SetFloat(mGameTime);
	TimeStep->SetFloat(mTimeStep);
	EyePosW->SetFloatVector (a);
	EmitPosW->SetFloatVector(a);
	ViewProj->SetMatrix(WVP);
	RandomTex->SetResource(RandomSRV);
	
	// Set IA stage.
	//
	pd3dContex->IASetInputLayout(Particlelayout);
    pd3dContex->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	UINT stride = sizeof(Particle);
    UINT offset = 0;

	// On the first pass, use the initialization VB.  Otherwise, use
	// the VB that contains the current particle list.
	if( mFirstRun )
		pd3dContex->IASetVertexBuffers(0, 1, &mInitVB, &stride, &offset);
	else
		pd3dContex->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);

	//
	// Draw the current particle list using stream-out only to update them.  
	// The updated vertices are streamed-out to the target VB. 
	//
	pd3dContex->SOSetTargets(1, &mStreamOutVB, &offset);
	
    D3DX11_TECHNIQUE_DESC techDesc;
	StreamOutTech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        StreamOutTech->GetPassByIndex( p )->Apply(0, pd3dContex);
        
	/*	if( mFirstRun )
		{
			pd3dContex->Draw(1, 0);
			mFirstRun = false;
		}
		else
		{*/
			//dc->DrawAuto();
		//	pd3dContex->DrawAuto();
			pd3dContex->Draw(g_numRainVertices, 0 );
	//	}
    }

	// done streaming-out--unbind the vertex buffer
	ID3D11Buffer* bufferArray[1] = {0};
	pd3dContex->SOSetTargets(1, bufferArray, &offset);

	// ping-pong the vertex buffers
	std::swap(mDrawVB, mStreamOutVB);
	mFirstRun = false;
	//
	// Draw the updated particle system we just streamed-out. 
	//
	pd3dContex->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);

	DrawTech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
       DrawTech->GetPassByIndex( p )->Apply(0, pd3dContex);
        
		//pd3dContex->DrawAuto();
		pd3dContex->Draw(g_numRainVertices, 0 );
    }
	
}

void ParticleSystem::BuildVB()
{
	//
	// Create the buffer to kick-off the particle system.
	//

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(Particle) * g_numRainVertices;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// The initial particle emitter has type 0 and age 0.  The rest
	// of the particle attributes do not apply to an emitter.
	Particle *p = new Particle[g_numRainVertices];
//	ZeroMemory(&p, sizeof(Particle));
	for(int i=0;i<g_numRainVertices;i++)
	{
		p[i].InitialVel = D3DXVECTOR3(RandF(-1.0f, 1.0f),RandF(-1.0f, 1.0f),RandF(-1.0f, 1.0f));
		p[i].InitialPos = D3DXVECTOR3(0.0f, 280.0f, 0.0f)+35*p[i].InitialVel;
		//p[i].InitialPos = D3DXVECTOR3(0,0,0);
		p[i].Age  = 100*D3DXVECTOR3(RandF(-1.0f, 1.0f),RandF(-1.0f, 0.0f),RandF(-1.0f, 1.0f));
		p[i].Type = 0; 
		p[i].Size = D3DXVECTOR2(1.0f, 1.0f);
		

	}
 
    D3D11_SUBRESOURCE_DATA vinitData;
	ZeroMemory( &vinitData, sizeof(D3D11_SUBRESOURCE_DATA) );
    vinitData.pSysMem = p;
	vinitData.SysMemPitch = sizeof(Particle);
	pd3dDevice->CreateBuffer(&vbd, &vinitData, &mInitVB);
	
	//
	// Create the ping-pong buffers for stream-out and drawing.
	//
//	vbd.ByteWidth = sizeof(Particle) * g_numRainVertices;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

    pd3dDevice->CreateBuffer(&vbd, 0, &mDrawVB);
	pd3dDevice->CreateBuffer(&vbd, 0, &mStreamOutVB);
}



HRESULT ParticleSystem::buildFX()
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
	HRESULT hr = D3DX11CompileFromFile(L"Firey.fx",0,0,0,"fx_5_0",flag,0,0,&compiledShader,&errMsg,0);    
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
}

ID3D11ShaderResourceView* ParticleSystem::CreateRandomTexture1DSRV()
{
	// 
	// Create the random data.
	//
	D3DXVECTOR4 randomValues[1024];

	for(int i = 0; i < 1024; ++i)
	{
		randomValues[i].x = RandF(-1.0f, 1.0f);
		randomValues[i].y = RandF(-1.0f, 1.0f);
		randomValues[i].z = RandF(-1.0f, 1.0f);
		randomValues[i].w = RandF(-1.0f, 1.0f);
	}

    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = randomValues;
	initData.SysMemPitch = 1024*sizeof(D3DXVECTOR4);
    initData.SysMemSlicePitch = 0;

	//
	// Create the texture.
	//
    D3D11_TEXTURE1D_DESC texDesc;
    texDesc.Width = 1024;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    texDesc.ArraySize = 1;

	ID3D11Texture1D* randomTex = 0;
    pd3dDevice->CreateTexture1D(&texDesc, &initData, &randomTex);

	//
	// Create the resource view.
	//
    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
    viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture1D.MostDetailedMip = 0;
	
	ID3D11ShaderResourceView* randomTexSRV = 0;
    pd3dDevice->CreateShaderResourceView(randomTex, &viewDesc, &randomTexSRV);

	ReleaseCOM(randomTex);

	return randomTexSRV;
}