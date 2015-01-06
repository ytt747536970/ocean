// ----------------------------------------------------------
// sky.cpp
// ----------------------------------------------------------

#include "DXUT.h"
#include "sky.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include "Effects.h"
#include <fstream>
//#include"d3dUtil.h"
//#include"Atmosphere.h"
//#include"Sun.h"
using namespace std;
//#include "Camera.h"
//extern CCAMERA g_camera;
extern int windowWidth;
extern int windowHeight;
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );
Sky::Sky(ID3D11Device* mpd3dDevice)
{
	
	thetaS = 0.0;
	phiS = 0.0;
	sun[0] = 0.0;
	sun[1] = 1.0;
	sun[2] = 0.0;
	T = 3.0;
	DomeVB = NULL;
	pd3dDevice = mpd3dDevice;
	assert(pd3dDevice);
	pd3dDevice->GetImmediateContext(&pd3dContext);
	assert(pd3dContext);
	
}

Sky::~Sky() 
{
	SAFE_RELEASE(DomeVB);
	SAFE_RELEASE(DomeIB);
//	glDeleteBuffers(1, &VBO);
//	glDeleteBuffers(1, &IBO);

}

void Sky::computeColour(SkyElement *e)
{
	
	double a;
	double theta, gamma, chi;
	
	double YZenith;
	double xZenith;
	double yZenith;
	double A, B, C, D, E;
	double Y, x, y, z;
	double d;
	
	a = e->v[0]*sun[0] + e->v[1]*sun[1] + e->v[2]*sun[2];

	if(a>1)
		gamma = 0;
	else if(a<-1)
		gamma = PI;
	else
		gamma = acos(a);

	theta = e->theta;
	
	// CIE Y
	//
	
	chi = (4.0f/9.0f - T/120.0f)*(PI - 2*thetaS);
	YZenith = (4.0453*T - 4.9710)*tan(chi) - 0.2155*T + 2.4192;

	if (YZenith < 0.0) YZenith = -YZenith;
//	YZenith *=1000;

	
	
	A = YDC[0][0]*T + YDC[0][1];
	B = YDC[1][0]*T + YDC[1][1];
	C = YDC[2][0]*T + YDC[2][1];
	D = YDC[3][0]*T + YDC[3][1];
	E = YDC[4][0]*T + YDC[4][1];
	
	d = computeDistribution(A, B, C, D, E, theta, gamma);
	Y = YZenith * d;


	Y = 1-exp(-1.0/25.0*Y);


	// x
	//
	
	xZenith = computeChromaticity(xZC);

	A = xDC[0][0]*T + xDC[0][1];
	B = xDC[1][0]*T + xDC[1][1];
	C = xDC[2][0]*T + xDC[2][1];
	D = xDC[3][0]*T + xDC[3][1];
	E = xDC[4][0]*T + xDC[4][1];
	
	d = computeDistribution(A, B, C, D, E, theta, gamma);

	x = xZenith * d;
	
	// y
	//
	
	yZenith = computeChromaticity(yZC);

	A = yDC[0][0]*T + yDC[0][1];
	B = yDC[1][0]*T + yDC[1][1];
	C = yDC[2][0]*T + yDC[2][1];
	D = yDC[3][0]*T + yDC[3][1];
	E = yDC[4][0]*T + yDC[4][1];
	
	d = computeDistribution(A, B, C, D, E, theta, gamma);

	y = yZenith * d;

	z = 1.0 - x - y;
	
	e->X = (x/y)*Y;
	e->Y = Y;
	e->Z = (z/y)*Y;
	
}

double Sky::computeDistribution(double A, double B,
								double C, double D,
								double E,
								double theta,
								double gamma)
{
	
	double e0, e1, e2;
	double f0, f1, f2;
	
	e0 = B / cos(theta);
	e1 = D * gamma;
	e2 = cos(gamma);
	e2 *= e2;
	f1 = (1 + A*exp(e0)) * (1 + C*exp(e1) + E*e2);
	
	e0 = B;
	e1 = D * thetaS;
	e2 = cos(thetaS);
	e2 *= e2;
	f2 = (1 + A*exp(e0)) * (1 + C*exp(e1) + E*e2);
	
	f0 = f1 / f2;
	return f0;
	
}
//计算色度
double Sky::computeChromaticity(double m[][4])
{
	
	double a;
	double T2;
	double t2, t3;
	
	T2 = T*T;
	
	t2 = thetaS*thetaS;
	t3 = thetaS*thetaS*thetaS;
	
	a =
		(T2*m[0][0] + T*m[1][0] + m[2][0])*t3 +
		(T2*m[0][1] + T*m[1][1] + m[2][1])*t2 +
		(T2*m[0][2] + T*m[1][2] + m[2][2])*thetaS +
		(T2*m[0][3] + T*m[1][3] + m[2][3]);
	
	return a;
	
}
void Sky::init(float radius, float dtheta, float dphi, float thetaSun, float phiSun, float turbidity)
{
	D3D11_INPUT_ELEMENT_DESC mesh_layout_desc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	buildFX();
	
	SkyTech = g_pEffect->GetTechniqueByName("SkyTech");
	ViewProj = g_pEffect->GetVariableByName("WorldViewProj")->AsMatrix();
	EyePosW = g_pEffect->GetVariableByName("gEyePosW")->AsVector();
	World = g_pEffect->GetVariableByName("gWorld")->AsMatrix();
	//Ftable = g_pEffect->GetVariableByName("Ftable")->AsShaderResource();
	if(loadLUTS("F_512_data.csv","Ftable",512,512) == S_FALSE)
		   loadLUTS("F_512_data.csv","Ftable",512,512);
	D3DX11_PASS_DESC passDesc;
	SkyTech->GetPassByIndex(0)->GetDesc(&passDesc);
	pd3dDevice->CreateInputLayout(mesh_layout_desc, 2, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &DomeLayout);

	

	SkyScatterTech = g_pEffect->GetTechniqueByName("SkyScatterTech");
	sunDirection = g_pEffect->GetVariableByName("sunDirection")->AsVector();
	betaRayleighMie = g_pEffect->GetVariableByName("betaRayleighMie")->AsVector();
	reflectance = g_pEffect->GetVariableByName("reflectance")->AsVector();
	HGg = g_pEffect->GetVariableByName("HGg")->AsVector();
	betaDashRay = g_pEffect->GetVariableByName("betaDashRay")->AsVector();
	betaDashMie = g_pEffect->GetVariableByName("betaDashMie")->AsVector();
	oneOverBetaRM = g_pEffect->GetVariableByName("oneOverBetaRM")->AsVector();;																																																																																																																																																																																																																																																																																																																																																																																																																								
	sunColourIntensity = g_pEffect->GetVariableByName("sunColourIntensity")->AsVector();		
	termMultipliers = g_pEffect->GetVariableByName("termMultipliers")->AsVector();
	gWVP = g_pEffect->GetVariableByName("gWVP")->AsMatrix(); ;
	eyePos = g_pEffect->GetVariableByName("eyePos")->AsVector();;
	D3DX11_PASS_DESC passDesc1;
	SkyScatterTech->GetPassByIndex(0)->GetDesc(&passDesc1);
	pd3dDevice->CreateInputLayout(mesh_layout_desc, 2, passDesc1.pIAInputSignature, passDesc1.IAInputSignatureSize, &DomeLayout1);

	setSVector(thetaSun, phiSun);
	setTurbidity(turbidity);
	this->radius = radius;
	
	SkyElement *Vertices;

	// Initialize our Vertex array
	rows = 90/dphi;
	cols = 360/dtheta;
	mNumVertices = rows * cols;
	mNumFaces = (rows-1)*cols*2;
	Vertices = new SkyElement[mNumVertices];                       // for compute color
	ZeroMemory(Vertices, sizeof(SkyElement)*mNumVertices);

	// Used to calculate the UV coordinates
	float vx, vy, vz, mag;

	// Generate the dome
	int n = 0;
	for (int theta=0; theta+dtheta <= 90; theta += (int)dtheta)
	{
		for (int phi=0; phi+dtheta <= 360 ; phi += (int)dphi)
		{
			// Calculate the vertex at phi, theta
			Vertices[n].x = radius * sinf(theta*DTOR) * cosf(DTOR*phi);
			Vertices[n].z = radius * sinf(theta*DTOR) * sinf(DTOR*phi);
			Vertices[n].y = radius * cosf(theta*DTOR);

			// Create a vector from the origin to this vertex
			vx = Vertices[n].x;
			vy = Vertices[n].y;
			vz = Vertices[n].z;

			// Normalize the vector
			mag = (float)sqrt(SQR(vx)+SQR(vy)+SQR(vz));
			vx /= mag;
			vy /= mag;
			vz /= mag;
            Vertices[n].v[0] = vx;
			Vertices[n].v[1] = vy;
		    Vertices[n].v[2] = vz;
			Vertices[n].theta = DTOR*theta;
			Vertices[n].phi = DTOR*phi;
			n++;

		}
	}

	//buildVB
	std::vector<SkyVertex> vertices(mNumVertices);   // for shader
	for( int i = 0; i< mNumVertices; i++)
	{
        computeColour(&Vertices[i]);
		XYZtoRGB(Vertices[i].X, Vertices[i].Y, Vertices[i].Z, &Vertices[i].r, &Vertices[i].g, &Vertices[i].b);	
		vertices[i].pos = D3DXVECTOR3(Vertices[i].x, Vertices[i].z, Vertices[i].y);
		vertices[i].color = D3DXVECTOR4(1.5*Vertices[i].r, 1.5*Vertices[i].g, 1.5*Vertices[i].b,1.0f);
	}
	delete []Vertices;

/*	glGenBuffers(1, &VBO); 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, mNumVertices*sizeof(SkyVertex), &vertices[0], GL_STATIC_DRAW);*/

	D3D11_BUFFER_DESC vb_desc;
	vb_desc.ByteWidth = mNumVertices * sizeof(SkyVertex);
	vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
	vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vb_desc.CPUAccessFlags = 0;
	vb_desc.MiscFlags = 0;
	vb_desc.StructureByteStride = sizeof(SkyVertex);

	D3D11_SUBRESOURCE_DATA init_data;
	init_data.pSysMem = &vertices[0];
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;
	
	SAFE_RELEASE(DomeVB);
	pd3dDevice->CreateBuffer(&vb_desc, &init_data, &DomeVB);
	assert(DomeVB);

	//buildIB
	std::vector<unsigned int> indices(mNumFaces*3); // 3 indices per face

	// Iterate over each quad and compute indices.
	int k = 0;
	for(unsigned int i = 0; i < rows-1; ++i)
	{
		for(unsigned int j = 0; j < cols; ++j)
		{
			unsigned int nextj = (j+1==cols? 0:j+1);
			
			indices[k]   = i*cols+j;
			indices[k+1] = (i+1)*cols+j;
			indices[k+2] = i*cols+nextj;

			indices[k+3] = (i+1)*cols+j;
			indices[k+4] = (i+1)*cols+nextj;
			indices[k+5] = i*cols+nextj;

			k += 6; // next quad
		}
	}


	/* glGenBuffers(1, &IBO);
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	 glBufferData(GL_ELEMENT_ARRAY_BUFFER, mNumFaces*3*sizeof(unsigned int),&indices[0], GL_STATIC_DRAW); */
	D3D11_BUFFER_DESC ib_desc;
	ib_desc.ByteWidth = mNumFaces * 3 * sizeof(DWORD);
	ib_desc.Usage = D3D11_USAGE_IMMUTABLE;
	ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ib_desc.CPUAccessFlags = 0;
	ib_desc.MiscFlags = 0;
	ib_desc.StructureByteStride = sizeof(DWORD);

	D3D11_SUBRESOURCE_DATA init_data1;
	init_data1.pSysMem = &indices[0];

//	SAFE_RELEASE(DomeIB);
	pd3dDevice->CreateBuffer(&ib_desc, &init_data1, &DomeIB);
	assert(DomeIB);
	buildvbib();
//	buildshader();
	
//	SAFE_DELETE_ARRAY(indices);


//	 compileAndLinkShader();
	
}



void Sky::draw(D3DXMATRIX* pmWorldViewProj,D3DXVECTOR3 eyepos)
{
	EyePosW->SetFloatVector(eyepos);
	ViewProj->SetMatrix(*pmWorldViewProj);
	D3DXMATRIX W;
	D3DXMatrixTranslation(&W, eyepos.x,eyepos.y,eyepos.z);
	World->SetMatrix(W);
//	Ftable->SetResource();
	D3D11_BUFFER_DESC cb_desc;
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb_desc.MiscFlags = 0;    
	cb_desc.ByteWidth = PAD16(sizeof(SkyCall));
	cb_desc.StructureByteStride = 0;
	pd3dDevice->CreateBuffer(&cb_desc, NULL, &g_pTestCB);
	assert(g_pTestCB);	

	/*SkyCall call_consts;
	//call_consts.WorldViewProj = WVP;
	D3DXMatrixTranspose(&call_consts.WorldViewProj, pmWorldViewProj);
	D3D11_MAPPED_SUBRESOURCE mapped_res;            
	pd3dContext->Map(g_pTestCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res);
	assert(mapped_res.pData);
	*(SkyCall*)mapped_res.pData = call_consts;
	pd3dContext->Unmap(g_pTestCB, 0);

	ID3D11Buffer* cbs[1]={g_pTestCB};
	pd3dContext->VSSetConstantBuffers(0, 1, cbs);
	pd3dContext->PSSetConstantBuffers(0, 1, cbs);*/

	pd3dContext->IASetIndexBuffer(DomeIB, DXGI_FORMAT_R32_UINT, 0);
	ID3D11Buffer* vbs[1] = {DomeVB};
//	pd3dContext->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);
//	ID3D11Buffer* vbs[1] = {mVB};
	UINT strides[1] = {sizeof(SkyVertex)};
	UINT offsets[1] = {0};
	pd3dContext->IASetVertexBuffers(0, 1, &vbs[0], &strides[0], &offsets[0]);

//	pd3dContext->VSSetShader(g_pSkyVS, NULL, 0);
//	pd3dContext->PSSetShader(g_pSkyPS, NULL, 0);
	pd3dContext->IASetInputLayout(DomeLayout);
	pd3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	D3DX11_TECHNIQUE_DESC techDesc;
	SkyTech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        SkyTech->GetPassByIndex( p )->Apply(0, pd3dContext);
        pd3dContext->DrawIndexed(mNumFaces*3, 0, 0);//小网格
	//	pd3dContext->DrawIndexed(mNumIndices, 0, 0);
	
    }
	//pd3dContext->RSSetState(g_pRSState_Solid);
	
//	pd3dContext->RSSetState(g_pRSState_Solid);
}

void Sky::DrawScatter(Atmosphere *atm, Sun *sun,D3DXMATRIX* pmWorldViewProj,D3DXVECTOR3 eyepos)
{
	
	gWVP->SetMatrix(*pmWorldViewProj);
	eyePos->SetFloatVector(eyepos);

	D3DXVECTOR4 vSunDir = sun->GetDirection();
	D3DXVECTOR4 tmp1(vSunDir.x, vSunDir.y, vSunDir.z, 0);
	sunDirection->SetFloatVector((float*)tmp1);

	float fReflectance = 0.1f;
	D3DXVECTOR4 vDiffuse(0.138f,0.113f, 0.08f,0); // Taken from soil's relectance spectrum data.
	vDiffuse *= fReflectance; 
	reflectance->SetFloatVector((float*)vDiffuse);

	D3DXVECTOR4 vSunColorIntensity = sun->GetColorAndIntensity();
	sunColourIntensity->SetFloatVector((float*)vSunColorIntensity);

	// Scattering multipliers.
	float fRayMult = atm->GetParam(eAtmBetaRayMultiplier);
	float fMieMult = atm->GetParam(eAtmBetaMieMultiplier);

	D3DXVECTOR3 vBetaR, vBetaDashR, vBetaM, vBetaDashM, vBetaRM;

	// Rayleigh
	vBetaR = atm->GetBetaRayleigh();
	vBetaR *= fRayMult;
	
	
	vBetaDashR = atm->GetBetaDashRayleigh();
	vBetaDashR *= fRayMult;
	D3DXVECTOR4 tmp2 = D3DXVECTOR4(vBetaDashR.x,vBetaDashR.y,vBetaDashR.z,0);
	betaDashRay->SetFloatVector((float*)tmp2);

	// Mie
	vBetaM = atm->GetBetaMie();
	vBetaM *= fMieMult;
	
	vBetaDashM = atm->GetBetaDashMie();
	vBetaDashM *= fMieMult;
	D3DXVECTOR4 tmp3 = D3DXVECTOR4(vBetaDashM.x,vBetaDashM.y,vBetaDashM.z,0);
	betaDashMie->SetFloatVector((float*)tmp3);

	// Rayleigh + Mie (optimization)
	vBetaRM = vBetaR + vBetaM;
	D3DXVECTOR4 tmp4(vBetaR.x+vBetaM.x, vBetaR.y+vBetaM.y,vBetaR.z+vBetaM.z,0);

	betaRayleighMie->SetFloatVector((float*)tmp4);

	D3DXVECTOR4 vOneOverBetaRM;
	vOneOverBetaRM[0] = 1.0/vBetaRM[0];
	vOneOverBetaRM[1] = 1.0/vBetaRM[1];
	vOneOverBetaRM[2] = 1.0/vBetaRM[2];
	vOneOverBetaRM[3] = 0;
	oneOverBetaRM->SetFloatVector((float*)vOneOverBetaRM);

	// Henyey Greenstein's G value.
	float g = atm->GetParam(eAtmHGg);
	D3DXVECTOR4 vG(1-g*g, 1+g, 2*g, 0);
	HGg->SetFloatVector((float*)vG);
	
	// each term (extinction, inscattering multiplier)
	float fExt = atm->GetParam(eAtmExtinctionMultiplier);
	float fIns = atm->GetParam(eAtmInscatteringMultiplier);
	D3DXVECTOR4 vTermMultipliers(fExt,fIns,0,0);
	termMultipliers->SetFloatVector((float*)vTermMultipliers);

	pd3dContext->IASetIndexBuffer(DomeIB, DXGI_FORMAT_R32_UINT, 0);
	ID3D11Buffer* vbs[1] = {DomeVB};
//	pd3dContext->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);
//	ID3D11Buffer* vbs[1] = {mVB};
	UINT strides[1] = {sizeof(SkyVertex)};
	UINT offsets[1] = {0};
	pd3dContext->IASetVertexBuffers(0, 1, &vbs[0], &strides[0], &offsets[0]);

//	pd3dContext->VSSetShader(g_pSkyVS, NULL, 0);
//	pd3dContext->PSSetShader(g_pSkyPS, NULL, 0);
	pd3dContext->IASetInputLayout(DomeLayout1);
	pd3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	D3DX11_TECHNIQUE_DESC techDesc;
	SkyScatterTech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        SkyScatterTech->GetPassByIndex( p )->Apply(0, pd3dContext);
        pd3dContext->DrawIndexed(mNumFaces*3, 0, 0);//小网格
	//	pd3dContext->DrawIndexed(mNumIndices, 0, 0);
	
    }
}

void Sky::buildshader()
{
	ID3DBlob* pBlobSkyVS = NULL;
	ID3DBlob* pBlobSkyPS = NULL;
	CompileShaderFromFile(L"sky.hlsl", "SkyVS", "vs_4_0", &pBlobSkyVS);
	CompileShaderFromFile(L"sky.hlsl", "SkyPS", "ps_4_0", &pBlobSkyPS);
	assert(pBlobSkyVS);
	assert(pBlobSkyPS);
	pd3dDevice->CreateVertexShader(pBlobSkyVS->GetBufferPointer(), pBlobSkyVS->GetBufferSize(), NULL, &g_pSkyVS);
	pd3dDevice->CreatePixelShader(pBlobSkyPS->GetBufferPointer(), pBlobSkyPS->GetBufferSize(), NULL, &g_pSkyPS);
	assert(g_pSkyVS);
	assert(g_pSkyPS);
	D3D11_INPUT_ELEMENT_DESC mesh_layout_desc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	pd3dDevice->CreateInputLayout(mesh_layout_desc, 2, pBlobSkyVS->GetBufferPointer(), pBlobSkyVS->GetBufferSize(), &DomeLayout);
	assert(DomeLayout);

	SAFE_RELEASE(pBlobSkyVS);
	SAFE_RELEASE(pBlobSkyPS);
}

HRESULT Sky::buildFX()
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
	HRESULT hr = D3DX11CompileFromFile(L"sky.hlsl",0,0,0,"fx_5_0",flag,0,0,&compiledShader,&errMsg,0);    
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

HRESULT Sky::loadLUTS(char* fileName, LPCSTR shaderTextureName, int xRes, int yRes)
{
    HRESULT hr = S_OK;
	
    ifstream infile (fileName ,ios::in);
    if (infile.is_open())
    {   
        float* data = new float[xRes*yRes];
        int index = 0;
        char tempc;
        for(int j=0;j<yRes;j++)
        {   for(int i=0;i<xRes-1;i++)  
               infile>>data[index++]>>tempc;
            infile>>data[index++];
            
        }
        
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.SysMemPitch = sizeof(float) * xRes;
        InitData.pSysMem = data;

        ID3D11Texture2D* texture = NULL;
        D3D11_TEXTURE2D_DESC texDesc;
        ZeroMemory( &texDesc, sizeof(D3D11_TEXTURE2D_DESC) );
        texDesc.Width = xRes;
        texDesc.Height = yRes;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        pd3dDevice->CreateTexture2D(&texDesc,&InitData,&texture);

        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
        ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
        SRVDesc.Format = texDesc.Format;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;
        SRVDesc.Texture2D.MostDetailedMip = 0;

        ID3D11ShaderResourceView* textureRview;
        pd3dDevice->CreateShaderResourceView( texture, &SRVDesc, &textureRview);
		ID3DX11EffectShaderResourceVariable* textureRVar ;
		textureRVar = g_pEffect->GetVariableByName( shaderTextureName )->AsShaderResource();
        textureRVar->SetResource( textureRview );

		texture->Release();
		textureRview->Release();
        delete[] data;
    }
    else
       hr = S_FALSE;
    return hr;
}

void Sky::buildvbib()
{
	std::vector<D3DXVECTOR3> vertices;
	std::vector<DWORD> indices;

	BuildGeoSphere(2, radius, vertices, indices);

	std::vector<SkyVertex> skyVerts(vertices.size());
	for(size_t i = 0; i < vertices.size(); ++i)
	{
		// Scale on y-axis to turn into an ellipsoid to make a flatter Sky surface
		skyVerts[i].pos = 0.5f*vertices[i];
		skyVerts[i].color = D3DXVECTOR4(0,0,0,0);
	}

	D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(SkyVertex) * (UINT)skyVerts.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &skyVerts[0];
    pd3dDevice->CreateBuffer(&vbd, &vinitData, &mVB);

	mNumIndices = (UINT)indices.size();

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(DWORD) * mNumIndices;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    pd3dDevice->CreateBuffer(&ibd, &iinitData, &mIB);

}

/*void sky::drawtexture(ID3D11DeviceContext* pd3dImmediateContext)
{
	
	pd3dImmediateContext->Draw(4, 0);

}*/