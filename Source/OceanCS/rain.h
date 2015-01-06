#pragma once
#include <DXUT.h>
//#include<SDKmisc.h>


#include "DXUTCamera.h"
#include "d3dx11effect.h"
#include<vector>
using namespace std;
//#include "d3dx11effect.h"
struct RainOftenCall 
{
	D3DXVECTOR3 g_TotalVel ;
	float g_ResponsePointLight ;
	float dirLightIntensity ;
};
struct WindValue
{
   D3DXVECTOR3 windAmount;
   int time;
   WindValue(D3DXVECTOR3 wA, int t){windAmount = wA; time=t;};
};
 //structures
struct RainVertex
{
    D3DXVECTOR3 pos;
    D3DXVECTOR3 seed;
    D3DXVECTOR3 speed;
    float random;
    unsigned char  Type;
};
struct RainPerCall 
{
	D3DXMATRIX g_mWorldViewProj;
	float g_timeCycle;
	D3DXVECTOR3 g_eyePos;   //eye in world space
};
//#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return DXUTTrace( __FILE__, (DWORD)__LINE__, hr, L#x, true ); } }
class Rain
{
public:
	Rain(ID3D11Device* mpd3dDevice);
	~Rain();
	void init();
	void draw(D3DXMATRIX g_mWorldViewProj,float g_timeCycle, D3DXVECTOR3 g_eyePos);
	void buildshader();
	HRESULT buildFX();
	void Draw(D3DXMATRIX WVP,D3DXVECTOR3 eyePos);
private:
	ID3D11Device* pd3dDevice;
	ID3D11DeviceContext* pd3dContex;
	ID3D11VertexShader* g_pRainVS ;
	ID3D11PixelShader* g_pRainPS ;
	ID3D11GeometryShader* g_pRainGS ;
	ID3D11InputLayout* RainLayout;
	int g_numRainVertices /*= 150000*/;
	ID3D11Buffer*     g_pParticleDrawFrom  ;
	ID3D11Buffer*  g_pParticleStart;
	ID3D11Buffer*  g_pParticleStreamTo;
	ID3D11Buffer* g_pRainPerCB ;
	ID3D11Buffer* g_pRainOftenCB ;
	bool firstFrame;
	ID3DX11EffectTechnique* RenderCheapTech;
	ID3DX11EffectTechnique* AdvanceTech;
	ID3D11InputLayout* rainlayout;
//	D3DXMATRIX g_mWorldViewProj;
//	float g_timeCycle;
//	D3DXVECTOR3 g_eyePos;   
	RainOftenCall rainoften;

	vector<WindValue> WindAnimation;
	ID3DX11EffectMatrixVariable* WorldViewProj;
	ID3DX11EffectVectorVariable* g_eyePos; //eye in world space
	ID3DX11EffectScalarVariable* g_FrameRate;//不知道是否对
	ID3DX11EffectVectorVariable* g_TotalVel ;
	ID3DX11EffectScalarVariable* g_ResponseDirLight ;
	ID3DX11EffectScalarVariable* g_ResponsePointLight ;
	ID3DX11EffectScalarVariable* g_dirLightIntensity ;
	ID3DX11EffectScalarVariable* g_SpriteSize ;
	ID3DX11EffectScalarVariable* g_heightRange ;


	float heightRange;
	float radiusRange;
	D3DXVECTOR3 vecEye;
	ID3DX11Effect*  g_pEffect  ;
};

