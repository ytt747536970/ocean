// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

#ifndef _OCEAN_WAVE_H
#define _OCEAN_WAVE_H

#include <D3DX11.h>

#include "CSFFT/fft_512x512.h"

//#define CS_DEBUG_BUFFER
#define PAD16(n) (((n)+15)/16*16)

struct OceanParameter
{
	// Must be power of 2.
	int dmap_dim;
	// Typical value is 1000 ~ 2000
	float patch_length;

	// Adjust the time interval for simulation.
	float time_scale;
	// Amplitude for transverse wave. Around 1.0
	float wave_amplitude;
	// Wind direction. Normalization not required.
	D3DXVECTOR2 wind_dir;
	// Around 100 ~ 1000
	float wind_speed;
	// This value damps out the waves against the wind direction.
	// Smaller value means higher wind dependency.
	float wind_dependency;
	// The amplitude for longitudinal wave. Must be positive.
	float choppy_scale;
};


class OceanSimulator
{
public:
	OceanSimulator(OceanParameter& params, ID3D11Device* pd3dDevice);
	~OceanSimulator();

	// -------------------------- Initialization & simulation routines ------------------------

	// Update ocean wave when tick arrives.
	void updateDisplacementMap(float time,ID3D11ShaderResourceView* gCurrStepSRV);

	// Texture access
	ID3D11ShaderResourceView* getD3D11DisplacementMap();
	ID3D11ShaderResourceView* getD3D11GradientMap();

	const OceanParameter& getParameters();

	ID3D11ShaderResourceView* getD3D11CurrStepMap();
	void Change(OceanParameter& params);
protected:
	OceanParameter m_param;

	// ---------------------------------- GPU shading asset -----------------------------------

	// D3D objects
	ID3D11Device* m_pd3dDevice;
	ID3D11DeviceContext* m_pd3dImmediateContext;
	
	// Displacement map
	ID3D11Texture2D* m_pDisplacementMap;		// (RGBA32F)
	ID3D11ShaderResourceView* m_pDisplacementSRV;
	ID3D11RenderTargetView* m_pDisplacementRTV;

	// Gradient field
	ID3D11Texture2D* m_pGradientMap;			// (RGBA16F)
	ID3D11ShaderResourceView* m_pGradientSRV;
	ID3D11RenderTargetView* m_pGradientRTV;

	// Samplers
	ID3D11SamplerState* m_pPointSamplerState;

	// Initialize the vector field.
	void initHeightMap(OceanParameter& params, D3DXVECTOR2* out_h0, float* out_omega);


	// ----------------------------------- CS simulation data ---------------------------------

	// Initial height field H(0) generated by Phillips spectrum & Gauss distribution.
	ID3D11Buffer* m_pBuffer_Float2_H0;
	ID3D11UnorderedAccessView* m_pUAV_H0;
	ID3D11ShaderResourceView* m_pSRV_H0;

	// Angular frequency
	ID3D11Buffer* m_pBuffer_Float_Omega;
	ID3D11UnorderedAccessView* m_pUAV_Omega;
	ID3D11ShaderResourceView* m_pSRV_Omega;

	// Height field H(t), choppy field Dx(t) and Dy(t) in frequency domain, updated each frame.
	ID3D11Buffer* m_pBuffer_Float2_Ht;
	ID3D11UnorderedAccessView* m_pUAV_Ht;
	ID3D11ShaderResourceView* m_pSRV_Ht;

	// Height & choppy buffer in the space domain, corresponding to H(t), Dx(t) and Dy(t)
	ID3D11Buffer* m_pBuffer_Float_Dxyz;
	ID3D11UnorderedAccessView* m_pUAV_Dxyz;
	ID3D11ShaderResourceView* m_pSRV_Dxyz;

	ID3D11Buffer* m_pQuadVB;

	// Shaders, layouts and constants
	ID3D11ComputeShader* m_pUpdateSpectrumCS;

	ID3D11VertexShader* m_pQuadVS;
	ID3D11PixelShader* m_pUpdateDisplacementPS;
	ID3D11PixelShader* m_pGenGradientFoldingPS;

	ID3D11InputLayout* m_pQuadLayout;

	ID3D11Buffer* m_pImmutableCB;
	ID3D11Buffer* m_pPerFrameCB;

	// FFT wrap-up
	CSFFT512x512_Plan m_fft_plan;

	//�Ķ�
	ID3D11Texture2D* m_pCurrStepMap;		// (RGBA32F)
	ID3D11ShaderResourceView* m_pCurrStepMapSRV;
	ID3D11RenderTargetView* m_pCurrStepMapRTV;

#ifdef CS_DEBUG_BUFFER
	ID3D11Buffer* m_pDebugBuffer;
#endif
};

#endif	// _OCEAN_WAVE_H
