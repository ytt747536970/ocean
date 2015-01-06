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

#include <DXUT.h>
#include <DXUTgui.h>
#include <DXUTmisc.h>
#include <DXUTCamera.h>
#include <DXUTSettingsDlg.h>
#include <SDKmisc.h>
#include<time.h>
#include "ocean_simulator.h"
#include "skybox11.h"
#include "sky.h"
#include"rain.h"
#include "ParticleSystem.h"
#include"physics.h"
//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager  g_DialogResourceManager;    // manager for shared resources of dialogs
CFirstPersonCamera          g_Camera;                   // A model viewing camera
CD3DSettingsDlg             g_D3DSettingsDlg;           // Device settings dialog
CDXUTDialog                 g_HUD;                      // dialog for standard controls
CDXUTDialog                 g_SampleUI;                 // dialog for sample specific controls

CDXUTTextHelper*            g_pTxtHelper = NULL;

// Ocean simulation variables
OceanSimulator* g_pOceanSimulator = NULL;

bool g_RenderWireframe = false;
bool g_PauseSimulation = false;
int g_BufferType = 0;

// Skybox
ID3D11Texture2D* g_pSkyCubeMap = NULL;
ID3D11ShaderResourceView* g_pSRV_SkyCube = NULL;

CSkybox11 g_Skybox;
Sky *sky;
Rain* rain;
ParticleSystem* particle;
//float thetaS = 1.3f;   //太阳经纬度
//float phiS = 1.91f;
float thetaS = 1.3f;
float phiS = 0.0f;
float turbidity = 3;
Atmosphere mAtm;
Sun mSun;
//--------------------------------------------------------------------------------------
//******************************************改动****************************************
//--------------------------------------------------------------------------------------
struct QuadVertex
{
	D3DXVECTOR3 pos;
	D3DXVECTOR2 texC;
};
ID3D11Buffer* mNDCQuadVB;
ID3D11VertexShader* g_pVS = NULL;
ID3D11PixelShader* g_pPS = NULL;
ID3D11InputLayout* PosTexlayout = NULL;
Physics *physics;
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
void showmap(ID3D11DeviceContext* pd3dImmediateContext,ID3D11Device* pd3dDevice,ID3D11ShaderResourceView* m_pCurrStepMapSRV);
ID3D11SamplerState* g_pCubeSampler1 = NULL;
ID3D11ShaderResourceView* gPrevStepMapSRV ;
ID3D11ShaderResourceView* gCurrStepMapSRV ;
ID3D11ShaderResourceView* gNextStepMapSRV ;
ID3D11ShaderResourceView* gReflectMapSRV ;


ID3D11RenderTargetView* gPrevStepMapRTV ;
ID3D11RenderTargetView* gCurrStepMapRTV ;
ID3D11RenderTargetView* gNextStepMapRTV ;
ID3D11RenderTargetView* gReflectMapRTV ;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_WIREFRAME			5
#define IDC_PAUSE				6

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext );

void InitApp();
void RenderText();

// init & cleanup
void initRenderResource(const OceanParameter& ocean_param, ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);
void cleanupRenderResource();
// Rendering routines
void renderShaded(const CBaseCamera& camera, ID3D11ShaderResourceView* displacemnet_map, ID3D11ShaderResourceView* gradient_map, float time, ID3D11DeviceContext* pd3dContext, ID3D11ShaderResourceView* g_ReflectMap
	,Atmosphere *atm, Sun *sun,ID3D11Device* pd3dDevice);
void renderWireframe(const CBaseCamera& camera, ID3D11ShaderResourceView* displacemnet_map, float time, ID3D11DeviceContext* pd3dContext,ID3D11ShaderResourceView* g_texCurrStep);
void draw(ID3D11DeviceContext* pd3dContext,ID3D11Device* pd3dDevice,ID3D11ShaderResourceView* gCurrStepMapSRV,ID3D11ShaderResourceView* gNextStepMapSRV,ID3D11ShaderResourceView* gReflectMap,const CBaseCamera& camera,ID3D11ShaderResourceView* g_pSRV_Gradient,ID3D11ShaderResourceView* gDisplaceMap,float fTime);
void drawdisturb(float x,float y,float height,ID3D11Device* pd3dDevice,ID3D11DeviceContext* pd3dContext,ID3D11RenderTargetView* gPrevStepMapRTV);
void next_norm(ID3D11Device* pd3dDevice,ID3D11DeviceContext* pd3dContext,ID3D11RenderTargetView* gNextStepMapRTV,ID3D11ShaderResourceView* gCurrStepMapSRV
	,ID3D11ShaderResourceView* gPrevStepMapSRV,ID3D11ShaderResourceView* tex_displacement);
void drawDis(ID3D11DeviceContext* pd3dContext,ID3D11Device* pd3dDevice,const CBaseCamera& camera,ID3D11ShaderResourceView* DisplacementSRV);
void drawFFtMap(ID3D11RenderTargetView* m_pFFtMapRTV,ID3D11DeviceContext* pd3dContext,ID3D11ShaderResourceView* displacemnet_map);
void draw1(ID3D11DeviceContext* pd3dContext,ID3D11Device* pd3dDevice,ID3D11ShaderResourceView* gNextStepMapSRV,const CBaseCamera& camera,ID3D11ShaderResourceView* gDisplaceMap,float fTime);
void Normal(ID3D11DeviceContext* pd3dContext,ID3D11RenderTargetView* gNormalMapRTV,ID3D11ShaderResourceView* gNormalMapSRV,ID3D11ShaderResourceView* gNextStepMapSRV,ID3D11ShaderResourceView* gDisplaceMapSRV);
void DrawReflect(ID3D11DeviceContext* pd3dContext);
// Returns random float in [0, 1).
/*inline float RandF()
{
	return (float)(rand()) / (float)RAND_MAX;
}

// Returns random float in [a, b).
inline float RandF(float a, float b)
{
	return a + RandF()*(b-a);
}*/
//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    HRESULT hr;
    V_RETURN(DXUTSetMediaSearchPath(L"..\\Source\\OceanCS"));
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Disable gamma correction on this sample
    DXUTSetIsInGammaCorrectMode( false );

    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );
    
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    InitApp();

    // Force create a ref device so that feature level D3D_FEATURE_LEVEL_11_0 is guaranteed
    DXUTInit( true, true ); // Parse the command line, show msgboxes on error, no extra command line params

    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"DirectCompute Ocean" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 1280, 720 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 32, iY, 140, 26 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 32, iY += 30, 140, 26, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 32, iY += 30, 140, 26, VK_F2 );
    g_HUD.AddButton( IDC_WIREFRAME, L"Toggle wireframe", 32, iY += 30, 140, 26, VK_F4 );
    g_HUD.AddButton( IDC_PAUSE, L"Pause", 32, iY += 30, 140, 26, VK_F5 );

    g_SampleUI.SetCallback( OnGUIEvent ); 

	g_Camera.SetRotateButtons( true, false, false );
	g_Camera.SetScalers(0.003f, 400.0f);

	
}

//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// DXUT will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    assert( pDeviceSettings->ver == DXUT_D3D11_DEVICE );
    
    // Disable vsync
    pDeviceSettings->d3d11.SyncInterval = 0;

	// Enable 4x MSAA
	pDeviceSettings->d3d11.sd.SampleDesc.Count = 4;

    return true;
}

//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext)
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );

	// Update simulation
	static double app_time = 0;
	if (g_PauseSimulation == false)
	{
		app_time += (double)fElapsedTime;
	//	static int count = 0;
	//	if(count == 0)
		g_pOceanSimulator->updateDisplacementMap((float)app_time,gCurrStepMapSRV);
	//	count++;
	}
//	drawdisturb(pd3dDevice, pd3dContext,gPrevStepMapRTV);
	particle->Update((float) fElapsedTime, (float) fTime);
}

void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
	LPCWSTR frame_stat = DXUTGetFrameStats( DXUTIsVsyncEnabled() );
	frame_stat += 6;
    g_pTxtHelper->DrawTextLine( frame_stat );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    g_pTxtHelper->End();
}

//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                         void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
    case IDC_TOGGLEFULLSCREEN:
        DXUTToggleFullScreen(); break;
    case IDC_TOGGLEREF:
        DXUTToggleREF(); break;
    case IDC_CHANGEDEVICE:
        g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;
	case IDC_WIREFRAME:
		g_RenderWireframe = !g_RenderWireframe;
		break;
	case IDC_PAUSE:
		if (g_PauseSimulation == false)
		{
			g_HUD.GetButton(IDC_PAUSE)->SetText(L"Resume");
			g_PauseSimulation = true;
		}
		else
		{
			g_HUD.GetButton(IDC_PAUSE)->SetText(L"Pause");
			g_PauseSimulation = false;
		}
		break;
    }

}

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, 
									   const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, 
									   bool bWindowed, void* pUserContext )
{
    return true;
}

//--------------------------------------------------------------------------------------
// Create an OceanSimulator object at startup time
//--------------------------------------------------------------------------------------
void CreateOceanSimAndRender(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
	// Create ocean simulating object
	// Ocean object
	OceanParameter ocean_param;

	// The size of displacement map. In this sample, it's fixed to 512.
	ocean_param.dmap_dim			= 512;
	// The side length (world space) of square patch
	ocean_param.patch_length		= 2000.0f;
	// Adjust this parameter to control the simulation speed
	ocean_param.time_scale			= 0.8f;
	// A scale to control the amplitude. Not the world space height
	ocean_param.wave_amplitude		= 0.35f;
	// 2D wind direction. No need to be normalized
	ocean_param.wind_dir			= D3DXVECTOR2(0.8f, 0.6f);
	// The bigger the wind speed, the larger scale of wave crest.
	// But the wave scale can be no larger than patch_length
	ocean_param.wind_speed			= 600.0f;
	// Damp out the components opposite to wind direction.
	// The smaller the value, the higher wind dependency
	ocean_param.wind_dependency		= 0.07f;
	// Control the scale of horizontal movement. Higher value creates
	// pointy crests.
	ocean_param.choppy_scale		= 1.3f;
/*	ocean_param.dmap_dim			= 512;
	// The side length (world space) of square patch
	ocean_param.patch_length		= 1500.0f;
	// Adjust this parameter to control the simulation speed
	ocean_param.time_scale			= 0.9f;
	// A scale to control the amplitude. Not the world space height
	ocean_param.wave_amplitude		= 0.35f;
	// 2D wind direction. No need to be normalized
	ocean_param.wind_dir			= D3DXVECTOR2(0.8f, 0.6f);
	// The bigger the wind speed, the larger scale of wave crest.
	// But the wave scale can be no larger than patch_length
	ocean_param.wind_speed			= 200.0f;
	// Damp out the components opposite to wind direction.
	// The smaller the value, the higher wind dependency
	ocean_param.wind_dependency		= 0.07f;
	// Control the scale of horizontal movement. Higher value creates
	// pointy crests.
	ocean_param.choppy_scale		= 0.8f;*/

	g_pOceanSimulator = new OceanSimulator(ocean_param, pd3dDevice);
	//rain = new Rain(pd3dDevice);
	//rain->init ();
	particle = new ParticleSystem(pd3dDevice);
	particle->Init();
	// Update the simulation for the first time.
	g_pOceanSimulator->updateDisplacementMap(0,gCurrStepMapSRV);

	
	
	
	// Init D3D11 resources for rendering
	initRenderResource(ocean_param, pd3dDevice, pBackBufferSurfaceDesc);
}

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnD3D11DestroyDevice callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;

	D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS options;
	hr = pd3dDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &options, sizeof(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS));
	if( !options.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x )
	{
		MessageBox(NULL, L"Compute Shaders are not supported on your hardware", L"Unsupported HW", MB_OK);
		return E_FAIL;
	}

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );

	// Initialize the font
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

	// Setup the camera's view parameters

	D3DXVECTOR3 vecEye(1562.24f, 854.291f, -1224.99f);
	D3DXVECTOR3 vecAt (1562.91f, 854.113f, -1225.71f);
//	D3DXVECTOR3 vecEye(0.0f, 2.0f, 80.0f);
//	D3DXVECTOR3 vecAt (10.91f,-50.113, -1220.71f);

//	 D3DXVECTOR3 vecEye = D3DXVECTOR3( 15.5f, 5.0f, 0.0f );
//	 D3DXVECTOR3 vecAt  = D3DXVECTOR3( 0.0f, 3.0f, 0.0f );
	g_Camera.SetViewParams(&vecEye, &vecAt);

	mSun.init(0.0,1.570796, 38);
	mAtm.init(0.2, 0.01, 0.3, 1, 0.81);
	// Create an OceanSimulator object and D3D11 rendering resources
	CreateOceanSimAndRender(pd3dDevice, pBackBufferSurfaceDesc);
	float dx = 2;
	float dt=0.03;
	float damping=7;
	float speed=30;
	float tex = 512;
	physics = new Physics(pd3dDevice,pd3dImmediateContext,damping,speed,dx,dt,tex);
	physics->init();
	gPrevStepMapRTV = physics->getgPrevStepMapRTV();
	gCurrStepMapRTV = physics->getgCurrStepMapRTV();
	gReflectMapRTV = physics->getgReflectMapRTV();
	gPrevStepMapSRV = physics->getgPrevStepMapSRV();
	gCurrStepMapSRV = physics->getgCurrStepMapSRV();
	gReflectMapSRV = physics->getgReflectMapSRV();
	// Sky box
 /*   WCHAR strPath[MAX_PATH];
    DXUTFindDXSDKMediaFileCch(strPath, MAX_PATH, L"Media\\OceanCS\\sky_cube1.dds");
	D3DX11CreateShaderResourceViewFromFile(pd3dDevice, strPath, NULL, NULL, &g_pSRV_SkyCube, NULL);
	assert(g_pSRV_SkyCube);

	g_pSRV_SkyCube->GetResource((ID3D11Resource**)&g_pSkyCubeMap);
	assert(g_pSkyCubeMap);
	*/
	g_Skybox.OnD3D11CreateDevice(pd3dDevice, 50, g_pSkyCubeMap, g_pSRV_SkyCube);
	sky = new Sky(pd3dDevice);
	sky->init(10000,5,5,thetaS, phiS, turbidity);

    return S_OK;
}

HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
//	g_Camera.SetProjParams(D3DX_PI/4, fAspectRatio, 100.0f, 200000.0f);
	g_Camera.SetProjParams(D3DX_PI/4, fAspectRatio, 10.0f, 30000.0f);
    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 180, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 350 );
    g_SampleUI.SetSize( 170, 300 );

	// Sky box
	g_Skybox.OnD3D11ResizedSwapChain(pBackBufferSurfaceDesc);

    return S_OK;
}

void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

	float ClearColor[4] = { 0.1f,0.2f,0.4f,0.0f };
	//float ClearColor[4] = { 0,0,0,0.0f };
	// Clear the main render target
	pd3dImmediateContext->ClearRenderTargetView( DXUTGetD3D11RenderTargetView(), ClearColor );
	// Clear the depth stencil
	pd3dImmediateContext->ClearDepthStencilView( DXUTGetD3D11DepthStencilView(), D3D10_CLEAR_DEPTH, 1.0, 0 );

	// Sky box rendering
	D3DXMATRIX W;
	D3DXMatrixTranslation(&W, 0.0f, 0.0f, -100.0f);
    D3DXMATRIXA16 mView = D3DXMATRIX(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1) * *g_Camera.GetViewMatrix();
    D3DXMATRIXA16 mProj = *g_Camera.GetProjMatrix();
    D3DXMATRIXA16 mWorldViewProjection1 = W*mView * mProj;
	D3DXMATRIXA16 mWorldViewProjection = mView * mProj;
	D3DXVECTOR3 a = *g_Camera.GetEyePt();
	
	/*D3DXPLANE waterPlaneW(0.0f, 1.0f, 0.0f, 0.0f);

	D3DXMATRIX RefMtx;
	D3DXMatrixIdentity(&RefMtx);
	if(waterPlaneW)
		
	{
	D3DXMatrixScaling(&RefMtx,1.0f,-1.0f,1.0f);
	}
	D3DXMATRIXA16 WVP = WVP * RefMtx;
	*/
	//if (!g_RenderWireframe)
	//    g_Skybox.D3D11Render( &mWorldViewProjection, pd3dImmediateContext );
	
	
	// Time
	static double app_time = fTime;
	static double app_prev_time = fTime;
	if (g_PauseSimulation == false)
		app_time += fTime - app_prev_time;
	app_prev_time = fTime;

	
	// Ocean rendering
/*	ID3D11ShaderResourceView* tex_displacement = g_pOceanSimulator->getD3D11DisplacementMap();
	ID3D11ShaderResourceView* tex_gradient = g_pOceanSimulator->getD3D11GradientMap();
	ID3D11ShaderResourceView* LastStepMap= g_pOceanSimulator->getD3D11gLastStepMap();
	
	ID3D11RenderTargetView* m_pCurrStepMapRTV = g_pOceanSimulator->getD3D11CurrStepMapRTV();//改动
	ID3D11RenderTargetView* m_pFFtMapRTV = g_pOceanSimulator->getD3D11gFFtMapRTV();//改动
	ID3D11ShaderResourceView* m_pFFtMapSRV = g_pOceanSimulator->getD3D11gFFtMap();//改动
	ID3D11RenderTargetView* m_pNormalRTV = g_pOceanSimulator->getD3D11gNormalMapRTV();
	ID3D11ShaderResourceView* m_pNormalMapSRV = g_pOceanSimulator->getD3D11gNormalMap();*/
	
//	gReflectMapRTV = g_pOceanSimulator->getD3D11gReflectMapRTV();
	//ID3D11ShaderResourceView* m_pNormalMapSRV = g_pOceanSimulator->getD3D11gNormalMap();
	static float timeElapsed = 0.0f;
	timeElapsed += (float)app_time;
	static float t_base = 0.0f;
//	srand(unsigned (time));

	

//	drawFFtMap(gPrevStepMapRTV,pd3dImmediateContext,tex_displacement);
		
	//	drawFFtMap(gCurrStepMapRTV,pd3dImmediateContext,tex_displacement);
	if( (timeElapsed - t_base) >= app_time )
		{
			t_base += 0.025f;
			for(int k=0;k<100;k++)
			{
				float i = RandF(-1.0f, 1.0f);
				float j = RandF(-1.0f, 1.0f);
		//		drawdisturb(i,j,10,pd3dDevice, pd3dImmediateContext, gPrevStepMapRTV);
		//		drawdisturb(i,j,10,pd3dDevice, pd3dImmediateContext, gCurrStepMapRTV);
			}
		}
		
	

	static int count= 0;
//	next_norm(pd3dDevice,pd3dImmediateContext,gNextStepMapRTV,gCurrStepMapSRV,gPrevStepMapSRV,tex_displacement);//写入gNextStepMapRTV
	
//	ID3D11ShaderResourceView* LastMapSRV;
//	LastStepMap = gPrevStepMapSRV;
//	g_pOceanSimulator->updatePrevMap();

/*	ID3D11ShaderResourceView* temp;
	temp = gPrevStepMapSRV;
	gPrevStepMapSRV = gCurrStepMapSRV;
	gCurrStepMapSRV = gNextStepMapSRV;
	gNextStepMapSRV = temp;
	
	ID3D11RenderTargetView* temp1;
	temp1 = gPrevStepMapRTV;
	gPrevStepMapRTV = gCurrStepMapRTV;
	gCurrStepMapRTV = gNextStepMapRTV;
	gNextStepMapRTV = temp1;*/

//	ID3D11ShaderResourceView* m_pCurrStepMapSRV = g_pOceanSimulator->getD3D11CurrStepMap();//改动
	ID3D11ShaderResourceView* tex_displacement = g_pOceanSimulator->getD3D11DisplacementMap();
	ID3D11ShaderResourceView* tex_gradient = g_pOceanSimulator->getD3D11GradientMap();
//	}
	DrawReflect(pd3dImmediateContext);
	if (g_RenderWireframe)
	//	
	{
//		renderWireframe(g_Camera, tex_displacement, (float)app_time, pd3dImmediateContext,gCurrStepMapSRV);
	//	next_norm(pd3dDevice,pd3dImmediateContext,gNextStepMapRTV,gCurrStepMapSRV,gPrevStepMapSRV);//写入gNextStepMapRTV,m_pNormalRTV
	//	drawFFtMap(gPrevStepMapRTV,pd3dImmediateContext,tex_displacement);
	//	draw1(pd3dImmediateContext,pd3dDevice,gCurrStepMapSRV,g_Camera,tex_displacement,fTime);//根据gNextStepMapSRV模拟
		
	//	showmap(pd3dImmediateContext,pd3dDevice, gPrevStepMapSRV);
	}
	else
	{
//		Normal(pd3dImmediateContext, m_pNormalRTV,m_pNormalMapSRV,gCurrStepMapSRV,tex_displacement);
	//	renderShaded(g_Camera, tex_displacement, tex_gradient, (float)app_time, pd3dImmediateContext);
//		DrawReflect(pd3dImmediateContext);
//		draw(pd3dImmediateContext,pd3dDevice,m_pNormalMapSRV,gCurrStepMapSRV,gReflectMapSRV,g_Camera,tex_gradient,tex_displacement,fTime);//根据gNextStepMapSRV模拟
	//	ID3D11ShaderResourceView* m_pCurrStepMapSRV = g_pOceanSimulator->getD3D11CurrStepMap();//改动
	//	Normal(pd3dImmediateContext, m_pNormalRTV,m_pNormalMapSRV,gNextStepMapSRV);
		renderShaded(g_Camera, tex_displacement, tex_gradient, (float)app_time, pd3dImmediateContext,gReflectMapSRV,&mAtm,&mSun,pd3dDevice);
	//	gNextStepMapSRV = g_pOceanSimulator->getD3D11gNextStepMap();
	//	
	//	drawDis(pd3dImmediateContext,pd3dDevice,g_Camera,tex_displacement);
		
	}
	//sky->draw (&mWorldViewProjection,a);
	sky->DrawScatter(&mAtm,&mSun,&mWorldViewProjection1,a);
	//rain->Draw (mWorldViewProjection,a);
	float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	particle->Draw(a,mWorldViewProjection,fElapsedTime);
//	pd3dImmediateContext->RSSetState(0);
//	pd3dImmediateContext->OMSetDepthStencilState(0, 0);
	pd3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
//	gReflectMapSRV = g_pOceanSimulator->getD3D11gReflectMap();
//	showmap(pd3dImmediateContext,pd3dDevice,gReflectMapSRV);
//	*/
	// HUD rendering
	g_HUD.OnRender( fElapsedTime ); 
	g_SampleUI.OnRender( fElapsedTime );
	
	RenderText();
}

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnD3D11CreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	cleanupRenderResource();

    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    // Ocean object
	SAFE_DELETE(g_pOceanSimulator);

	SAFE_RELEASE(mNDCQuadVB);
	SAFE_RELEASE(g_pVS);
	SAFE_RELEASE(g_pPS);
	SAFE_RELEASE(PosTexlayout);
	SAFE_RELEASE(g_pCubeSampler1);
	// Sky box
    g_Skybox.OnD3D11DestroyDevice();
}

void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();

    g_Skybox.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// 改动
//--------------------------------------------------------------------------------------
void buildNDCQuad(ID3D11Device* pd3dDevice)
{
	D3DXVECTOR3 pos[] = 
	{
		D3DXVECTOR3(0.0f, -1.0f, 0.0f),
		D3DXVECTOR3(0.0f,  0.0f, 0.0f),
		D3DXVECTOR3(1.0f,  0.0f, 0.0f),

		D3DXVECTOR3(0.0f, -1.0f, 0.0f),
		D3DXVECTOR3(1.0f,  0.0f, 0.0f),
		D3DXVECTOR3(1.0f, -1.0f, 0.0f)

	};

	D3DXVECTOR2 tex[] = 
	{
		D3DXVECTOR2(0.0f, 1.0f),
		D3DXVECTOR2(0.0f, 0.0f),
		D3DXVECTOR2(1.0f, 0.0f),

		D3DXVECTOR2(0.0f, 1.0f),
		D3DXVECTOR2(1.0f, 0.0f),
		D3DXVECTOR2(1.0f, 1.0f)
	};

	QuadVertex qv[6];

	for(int i = 0; i < 6; ++i)
	{
		qv[i].pos  = pos[i];
		qv[i].texC = tex[i];
	}

	D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(QuadVertex) * 6;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = qv;
    pd3dDevice->CreateBuffer(&vbd, &vinitData, &mNDCQuadVB);
	
}
void init_showmap( ID3D11Device* pd3dDevice)
{
	buildNDCQuad(pd3dDevice);
	ID3DBlob* pBlobVS1 = NULL;
	ID3DBlob* pBlobPS1 = NULL;
	CompileShaderFromFile(L"showmap.hlsl", "VS", "vs_4_0", &pBlobVS1);
	CompileShaderFromFile(L"showmap.hlsl", "PS", "ps_4_0", &pBlobPS1);
	assert(pBlobVS1);
	assert(pBlobPS1);
	pd3dDevice->CreateVertexShader(pBlobVS1->GetBufferPointer(), pBlobVS1->GetBufferSize(), NULL, &g_pVS);
	pd3dDevice->CreatePixelShader(pBlobPS1->GetBufferPointer(), pBlobPS1->GetBufferSize(), NULL, &g_pPS);
	assert(g_pVS);
	assert(g_pPS);
	D3D11_INPUT_ELEMENT_DESC quad_desc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	pd3dDevice->CreateInputLayout(quad_desc,2, pBlobVS1->GetBufferPointer(), pBlobVS1->GetBufferSize(), &PosTexlayout);

	
	SAFE_RELEASE(pBlobVS1);
	SAFE_RELEASE(pBlobPS1);
	assert(PosTexlayout);
}
void showmap(ID3D11DeviceContext* pd3dImmediateContext,ID3D11Device* pd3dDevice,ID3D11ShaderResourceView* m_pCurrStepMapSRV)
{
	init_showmap(pd3dDevice);
	
	ID3D11ShaderResourceView* ps_srvs[1]={m_pCurrStepMapSRV};
    pd3dImmediateContext->PSSetShaderResources(0, 1, ps_srvs);

	// Samplers
	D3D11_SAMPLER_DESC sam_desc;
	sam_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
//	sam_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
//	sam_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
//	sam_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sam_desc.MipLODBias = 0;
	sam_desc.MaxAnisotropy = 1;
	sam_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sam_desc.BorderColor[0] = 1.0f;
	sam_desc.BorderColor[1] = 1.0f;
	sam_desc.BorderColor[2] = 1.0f;
	sam_desc.BorderColor[3] = 1.0f;
	sam_desc.MinLOD = 0;
	sam_desc.MaxLOD = FLT_MAX;
	sam_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sam_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sam_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sam_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	pd3dDevice->CreateSamplerState(&sam_desc, &g_pCubeSampler1);
	assert(g_pCubeSampler1);

	ID3D11SamplerState* ps_samplers[1] = {g_pCubeSampler1};
	pd3dImmediateContext->PSSetSamplers(0, 1, &ps_samplers[0]);

	//init_tex(pd3dDevice);
	UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    pd3dImmediateContext->IASetVertexBuffers(0, 1, &mNDCQuadVB, &stride, &offset);
	pd3dImmediateContext->IASetInputLayout(PosTexlayout);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dImmediateContext->VSSetShader( g_pVS, NULL, 0 );
    pd3dImmediateContext->PSSetShader( g_pPS, NULL, 0 );

	pd3dImmediateContext->Draw(6, 0);
}

void DrawReflect(ID3D11DeviceContext* pd3dContext)
{
	ID3D11RenderTargetView* old_target;
	ID3D11DepthStencilView* old_depth;
	pd3dContext->OMGetRenderTargets(1, &old_target, &old_depth); 

	D3D11_VIEWPORT old_viewport;
	UINT num_viewport = 1;
	pd3dContext->RSGetViewports(&num_viewport, &old_viewport);

	D3D11_VIEWPORT new_vp = {0, 0, (float)512, (float)512, 0.0f, 1.0f};
	pd3dContext->RSSetViewports(1, &new_vp);

	// Set RT
	ID3D11RenderTargetView* rt_views[1] = {gReflectMapRTV};
	pd3dContext->OMSetRenderTargets(1, rt_views, NULL);
//	pd3dContext->ClearRenderTargetView(gReflectMapRTV,black);
//	pd3dContext->ClearDepthStencilView();
	// Sky box rendering
    D3DXMATRIXA16 mView = D3DXMATRIX(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1) * *g_Camera.GetViewMatrix();
    D3DXMATRIXA16 mProj = *g_Camera.GetProjMatrix();
    D3DXMATRIXA16 mWorldViewProjection = mView * mProj;
	D3DXVECTOR3 a = *g_Camera.GetEyePt();
	D3DXMATRIXA16 mView1 = D3DXMATRIX(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1) * *g_Camera.GetViewMatrix();
	D3DXMATRIX W;
	D3DXMatrixTranslation(&W, 0.0f, 0.0f, -100.0f);
    D3DXMATRIXA16 mProj1 = *g_Camera.GetProjMatrix();
    D3DXMATRIXA16 mWorldViewProjection1 = W*mView1 * mProj1;
	D3DXVECTOR3 a1 = *g_Camera.GetEyePt();
	D3DXPLANE waterPlaneW(0.0f, 0.0f, 0.0f, 0.0f);

	
	D3DXMATRIX RefMtx;
	D3DXMatrixIdentity(&RefMtx);
	if(waterPlaneW)	
	{
	D3DXMatrixScaling(&RefMtx,1.0f,1.0f,-1.0f);//因为本框架是y与z是反的，所以镜像镜错了,不对，不是这个问题
	}
	D3DXMATRIXA16 WVP = mWorldViewProjection1 * RefMtx ;
	
  //  g_Skybox.D3D11Render( &WVP, pd3dContext );
	sky->DrawScatter(&mAtm,&mSun,/*&mWorldViewProjection1*/&WVP,a1);


	pd3dContext->RSSetViewports(1, &old_viewport);
	pd3dContext->OMSetRenderTargets(1, &old_target, old_depth);
	SAFE_RELEASE(old_target);
	SAFE_RELEASE(old_depth);

	pd3dContext->GenerateMips(gReflectMapSRV);
}