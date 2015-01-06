#include "DXUT.h"
#include"physics.h"
void createTextureAndViews(ID3D11Device* pd3dDevice, UINT width, UINT height, DXGI_FORMAT format,
						   ID3D11Texture2D** ppTex, ID3D11ShaderResourceView** ppSRV, ID3D11RenderTargetView** ppRTV);

Physics::Physics(ID3D11Device* mpd3dDevice,ID3D11DeviceContext* mpd3dContex,float Damping,float Speed,float Dx,float Dt,float MeshDim1)
{
	pd3dDevice = mpd3dDevice;
	assert(pd3dDevice);
	pd3dContex = mpd3dContex;
	assert(pd3dContex);
	DisturbVB = NULL;
	DisturbIB = NULL;
	m_pQuadVB = NULL;
	damping = Damping;
	speed =Speed;
	dx = Dx;
	dt = Dt;
	meshDim = MeshDim1;
	gPrevStepMapRTV = 0;
	gCurrStepMapRTV = 0;
	//ID3D11RenderTargetView* gNextStepMapRTV;
	gPrevStepMapSRV = 0;
	gCurrStepMapSRV = 0;
	gPrevStepTex = 0;
	gCurrStepTex = 0;
}
Physics::~Physics()
{
	SAFE_RELEASE(DisturbVB);
	SAFE_RELEASE(DisturbIB);
	SAFE_RELEASE(m_pQuadVB);

}
ID3D11ShaderResourceView* Physics::getgCurrStepMapSRV()
{
	return gCurrStepMapSRV;
}
ID3D11ShaderResourceView* Physics::getgPrevStepMapSRV()
{
	return gPrevStepMapSRV;
}
ID3D11ShaderResourceView* Physics::getgReflectMapSRV()
{
	return gReflectMapSRV;
}
ID3D11RenderTargetView* Physics::getgCurrStepMapRTV()
{
	return gCurrStepMapRTV;
}
ID3D11RenderTargetView* Physics::getgReflectMapRTV()
{
	return gReflectMapRTV;
}
ID3D11RenderTargetView* Physics::getgPrevStepMapRTV()
{
	return gPrevStepMapRTV;
}
/*ID3D11RenderTargetView* Physics::getgNextStepMapRTV()
{
	return gNextStepMapRTV;
}*/
void Physics::init()
{
	
	buildFX();
	PhysicsTech = g_pEffect->GetTechniqueByName("PhysicsTech");
	DisturbTech = g_pEffect->GetTechniqueByName("DisturbTech");
	CopyTech = g_pEffect->GetTechniqueByName("CopyTech");
	
	gPrevStepMap = g_pEffect->GetVariableByName("gPrevStepMap")->AsShaderResource();
	gCurrStepMap = g_pEffect->GetVariableByName("gCurrStepMap")->AsShaderResource();
	gK1 = g_pEffect->GetVariableByName("gK1")->AsScalar();
	gK2 = g_pEffect->GetVariableByName("gK2")->AsScalar();
	gK3 = g_pEffect->GetVariableByName("gK3")->AsScalar();
	gTexelSize = g_pEffect->GetVariableByName("gTexelSize")->AsScalar();
		// Quad vertex buffer
	D3D11_BUFFER_DESC vb_desc;
	vb_desc.ByteWidth = 4 * sizeof(D3DXVECTOR4);
	vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
	vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vb_desc.CPUAccessFlags = 0;
	vb_desc.MiscFlags = 0;
	float quad_verts[] =
	{
		-1, -1, 0, 1,
		-1,  1, 0, 1,
		 1, -1, 0, 1,
		 1,  1, 0, 1,
	};

	D3D11_SUBRESOURCE_DATA init_data;
	init_data.pSysMem = &quad_verts[0];
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;
	
	pd3dDevice->CreateBuffer(&vb_desc, &init_data, &m_pQuadVB);
	assert(m_pQuadVB);
	D3DX11_PASS_DESC passDesc;
	//改动
	D3D11_INPUT_ELEMENT_DESC quad_layout_desc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	PhysicsTech->GetPassByIndex(0)->GetDesc(&passDesc);
	pd3dDevice->CreateInputLayout(quad_layout_desc, 1, passDesc.pIAInputSignature, 
		passDesc.IAInputSignatureSize, &m_pQuadLayout);
	D3D11_INPUT_ELEMENT_DESC quad_layout_desc2[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	CopyTech->GetPassByIndex(0)->GetDesc(&passDesc);
	pd3dDevice->CreateInputLayout(quad_layout_desc2, 1, passDesc.pIAInputSignature, 
		passDesc.IAInputSignatureSize, &m_pQuadLayout1);
	
	//改动
	D3D11_INPUT_ELEMENT_DESC quad_layout_desc1[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	DisturbTech->GetPassByIndex(0)->GetDesc(&passDesc);
	pd3dDevice->CreateInputLayout(quad_layout_desc1, 1, passDesc.pIAInputSignature, 
		passDesc.IAInputSignatureSize, &g_pDisturbLayout);
	D3D11_BLEND_DESC blend_desc;
	memset(&blend_desc, 0, sizeof(D3D11_BLEND_DESC));
	blend_desc.AlphaToCoverageEnable = FALSE;
	blend_desc.IndependentBlendEnable = FALSE;
	blend_desc.RenderTarget[0].BlendEnable = TRUE;
	blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	pd3dDevice->CreateBlendState(&blend_desc, &g_pBState_Transparent);
	assert(g_pBState_Transparent);
	blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
//	blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
//	blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	pd3dDevice->CreateBlendState(&blend_desc, &g_pBState_Transparent1);
	assert(g_pBState_Transparent1);

	blend_desc.RenderTarget[0].BlendEnable = FALSE;
	pd3dDevice->CreateBlendState(&blend_desc, &g_pBState_Transparent2);
	assert(g_pBState_Transparent2);
	createTextureAndViews(pd3dDevice, 512, 512, DXGI_FORMAT_R16G16B16A16_FLOAT, &gPrevStepTex, &gPrevStepMapSRV, &gPrevStepMapRTV);
	createTextureAndViews(pd3dDevice, 512, 512, DXGI_FORMAT_R16G16B16A16_FLOAT, &gCurrStepTex, &gCurrStepMapSRV, &gCurrStepMapRTV);
	createTextureAndViews(pd3dDevice, 512, 512, DXGI_FORMAT_R16G16B16A16_FLOAT, &gReflectTex, &gReflectMapSRV, &gReflectMapRTV);
	buildDisturbIB();
}
void Physics::buildDisturbVB(float x,float y,float height)
{
	
	D3DXVECTOR3* pV = new D3DXVECTOR3[9];
	assert(pV);

/*	for (int i = 0; i < 8; i++)
	{//改变（x,y）周围八个点，半径为1
		float f = i * 2.0f * PI / 8;
		pV[i] = D3DXVECTOR3(x + 0.025f * cosf(f), y + 0.025f * sinf(f),0);
	}
	pV[8]=D3DXVECTOR3(x, y, height);*/
	float radius  =0.003;
	pV[0] = D3DXVECTOR3(x-radius, y+radius, 0.1f*height);
	pV[1] = D3DXVECTOR3(x, y+radius, 0.4f*height);
	pV[2] = D3DXVECTOR3(x+radius, y+radius, 0.1f*height);
	pV[3] = D3DXVECTOR3(x-radius, y, 0.4f*height);
	pV[4] = D3DXVECTOR3(x, y, height);
	pV[5] = D3DXVECTOR3(x+radius, y, 0.4f*height);
	pV[6] = D3DXVECTOR3(x-radius, y-radius, 0.1f*height);
	pV[7] = D3DXVECTOR3(x, y-radius, 0.4f*height);
	pV[8] = D3DXVECTOR3(x+radius, y-radius, 0.1f*height);
	D3D11_BUFFER_DESC vb_desc;
	vb_desc.ByteWidth = 9 * sizeof(D3DXVECTOR3);
	vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
	vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vb_desc.CPUAccessFlags = 0;
	vb_desc.MiscFlags = 0;
	vb_desc.StructureByteStride = sizeof(D3DXVECTOR3);

	D3D11_SUBRESOURCE_DATA init_data;
	init_data.pSysMem = pV;
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;
	
	SAFE_RELEASE(DisturbVB);
	pd3dDevice->CreateBuffer(&vb_desc, &init_data, &DisturbVB);
	assert(DisturbVB);

	SAFE_DELETE_ARRAY(pV);

}
void Physics::buildDisturbIB()
{
//	 numfaces = g_MeshDim1 * g_MeshDim1 * 2;
//	int num_verts = (g_MeshDim + 1) * (g_MeshDim + 1);
	int *indices = new int[8 * 3];
	assert(indices);

//	WORD* indices = 0;
//	mDisturbanceIB->Lock(0, 0, (void**)&indices, 0);
	indices[0]  = 0;  indices[1]  = 1;  indices[2]  = 3;
	indices[3]  = 1;  indices[4]  = 4;  indices[5]  = 3;
	indices[6]  = 1;  indices[7]  = 2;  indices[8]  = 4;
	indices[9]  = 2;  indices[10] = 5;  indices[11] = 4;
	indices[12] = 3;  indices[13] = 4;  indices[14] = 6;
	indices[15] = 4;  indices[16] = 7;  indices[17] = 6;
	indices[18] = 4;  indices[19] = 5;  indices[20] = 7;
	indices[21] = 5;  indices[22] = 8;  indices[23] = 7;

	D3D11_BUFFER_DESC ib_desc;
	ib_desc.ByteWidth = 8 * 3 * sizeof(DWORD);
	ib_desc.Usage = D3D11_USAGE_IMMUTABLE;
	ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ib_desc.CPUAccessFlags = 0;
	ib_desc.MiscFlags = 0;
	ib_desc.StructureByteStride = sizeof(DWORD);

	D3D11_SUBRESOURCE_DATA init_data;
	init_data.pSysMem = &indices[0];

	SAFE_RELEASE(DisturbIB);
	pd3dDevice->CreateBuffer(&ib_desc, &init_data, &DisturbIB);
	assert(DisturbIB);
	SAFE_DELETE_ARRAY(indices);



	// We disturb the ijth texel and its eight neighbors.
	// 0*-1*-2*
	//  |  |  |
	// 3*-4*-5*
	//  |  |  |
	// 6*-7*-8*

	
}
void Physics::drawdisturb(float x,float y,float height,ID3D11RenderTargetView* gCurrStepMapRTV)
{
	buildDisturbVB(x,y,height);
	pd3dContex->IASetIndexBuffer(DisturbIB, DXGI_FORMAT_R32_UINT, 0);
	ID3D11Buffer* vbs[1] = {DisturbVB};
	UINT strides[1] = {sizeof(D3DXVECTOR3)};
	UINT offsets[1] = {0};
	pd3dContex->IASetVertexBuffers(0, 1, &vbs[0], &strides[0], &offsets[0]);
	
	ID3D11RenderTargetView* old_target;
	ID3D11DepthStencilView* old_depth;
	pd3dContex->OMGetRenderTargets(1, &old_target, &old_depth); 

	D3D11_VIEWPORT old_viewport;
	UINT num_viewport = 1;
	pd3dContex->RSGetViewports(&num_viewport, &old_viewport);

	D3D11_VIEWPORT new_vp = {0, 0, (float)512, (float)512, 0.0f, 1.0f};
	pd3dContex->RSSetViewports(1, &new_vp);

	float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	pd3dContex->OMSetBlendState(g_pBState_Transparent1,blendFactor,0xffffffff);
	// Set RT
	ID3D11RenderTargetView* rt_views[1] = {gCurrStepMapRTV};
	pd3dContex->OMSetRenderTargets(1, rt_views, NULL);
	
	pd3dContex->IASetInputLayout(g_pDisturbLayout);
	pd3dContex->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	D3DX11_TECHNIQUE_DESC techDesc;
	DisturbTech->GetDesc( &techDesc );
	for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = DisturbTech->GetPassByIndex(p);
		pass->Apply(0, pd3dContex);
		pd3dContex->DrawIndexed(8*3, 0, 0);//小网格
	
	}
	
	pd3dContex->RSSetViewports(1, &old_viewport);
	pd3dContex->OMSetRenderTargets(1, &old_target, old_depth);
	SAFE_RELEASE(old_target);
	SAFE_RELEASE(old_depth);

	pd3dContex->OMSetBlendState(g_pBState_Transparent2,blendFactor,0xffffffff);
}
void Physics::draw(D3DXMATRIX g_mWorldViewProj, D3DXVECTOR3 g_eyePos,ID3D11ShaderResourceView* gPrevStepMapSRV,ID3D11ShaderResourceView* gCurrStepMapSRV,ID3D11RenderTargetView* gPrevStepMapRTV)
{
	gPrevStepMap->SetResource(gPrevStepMapSRV);
	gCurrStepMap->SetResource(gCurrStepMapSRV);
	float d      = damping*dt+2.0f;
	float e      = (speed*speed)*(dt*dt)/(dx*dx);
	float gk1          = (damping*dt-2.0f)/ d;
	float gk2          = (4.0f-8.0f*e) / d;
	float gk3          = (2.0f*e) / d;
	float gtexelSize   = 1.0f / (float)meshDim;
	gK1->SetFloat(gk1);
	gK2->SetFloat(gk2);
	gK3->SetFloat(gk3);
	gTexelSize->SetFloat(gtexelSize);

	ID3D11RenderTargetView* old_target;
	ID3D11DepthStencilView* old_depth;
	pd3dContex->OMGetRenderTargets(1, &old_target, &old_depth); 

	D3D11_VIEWPORT old_viewport;
	UINT num_viewport = 1;
	pd3dContex->RSGetViewports(&num_viewport, &old_viewport);

	D3D11_VIEWPORT new_vp = {0, 0, (float)512, (float)512, 0.0f, 1.0f};
	pd3dContex->RSSetViewports(1, &new_vp);

	// Set RT
	ID3D11RenderTargetView* rt_views[1] = {gCurrStepMapRTV};
	pd3dContex->OMSetRenderTargets(1, rt_views, NULL);
	
	ID3D11Buffer* vbs[1] = {m_pQuadVB};
	UINT strides[1] = {sizeof(D3DXVECTOR4)};
	UINT offsets[1] = {0};
	pd3dContex->IASetVertexBuffers(0, 1, &vbs[0], &strides[0], &offsets[0]);

	pd3dContex->IASetInputLayout(m_pQuadLayout);
	pd3dContex->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    D3DX11_TECHNIQUE_DESC techDesc;
	PhysicsTech->GetDesc( &techDesc );
	for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = PhysicsTech->GetPassByIndex(p);
		pass->Apply(0, pd3dContex);
		pd3dContex->Draw(4, 0);
	
	}
	pd3dContex->RSSetViewports(1, &old_viewport);
	pd3dContex->OMSetRenderTargets(1, &old_target, old_depth);
	SAFE_RELEASE(old_target);
	SAFE_RELEASE(old_depth);
}
void Physics::Copy(ID3D11ShaderResourceView* gPrevStepMapSRV,ID3D11ShaderResourceView* gCurrStepMapSRV,ID3D11RenderTargetView* gPrevStepMapRTV)
{
	gPrevStepMap->SetResource(gPrevStepMapSRV);
	gCurrStepMap->SetResource(gCurrStepMapSRV);
	ID3D11RenderTargetView* old_target;
	ID3D11DepthStencilView* old_depth;
	pd3dContex->OMGetRenderTargets(1, &old_target, &old_depth); 

	D3D11_VIEWPORT old_viewport;
	UINT num_viewport = 1;
	pd3dContex->RSGetViewports(&num_viewport, &old_viewport);

	D3D11_VIEWPORT new_vp = {0, 0, (float)512, (float)512, 0.0f, 1.0f};
	pd3dContex->RSSetViewports(1, &new_vp);

	// Set RT
	ID3D11RenderTargetView* rt_views[1] = {gPrevStepMapRTV};
	pd3dContex->OMSetRenderTargets(1, rt_views, NULL);
	
	ID3D11Buffer* vbs[1] = {m_pQuadVB};
	UINT strides[1] = {sizeof(D3DXVECTOR4)};
	UINT offsets[1] = {0};
	pd3dContex->IASetVertexBuffers(0, 1, &vbs[0], &strides[0], &offsets[0]);

	pd3dContex->IASetInputLayout(m_pQuadLayout1);
	pd3dContex->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    D3DX11_TECHNIQUE_DESC techDesc;
	CopyTech->GetDesc( &techDesc );
	for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = CopyTech->GetPassByIndex(p);
		pass->Apply(0, pd3dContex);
		pd3dContex->Draw(4, 0);
	
	}
	pd3dContex->RSSetViewports(1, &old_viewport);
	pd3dContex->OMSetRenderTargets(1, &old_target, old_depth);
	SAFE_RELEASE(old_target);
	SAFE_RELEASE(old_depth);
}
HRESULT Physics::buildFX()
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
	HRESULT hr = D3DX11CompileFromFile(L"physics.hlsl",0,0,0,"fx_5_0",flag,0,0,&compiledShader,&errMsg,0);    
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

void Physics::createTextureAndViews(ID3D11Device* pd3dDevice, UINT width, UINT height, DXGI_FORMAT format,
						   ID3D11Texture2D** ppTex, ID3D11ShaderResourceView** ppSRV, ID3D11RenderTargetView** ppRTV)
{
	// Create 2D texture
	D3D11_TEXTURE2D_DESC tex_desc;
	tex_desc.Width = width;
	tex_desc.Height = height;
	tex_desc.MipLevels = 0;
	tex_desc.ArraySize = 1;
	tex_desc.Format = format;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	pd3dDevice->CreateTexture2D(&tex_desc, NULL, ppTex);
	assert(*ppTex);

	// Create shader resource view
	(*ppTex)->GetDesc(&tex_desc);
	if (ppSRV)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		srv_desc.Format = format;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
		srv_desc.Texture2D.MostDetailedMip = 0;

		pd3dDevice->CreateShaderResourceView(*ppTex, &srv_desc, ppSRV);
		assert(*ppSRV);
	}

	// Create render target view
	if (ppRTV)
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
		rtv_desc.Format = format;
		rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtv_desc.Texture2D.MipSlice = 0;

		pd3dDevice->CreateRenderTargetView(*ppTex, &rtv_desc, ppRTV);
		assert(*ppRTV);
	}
}

