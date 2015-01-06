#include <DXUT.h>
#include "d3dx11effect.h"
class Physics
{
public:
	Physics(ID3D11Device* mpd3dDevice,ID3D11DeviceContext* mpd3dContex,float Damping,float Speed,float Dx,float Dt,float MeshDim1);
	~Physics();
	void init();
	void draw(D3DXMATRIX g_mWorldViewProj, D3DXVECTOR3 g_eyePos,ID3D11ShaderResourceView* gPrevStepMapSRV,ID3D11ShaderResourceView* gCurrStepMapSRV,ID3D11RenderTargetView* gPrevStepMapRTV);
	HRESULT buildFX();
	void drawdisturb(float x,float y,float height,ID3D11RenderTargetView* gCurrStepMapRTV);
	void buildDisturbIB();
	void buildDisturbVB(float x,float y,float height);
	ID3D11ShaderResourceView* getgCurrStepMapSRV();
	ID3D11ShaderResourceView* getgPrevStepMapSRV();
	ID3D11ShaderResourceView* getgReflectMapSRV();
	//ID3D11ShaderResourceView* getgNextStepMapSRV();
	ID3D11RenderTargetView* getgCurrStepMapRTV();
	ID3D11RenderTargetView* getgPrevStepMapRTV();
	ID3D11RenderTargetView* getgReflectMapRTV();
	void Copy(ID3D11ShaderResourceView* gPrevStepMapSRV,ID3D11ShaderResourceView* gCurrStepMapSRV,ID3D11RenderTargetView* gPrevStepMapRTV);
	void createTextureAndViews(ID3D11Device* pd3dDevice, UINT width, UINT height, DXGI_FORMAT format,
						   ID3D11Texture2D** ppTex, ID3D11ShaderResourceView** ppSRV, ID3D11RenderTargetView** ppRTV);
	void DrawReflect(ID3D11RenderTargetView* gReflectMapRTV,D3DXMATRIX mWorldViewProjection);
private:
	ID3D11Device* pd3dDevice;
	ID3D11DeviceContext* pd3dContex;
	ID3DX11EffectTechnique* PhysicsTech;
	ID3DX11EffectTechnique* DisturbTech;
	ID3DX11EffectTechnique* CopyTech;
	ID3D11Texture2D* gPrevStepTex;
	ID3D11Texture2D* gCurrStepTex;
	ID3D11Texture2D* gReflectTex;
	ID3D11RenderTargetView* gPrevStepMapRTV;
	ID3D11RenderTargetView* gCurrStepMapRTV;
	ID3D11RenderTargetView* gReflectMapRTV;
	ID3D11ShaderResourceView* gPrevStepMapSRV;
	ID3D11ShaderResourceView* gCurrStepMapSRV;
	ID3D11ShaderResourceView* gReflectMapSRV;
	ID3DX11EffectShaderResourceVariable* gPrevStepMap;
	ID3DX11EffectShaderResourceVariable* gCurrStepMap;
	
	ID3DX11Effect*  g_pEffect;
	
	ID3D11InputLayout* m_pQuadLayout;
	ID3D11InputLayout* m_pQuadLayout1;
	ID3D11InputLayout* g_pDisturbLayout;
	ID3D11BlendState* g_pBState_Transparent ;
	ID3D11BlendState* g_pBState_Transparent1;
	ID3D11BlendState* g_pBState_Transparent2;
	ID3D11Buffer* m_pQuadVB;
	ID3D11Buffer* DisturbVB ;
	ID3D11Buffer* DisturbIB ;
	float damping;
	float speed;
	float dx;
	float dt;
	float meshDim;
	ID3DX11EffectScalarVariable* gK1;
	ID3DX11EffectScalarVariable* gK2;
	ID3DX11EffectScalarVariable* gK3;
	ID3DX11EffectScalarVariable* gTexelSize;
	
};
