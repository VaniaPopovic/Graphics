//--------------------------------------------------------------------------------------
//	GraphicsAssign1.cpp
//
//	Shaders Graphics Assignment
//	Add further models using different shader techniques
//	See assignment specification for details
//--------------------------------------------------------------------------------------

//***|  INFO  |****************************************************************
// Lights:
//   The initial project shows models for a couple of point lights, but the
//   initial shaders don't actually apply any lighting. Part of the assignment
//   is to add a shader that uses this information to actually light a model.
//   Refer to lab work to determine how best to do this.
// 
// Textures:
//   The models in the initial scene have textures loaded but the initial
//   technique/shaders doesn't show them. Part of the assignment is to add 
//   techniques to show a textured model
//
// Shaders:
//   The initial shaders render plain coloured objects with no lighting:
//   - The vertex shader performs basic transformation of 3D vertices to 2D
//   - The pixel shader colours every pixel the same colour
//   A few shader variables are set up to communicate between C++ and HLSL
//   You will usually need to add further variables to add new techniques
//*****************************************************************************

#include <windows.h>
#include <d3d10.h>
#include <d3dx10.h>
#include <atlbase.h>
#include "resource.h"

#include "Defines.h" // General definitions shared by all source files // Model class - encapsulates working with vertex/index data and world matrix
#include "Light.h"
#include "Camera.h"  // Camera class - encapsulates the camera's view and projection matrix
#include "Input.h"   // Input functions - not DirectX

#define NUM_OF_POINT_LIGHTS 4
#define NUM_OF_SPOT_LIGTHS 3
#define NUM_OF_MODELS 6
//--------------------------------------------------------------------------------------
// Global Scene Variables
//--------------------------------------------------------------------------------------

// Models and cameras encapsulated in classes for flexibity and convenience
// The CModel class collects together geometry and world matrix, and provides functions to control the model and render it
// The CCamera class handles the view and projections matrice, and provides functions to control the camera
CModel* Cube;
CModel* Floor;
CModel* Sphere;
CModel* TeaPot;
CCamera* Camera;
CModel* Troll;
Light* SpotLights[3];
CModel* Models[6];
Light* PointLights[2];

// Textures - no texture class yet so using DirectX variables
ID3D10ShaderResourceView* CubeDiffuseMap = NULL;
ID3D10ShaderResourceView* CubeNormalMap = NULL;
ID3D10ShaderResourceView* FloorDiffuseMap = NULL;
ID3D10ShaderResourceView* FloorNormalMap = NULL;
ID3D10ShaderResourceView* SphereDiffuseMap = NULL;
ID3D10ShaderResourceView* TeapotDiffuseMap = NULL;
ID3D10ShaderResourceView* TeapotNormalMap = NULL;
ID3D10ShaderResourceView* TrollDiffuseMap = NULL;
ID3D10ShaderResourceView* CellMap = NULL;



// Cell shading data
D3DXVECTOR3 OutlineColour = D3DXVECTOR3(0, 0, 0); // Black outlines
float       OutlineThickness = 0.015f;


float ParallaxDepth = 0.08f; // Overall depth of bumpiness for parallax mapping
bool UseParallax = true;  // Toggle for parallax 
// Light data - stored manually as there is no light class

D3DXVECTOR3 AmbientColour = D3DXVECTOR3( 0.2f, 0.2f, 0.2f );
float SpecularPower = 256.0f;

// Display models where the lights are. One of the lights will follow an orbit
Light* Light1;
Light* Light2;
Light* AmbientLight;

const float LightOrbitRadius = 20.0f;
const float LightOrbitSpeed  = 0.5f;
// Note: There are move & rotation speed constants in Defines.h



//--------------------------------------------------------------------------------------
// Shader Variables
//--------------------------------------------------------------------------------------
// Variables to connect C++ code to HLSL shaders

// Effects / techniques
ID3D10Effect*          Effect = NULL;
ID3D10EffectTechnique* PlainColourTechnique = NULL;
ID3D10EffectTechnique* DiffuseTextureTechnique = NULL;
ID3D10EffectTechnique* ParallaxMappingTechnique = NULL;
ID3D10EffectTechnique* VertexLitDiffuseTechnique = NULL;
ID3D10EffectTechnique* test = NULL;

// Matrices
ID3D10EffectMatrixVariable* WorldMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* ProjMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewProjMatrixVar = NULL;

// Textures - two textures in the pixel shader now - diffuse/specular map and normal/depth map
ID3D10EffectShaderResourceVariable* DiffuseMapVar = NULL;
ID3D10EffectShaderResourceVariable* NormalMapVar = NULL;

ID3D10EffectVectorVariable* CameraPosVar = NULL;
ID3D10EffectVectorVariable* Light1PosVar = NULL;
ID3D10EffectVectorVariable* Light1ColourVar = NULL;
ID3D10EffectVectorVariable* Light2PosVar = NULL;
ID3D10EffectScalarVariable* numOfSpotLights = NULL;
ID3D10EffectVectorVariable* SpotlightPos=NULL;
ID3D10EffectVectorVariable* SpotlightDirections = NULL;
ID3D10EffectVectorVariable* SpotlightColours= NULL;
ID3D10EffectScalarVariable* SpotlightAngles = NULL;
ID3D10EffectScalarVariable* SpotlightIntensities = NULL;
ID3D10EffectVectorVariable* PointlightPos = NULL;
ID3D10EffectVectorVariable* PointlightColors = NULL;

ID3D10EffectVectorVariable* Light2ColourVar = NULL;
ID3D10EffectVectorVariable* Light3PosVar = NULL;
ID3D10EffectVectorVariable* Light3ColourVar = NULL;
ID3D10EffectVectorVariable* AmbientColourVar = NULL;
ID3D10EffectScalarVariable* SpecularPowerVar = NULL;

// Other 
ID3D10EffectScalarVariable* ParallaxDepthVar = NULL;
ID3D10EffectVectorVariable* TintColourVar = NULL;
// Miscellaneous
ID3D10EffectVectorVariable* ModelColourVar = NULL;


//--------------------------------------------------------------------------------------
// DirectX Variables
//--------------------------------------------------------------------------------------

// The main D3D interface, this pointer is used to access most D3D functions (and is shared across all cpp files through Defines.h)
ID3D10Device* g_pd3dDevice = NULL;

// Width and height of the window viewport
int g_ViewportWidth;
int g_ViewportHeight;

// Variables used to setup D3D
IDXGISwapChain*         SwapChain = NULL;
ID3D10Texture2D*        DepthStencil = NULL;
ID3D10DepthStencilView* DepthStencilView = NULL;
ID3D10RenderTargetView* RenderTargetView = NULL;


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
bool InitDevice(HWND hWnd)
{
	// Many DirectX functions return a "HRESULT" variable to indicate success or failure. Microsoft code often uses
	// the FAILED macro to test this variable, you'll see it throughout the code - it's fairly self explanatory.
	HRESULT hr = S_OK;


	////////////////////////////////
	// Initialise Direct3D

	// Calculate the visible area the window we are using - the "client rectangle" refered to in the first function is the 
	// size of the interior of the window, i.e. excluding the frame and title
	RECT rc;
	GetClientRect(hWnd, &rc);
	g_ViewportWidth = rc.right - rc.left;
	g_ViewportHeight = rc.bottom - rc.top;


	// Create a Direct3D device (i.e. initialise D3D), and create a swap-chain (create a back buffer to render to)
	DXGI_SWAP_CHAIN_DESC sd;         // Structure to contain all the information needed
	ZeroMemory( &sd, sizeof( sd ) ); // Clear the structure to 0 - common Microsoft practice, not really good style
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_ViewportWidth;             // Target window size
	sd.BufferDesc.Height = g_ViewportHeight;           // --"--
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Pixel format of target window
	sd.BufferDesc.RefreshRate.Numerator = 60;          // Refresh rate of monitor
	sd.BufferDesc.RefreshRate.Denominator = 1;         // --"--
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.OutputWindow = hWnd;                            // Target window
	sd.Windowed = TRUE;                                // Whether to render in a window (TRUE) or go fullscreen (FALSE)
	hr = D3D10CreateDeviceAndSwapChain( NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
										D3D10_SDK_VERSION, &sd, &SwapChain, &g_pd3dDevice );
	if( FAILED( hr ) ) return false;


	// Specify the render target as the back-buffer - this is an advanced topic. This code almost always occurs in the standard D3D setup
	ID3D10Texture2D* pBackBuffer;
	hr = SwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBackBuffer );
	if( FAILED( hr ) ) return false;
	hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &RenderTargetView );
	pBackBuffer->Release();
	if( FAILED( hr ) ) return false;


	// Create a texture (bitmap) to use for a depth buffer
	D3D10_TEXTURE2D_DESC descDepth;
	descDepth.Width = g_ViewportWidth;
	descDepth.Height = g_ViewportHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D10_USAGE_DEFAULT;
	descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &DepthStencil );
	if( FAILED( hr ) ) return false;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView( DepthStencil, &descDSV, &DepthStencilView );
	if( FAILED( hr ) ) return false;

	// Select the back buffer and depth buffer to use for rendering now
	g_pd3dDevice->OMSetRenderTargets( 1, &RenderTargetView, DepthStencilView );


	// Setup the viewport - defines which part of the window we will render to, almost always the whole window
	D3D10_VIEWPORT vp;
	vp.Width  = g_ViewportWidth;
	vp.Height = g_ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );

	return true;
}


// Release the memory held by all objects created
void ReleaseResources()
{
	// The D3D setup and preparation of the geometry created several objects that use up memory (e.g. textures, vertex/index buffers etc.)
	// Each object that allocates memory (or hardware resources) needs to be "released" when we exit the program
	// There is similar code in every D3D program, but the list of objects that need to be released depends on what was created
	// Test each variable to see if it exists before deletion
	if( g_pd3dDevice )     g_pd3dDevice->ClearState();
	for (int i = 0; i < NUM_OF_SPOT_LIGTHS; ++i) delete SpotLights[i];
	delete Light2;
	delete Light1;
	delete PointLights[0];
	delete PointLights[1];
	delete Floor;
	delete Cube;
	delete Camera;
	delete Sphere;
	delete TeaPot;

    if( FloorDiffuseMap )  FloorDiffuseMap->Release();
	if (FloorNormalMap)   FloorNormalMap->Release();
	if (CubeNormalMap)    CubeNormalMap->Release();
	if (CubeDiffuseMap)   CubeDiffuseMap->Release();
	if (TeapotNormalMap)  TeapotNormalMap->Release();
	if (TeapotDiffuseMap) TeapotDiffuseMap->Release();
	if( Effect )           Effect->Release();
	if( DepthStencilView ) DepthStencilView->Release();
	if( RenderTargetView ) RenderTargetView->Release();
	if( DepthStencil )     DepthStencil->Release();
	if( SwapChain )        SwapChain->Release();
	if( g_pd3dDevice )     g_pd3dDevice->Release();

}



//--------------------------------------------------------------------------------------
// Load and compile Effect file (.fx file containing shaders)
//--------------------------------------------------------------------------------------
// An effect file contains a set of "Techniques". A technique is a combination of vertex, geometry and pixel shaders (and some states) used for
// rendering in a particular way. We load the effect file at runtime (it's written in HLSL and has the extension ".fx"). The effect code is compiled
// *at runtime* into low-level GPU language. When rendering a particular model we specify which technique from the effect file that it will use
//
bool LoadEffectFile()
{
	ID3D10Blob* pErrors; // This strangely typed variable collects any errors when compiling the effect file
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS; // These "flags" are used to set the compiler options

	// Load and compile the effect file
	HRESULT hr = D3DX10CreateEffectFromFile( L"GraphicsAssign1.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3dDevice, NULL, NULL, &Effect, &pErrors, NULL );
	if( FAILED( hr ) )
	{
		if (pErrors != 0)  MessageBox( NULL, CA2CT(reinterpret_cast<char*>(pErrors->GetBufferPointer())), L"Error", MB_OK ); // Compiler error: display error message
		else               MessageBox( NULL, L"Error loading FX file. Ensure your FX file is in the same folder as this executable.", L"Error", MB_OK );  // No error message - probably file not found
		return false;
	}

	// Now we can select techniques from the compiled effect file
	PlainColourTechnique = Effect->GetTechniqueByName( "PlainColour" );
	DiffuseTextureTechnique =Effect->GetTechniqueByName("DiffuseTex");
	ParallaxMappingTechnique = Effect->GetTechniqueByName("ParallaxMapping");
	VertexLitDiffuseTechnique = Effect->GetTechniqueByName("VertexLitTex");
	test = Effect->GetTechniqueByName("PixelShaderFunctionWithTex");
	// Create special variables to allow us to access global variables in the shaders from C++
	WorldMatrixVar    = Effect->GetVariableByName( "WorldMatrix" )->AsMatrix();
	ViewMatrixVar     = Effect->GetVariableByName( "ViewMatrix"  )->AsMatrix();
	ProjMatrixVar     = Effect->GetVariableByName( "ProjMatrix"  )->AsMatrix();

	

	// Also access shader variables needed for lighting
	CameraPosVar = Effect->GetVariableByName("CameraPos")->AsVector();
	Light1PosVar = Effect->GetVariableByName("Light1Pos")->AsVector();
	PointlightPos = Effect->GetVariableByName("Light1Pos")->AsVector();
	PointlightPos = Effect->GetVariableByName("Light1Colour")->AsVector();
	Light1ColourVar = Effect->GetVariableByName("Light1Colour")->AsVector();
	Light2PosVar = Effect->GetVariableByName("Light2Pos")->AsVector();
	Light2ColourVar = Effect->GetVariableByName("Light2Colour")->AsVector();
	Light3PosVar = Effect->GetVariableByName("Light3Pos")->AsVector();
	Light3ColourVar = Effect->GetVariableByName("Light3Colour")->AsVector();
	AmbientColourVar = Effect->GetVariableByName("AmbientColour")->AsVector();
	SpecularPowerVar = Effect->GetVariableByName("SpecularPower")->AsScalar();
	numOfSpotLights = Effect->GetVariableByName("NumberOfSpotLights")->AsScalar();
	SpotlightPos = Effect->GetVariableByName("SpotLightPositions")->AsVector();
	SpotlightColours = Effect->GetVariableByName("SpotLightColours")->AsVector();
	SpotlightIntensities = Effect->GetVariableByName("SpotLightIntensities")->AsScalar();
	SpotlightDirections = Effect->GetVariableByName("SpotLightDirections")->AsVector();
	SpotlightAngles = Effect->GetVariableByName("SpotLightAngles")->AsScalar();
	


	// We access the texture variable in the shader in the same way as we have before for matrices, light data etc.
	// Only difference is that this variable is a "Shader Resource"
	DiffuseMapVar = Effect->GetVariableByName("DiffuseMap")->AsShaderResource();
	NormalMapVar = Effect->GetVariableByName("NormalMap")->AsShaderResource();

	// Other shader variables
	ModelColourVar = Effect->GetVariableByName( "ModelColour"  )->AsVector();
	ParallaxDepthVar = Effect->GetVariableByName("ParallaxDepth")->AsScalar();
	TintColourVar = Effect->GetVariableByName("TintColour")->AsVector();

	return true;
}



//--------------------------------------------------------------------------------------
// Scene Setup / Update / Rendering
//--------------------------------------------------------------------------------------

// Create / load the camera, models and textures for the scene
bool InitScene()
{
	//////////////////
	// Create camera

	Camera = new CCamera();
	Camera->SetPosition( D3DXVECTOR3(-15, 20,-40) );
	Camera->SetRotation( D3DXVECTOR3(ToRadians(13.0f), ToRadians(18.0f), 0.0f) ); // ToRadians is a new helper function to convert degrees to radians


	///////////////////////
	// Load/Create models

	Cube = new CModel;
	Sphere = new CModel;
	TeaPot = new CModel;
	Floor = new CModel;
	Light1 = new Light;
	Light2 = new Light;

	SpotLights[0] = new Light;
	SpotLights[1] = new Light;
	SpotLights[2] = new Light;

	PointLights[0] = new Light;


	// The model class can load ".X" files. It encapsulates (i.e. hides away from this code) the file loading/parsing and creation of vertex/index buffers
	// We must pass an example technique used for each model. We can then only render models with techniques that uses matching vertex input data
	if (!Cube->Load("Cube.x", ParallaxMappingTechnique, true)) return false;
	if (!Floor->Load("Floor.x", VertexLitDiffuseTechnique)) return false;
	if (!TeaPot->Load("Teapot.x", ParallaxMappingTechnique, true)) return false;
	if (!Sphere->Load("Sphere.x", VertexLitDiffuseTechnique)) return false;
	if (!Light1->Load( "Sphere.x", PlainColourTechnique )) return false;
	if (!Light2->Load( "Sphere.x", PlainColourTechnique )) return false;
	if (!SpotLights[0]->Load("Sphere.x", PlainColourTechnique)) return false;
	if (!SpotLights[1]->Load("Sphere.x", PlainColourTechnique)) return false;
	if (!SpotLights[2]->Load("Sphere.x", PlainColourTechnique)) return false;
	if (!PointLights[0]->Load("Sphere.x", PlainColourTechnique)) return false;

	
	D3DXVECTOR3 Light1Colour = D3DXVECTOR3(1.0f, 0.0f, 0.7f) * 15;
	D3DXVECTOR3 Light2Colour = D3DXVECTOR3(1.0f, 0.8f, 0.2f) * 6;
	D3DXVECTOR3 SpotLightColour = D3DXVECTOR3(0.3f, 0.3f, 0.3f) * 6;
	// Initial positions
	Cube->SetPosition( D3DXVECTOR3(0, 10, 0) );
	Sphere->SetPosition( D3DXVECTOR3(25,10,10) );
	TeaPot->SetPosition(D3DXVECTOR3(100, 10, 100));
	Light1->SetPosition( D3DXVECTOR3(30, 10, 0) );
	Light1->SetScale( 0.1f ); // Nice if size of light reflects its brightness
	Light2->SetPosition( D3DXVECTOR3(-20, 30, 50) );
	Light2->SetScale( 0.2f );
	Light1->m_diffuse_colour(Light1Colour);
	Light2->m_diffuse_colour(Light2Colour);


	PointLights[0]->SetPosition(D3DXVECTOR3(50, 10, 0));
	PointLights[0]->m_diffuse_colour(Light1Colour);
	SpotLights[0]->SetPosition(D3DXVECTOR3(0, 30, 0));
	SpotLights[1]->SetPosition(D3DXVECTOR3(40, 30, 0));
	SpotLights[2]->SetPosition(D3DXVECTOR3(0, 30, 80));

	SpotLights[0]->m_diffuse_colour(D3DXVECTOR3(1.0f, 0.0f, 0.7f));
	SpotLights[1]->m_diffuse_colour(D3DXVECTOR3(0.5f, 0.0f, 0.7f));
	SpotLights[2]->m_diffuse_colour(D3DXVECTOR3(0.7f, 0.6f, 0.7f));

	
	
	//////////////////
	// Load textures
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"TechDiffuseSpecular.dds", NULL, NULL, &CubeDiffuseMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"TechNormalDepth.dds", NULL, NULL, &CubeNormalMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"PatternDiffuseSpecular.dds", NULL, NULL, &TeapotDiffuseMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"PatternNormalDepth.dds", NULL, NULL, &TeapotNormalMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"WoodDiffuseSpecular.dds", NULL, NULL, &FloorDiffuseMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"CobbleNormalDepth.dds", NULL, NULL, &FloorNormalMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"StoneDiffuseSpecular.dds", NULL, NULL, &SphereDiffuseMap, NULL))) return false;
	//if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"flare.jpg", NULL, NULL, &LightDiffuseMap, NULL))) return false;

	return true;
}


// Update the scene - move/rotate each model and the camera, then update their matrices
void UpdateScene( float frameTime )
{
	// Control camera position and update its matrices (view matrix, projection matrix) each frame
	// Don't be deceived into thinking that this is a new method to control models - the same code we used previously is in the camera class
	Camera->Control( frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );
	Camera->UpdateMatrices();
	
	// Control cube position and update its world matrix each frame
	Cube->Control( frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma );


	Cube->UpdateMatrix();
	Sphere->UpdateMatrix();
	TeaPot->UpdateMatrix();
	SpotLights[0]-> UpdateMatrix();
	SpotLights[1]->UpdateMatrix();
	SpotLights[2]->UpdateMatrix();
	// Update the orbiting light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float Rotate = 0.0f;
	Light1->SetPosition( Cube->GetPosition() + D3DXVECTOR3(cos(Rotate)*LightOrbitRadius, 0, sin(Rotate)*LightOrbitRadius) );
	Rotate -= LightOrbitSpeed * frameTime;
	Light1->UpdateMatrix();
	PointLights[0]->UpdateMatrix();
//	SpotLight->UpdateMatrix();
	// Second light doesn't move, but do need to make sure its matrix has been calculated - could do this in InitScene instead
	Light2->UpdateMatrix();
	if (KeyHit(Key_1))
	{
		UseParallax = !UseParallax;
	}
}


// Render everything in the scene
void RenderScene()
{
	// Clear the back buffer - before drawing the geometry clear the entire window to a fixed colour
	float ClearColor[4] = { 0.2f, 0.2f, 0.3f, 1.0f }; // Good idea to match background to ambient colour
	g_pd3dDevice->ClearRenderTargetView( RenderTargetView, ClearColor );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 ); // Clear the depth buffer too


	//---------------------------
	// Common rendering settings

	// Common features for all models, set these once only

	// Pass the camera's matrices to the vertex shader
	ViewMatrixVar->SetMatrix( (float*)&Camera->GetViewMatrix() );
	ProjMatrixVar->SetMatrix( (float*)&Camera->GetProjectionMatrix() );

	// Pass light information to the vertex shader - lights are the same for each model
	Light1PosVar->SetRawValue(Light1->GetPosition(), 0, 12);  // Send 3 floats (12 bytes) from C++ LightPos variable (x,y,z) to shader counterpart (middle parameter is unused) 
	Light1ColourVar->SetRawValue(Light1->m_diffuse_colour(), 0, 12);
	Light2PosVar->SetRawValue(Light2->GetPosition(), 0, 12);
	Light2ColourVar->SetRawValue(Light2->m_diffuse_colour(), 0, 12);
	AmbientColourVar->SetRawValue(AmbientColour, 0, 12);
	CameraPosVar->SetRawValue(Camera->GetPosition(), 0, 12);
	SpecularPowerVar->SetFloat(SpecularPower);
	


	D3DXVECTOR3 SpotLightPositions[3];

	SpotLightPositions[0] = SpotLights[0]->GetPosition();
	SpotLightPositions[1] = SpotLights[1]->GetPosition();
	SpotLightPositions[2] = SpotLights[2]->GetPosition();
	D3DXVECTOR3 SpotLightColours[3];

	SpotLightColours[0] = SpotLights[0]->m_diffuse_colour();
	SpotLightColours[1] = SpotLights[1]->m_diffuse_colour();
	SpotLightColours[2] = SpotLights[2]->m_diffuse_colour();

	D3DXVECTOR3 SpotLightDirections[3];

	SpotLightDirections[0] = SpotLights[0]->GetFacing();
	SpotLightDirections[1] = SpotLights[1]->GetFacing();
	SpotLightDirections[2] = SpotLights[2]-> GetFacing();

	float SpotLightAngles[3];

	SpotLightAngles[0] = 90.0f;
	SpotLightAngles[1] = 90.0f;
	SpotLightAngles[2] = 90.0f;

	float Intensities[3];
	Intensities[0] = 10.0f;
	Intensities[1] = 10.0f;
	Intensities[2] = 10.0f;

	SpotlightPos->SetFloatVectorArray((float*)SpotLightPositions, 0, 3);
	SpotlightColours->SetFloatVectorArray((float*)SpotLightColours, 0, 3);
	SpotlightDirections->SetFloatVectorArray((float*)SpotLightDirections, 0, 3);
	SpotlightAngles->SetFloatArray((float*)SpotLightAngles, 0, 3);
	SpotlightIntensities->SetFloatArray((float*)SpotlightIntensities, 0, 3);

	// Parallax mapping depth
	ParallaxDepthVar->SetFloat(UseParallax ? ParallaxDepth : 0.0f);
	//---------------------------
	// Render each model
	
	// Constant colours used for models in initial shaders
	D3DXVECTOR3 Black( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 Blue( 0.0f, 0.0f, 1.0f );

	// Render cube
	WorldMatrixVar->SetMatrix((float*)Cube->GetWorldMatrix());  // Send the cube's world matrix to the shader
	DiffuseMapVar->SetResource(CubeDiffuseMap);                 // Send the cube's diffuse/specular map to the shader
	NormalMapVar->SetResource(CubeNormalMap);                   // Send the cube's normal/depth map to the shader
	Cube->Render(ParallaxMappingTechnique);                     // Pass rendering technique to the model class

																// Same for the other models in the scene
	WorldMatrixVar->SetMatrix((float*)TeaPot->GetWorldMatrix());
	DiffuseMapVar->SetResource(TeapotDiffuseMap);
	NormalMapVar->SetResource(TeapotNormalMap);
	TeaPot->Render(ParallaxMappingTechnique);

//	WorldMatrixVar->SetMatrix((float*)Floor->GetWorldMatrix());
//	DiffuseMapVar->SetResource(FloorDiffuseMap);
//	NormalMapVar->SetResource(FloorNormalMap);
//	Floor->Render(ParallaxMappingTechnique);

																	 // Render SPHERE
	WorldMatrixVar->SetMatrix((float*)Sphere->GetWorldMatrix());  // Send the cube's world matrix to the shader
	DiffuseMapVar->SetResource(SphereDiffuseMap);                 // Send the cube's diffuse/specular map to the shader
	ModelColourVar->SetRawValue(Blue, 0, 12);           // Set a single colour to render the model
	Sphere->Render(VertexLitDiffuseTechnique);                         // Pass rendering technique to the model class

//	WorldMatrixVar->SetMatrix((float*)TeaPot->GetWorldMatrix());  // Send the cube's world matrix to the shader
//	DiffuseMapVar->SetResource(CubeDiffuseMap);                 // Send the cube's diffuse/specular map to the shader
//	ModelColourVar->SetRawValue(Blue, 0, 12);           // Set a single colour to render the model
//	TeaPot->Render(PlainColourTechnique);                         // Pass rendering technique to the model class


	// Same for the other models in the scene
	WorldMatrixVar->SetMatrix( (float*)Floor->GetWorldMatrix() );
    DiffuseMapVar->SetResource( FloorDiffuseMap );
	ModelColourVar->SetRawValue( Black, 0, 12 );
	Floor->Render(VertexLitDiffuseTechnique);

//	WorldMatrixVar->SetMatrix( (float*)Light1->GetWorldMatrix() );
//	ModelColourVar->SetRawValue( Light1->m_diffuse_colour(), 0, 12 );
//	Light1->Render( PlainColourTechnique );
//
//	WorldMatrixVar->SetMatrix( (float*)Light2->GetWorldMatrix() );
//	ModelColourVar->SetRawValue( Light2->m_diffuse_colour(), 0, 12 );
//	Light2->Render( PlainColourTechnique );

//	WorldMatrixVar->SetMatrix((float*)SpotLight->GetWorldMatrix());
//	ModelColourVar->SetRawValue(SpotLight->m_diffuse_colour(), 0, 12);
//	SpotLight->Render(PlainColourTechnique);

	WorldMatrixVar->SetMatrix((float*)SpotLights[0]->GetWorldMatrix());
	ModelColourVar->SetRawValue(Blue, 0, 12);
	SpotLights[0]->Render(PlainColourTechnique);


	WorldMatrixVar->SetMatrix((float*)SpotLights[1]->GetWorldMatrix());
	ModelColourVar->SetRawValue(Blue, 0, 12);
	SpotLights[1]->Render(PlainColourTechnique);

	WorldMatrixVar->SetMatrix((float*)SpotLights[2]->GetWorldMatrix());
	ModelColourVar->SetRawValue(Black, 0, 12);
	SpotLights[2]->Render(PlainColourTechnique);


	WorldMatrixVar->SetMatrix((float*)PointLights[0]->GetWorldMatrix());  // Send the cube's world matrix to the shader
	DiffuseMapVar->SetResource(SphereDiffuseMap);                 // Send the cube's diffuse/specular map to the shader
	ModelColourVar->SetRawValue(Blue, 0, 12);           // Set a single colour to render the model
	PointLights[0]->Render(VertexLitDiffuseTechnique);
	//---------------------------
	// Display the Scene

	// After we've finished drawing to the off-screen back buffer, we "present" it to the front buffer (the screen)
	SwapChain->Present( 0, 0 );
}
