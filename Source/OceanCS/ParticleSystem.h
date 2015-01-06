//***************************************************************************************
// ParticleSystem.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

//#include "d3dUtil.h"
#include <string>
#include <vector>
#include"rain.h"
#include"d3dutil.h"
//class Camera;
class ParticleEffect;
/*static float RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	// Returns random float in [a, b).
	static float RandF(float a, float b)
	{
		return a + RandF()*(b-a);
	}*/
	#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }
struct Particle
	{
		D3DXVECTOR3 InitialPos;
		D3DXVECTOR3 InitialVel;
	//	D3DXVECTOR3 speed;
		D3DXVECTOR2 Size;
	//	float Age;
		D3DXVECTOR3 Age;
		unsigned int Type;
		
	};
/*struct WindValue
{
   D3DXVECTOR3 windAmount;
   int time;
   WindValue(D3DXVECTOR3 wA, int t){windAmount = wA; time=t;};
};*/
class ParticleSystem
{
public:
	
	ParticleSystem(ID3D11Device* mpd3dDevice);
	~ParticleSystem();
	float Random()
	{
    return (float(   (double)rand() / ((double)(RAND_MAX)+(double)(1)) ));
	}
	// Time elapsed since the system was reset.
	float GetAge()const;

	void SetEyePos(const D3DXVECTOR3& eyePosW);
	void SetEmitPos(const D3DXVECTOR3& emitPosW);
	void SetEmitDir(const D3DXVECTOR3& emitDirW);

	void Init();
	void Raininit(ID3D11Device* device);
	void Reset();
	void Update(float dt, float gameTime);
	void Draw(D3DXVECTOR3 a,D3DXMATRIX WVP,float dt);
	HRESULT buildFX();
	ID3D11ShaderResourceView* CreateRandomTexture1DSRV();

private:
	void BuildVB();

	ParticleSystem(const ParticleSystem& rhs);
	ParticleSystem& operator=(const ParticleSystem& rhs);
 
private:
//	ID3D11Buffer *g_pParticleStart;
//	ID3D11Buffer *g_pParticleDrawFrom;
//	ID3D11Buffer *g_pParticleStreamTo;
	//UINT mMaxParticles;
	bool mFirstRun;
	int g_numRainVertices;
	//vector<WindValue> WindAnimation;
	float mGameTime;
	float mTimeStep;
	float mAge;
	float g_heightRange;
	float g_radiusRange;
	D3DXVECTOR3 g_vecEye;
	D3DXVECTOR3 mEyePosW;
	D3DXVECTOR3 mEmitPosW;
	D3DXVECTOR3 mEmitDirW;
	vector<WindValue> WindAnimation;
	ParticleEffect* mFX;

	ID3D11Buffer* mInitVB;	
	ID3D11Buffer* mDrawVB;
	ID3D11Buffer* mStreamOutVB;
 
//	ID3D11ShaderResourceView* mTexArraySRV;
//	ID3D11ShaderResourceView* mRandomTexSRV;

	ID3DX11EffectTechnique* StreamOutTech;
	ID3DX11EffectTechnique* DrawTech;

	ID3DX11EffectMatrixVariable* ViewProj;
	ID3DX11EffectScalarVariable* GameTime;
	ID3DX11EffectScalarVariable* TimeStep;
	ID3DX11EffectVectorVariable* EyePosW;
	ID3DX11EffectVectorVariable* EmitPosW;
	ID3DX11EffectVectorVariable* EmitDirW;
	ID3DX11EffectVectorVariable* g_TotalVel;
	ID3DX11EffectShaderResourceVariable* TexArray;
	ID3DX11EffectShaderResourceVariable* RandomTex;
	ID3DX11Effect*  g_pEffect  ;
	ID3DX11EffectScalarVariable* g_FrameRate;
	ID3D11InputLayout* Particlelayout;
	ID3D11Device* pd3dDevice;
	ID3D11DeviceContext* pd3dContex;
	ID3D11ShaderResourceView* RandomSRV;
	int maxParticles;
	
};

#endif // PARTICLE_SYSTEM_H