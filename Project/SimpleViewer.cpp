//--------------------------------------------------------------------------------------
// File: Tutorial06.cpp
//
// This application demonstrates simple lighting in the vertex shader
//
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729723.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "resource.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "MeshUtils.h"
#include "LineUtils.h"
#include "Renderable.h"
#include "LoaderUtils.h"
#include "debug_renderer.h"
#include "PersonalMath.h"
#include <bitset>
#include <windowsx.h>
#include <chrono>
//#include "debug_renderer.cpp"
#define MIN (a,b) ({ int _a = (a); int _b = (b); _a < _b ? _a : _b; })
using namespace DirectX;
using namespace std;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
bool RENDER_STYLE_WIREFRAME = false;
bool RENDER_STYLE_TEXTURED = true;
bool RENDER_STYLE_TRANSPARENCY = false;
bool RASTER_FILL_CULL_NONE = false;
bool DEPTH_WRITE_ENABLED = true;
bool SKYBOX_ENABLED = false;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = nullptr;
HWND                    g_hWnd = nullptr;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*			g_pd3dDevice = nullptr;
ID3D11Device1*			g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*	g_pImmediateContext = nullptr;
ID3D11DeviceContext1*	g_pImmediateContext1 = nullptr;
IDXGISwapChain*			g_pSwapChain = nullptr;
IDXGISwapChain1*		g_pSwapChain1 = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Texture2D*		g_pDepthStencil = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11PixelShader*		g_pPixelShaderSolid = nullptr;
XMMATRIX                g_World;
XMMATRIX                g_View;
XMMATRIX                g_Projection;
ID3D11RasterizerState*	rasterStateDefault;
ID3D11RasterizerState*	rasterStateWireframe;
ID3D11RasterizerState*	rasterStateFillNoCull;
ID3D11BlendState*		transparencyState;
ID3D11DepthStencilState* pDSState;
ID3D11DepthStencilState* pDSStateNoWrite;

TransformsConstantBuffer modelViewProjection;
LightsConstantBuffer lightsAndColor;

std::bitset<256> keys = { 0 };
int currX = 0;
int currY = 180;
int x, y;
void UpdateCamera(end::float4x4 &viewMat);
bool ShowSkeleton = false;
//end::PersonalMath mathproxy;
vector<Renderable> renderables;
XMMATRIX JointContainers[65];
//XMMATRIX ConvertXM(end::float4x4);
// Main mesh
//Renderable meshRenderable;
XMMATRIX convertXM(end::float4x4 mat);
end::float4x4 convertfl(XMMATRIX mat);
// Grid mesh
Renderable gridRenderable;

int currKF = 16;

anim_clip_t AnimationClip;
// Main mesh
Renderable skyboxRenderable;

Renderable dbRenderers;
bool Backwards = false;

double delta_time = 0.0;
double finalvalue = 0.0;
double get_delta_time() 
{
	return delta_time;
}

double calc_delta_time()
{
	static std::chrono::time_point<std::chrono::high_resolution_clock> last_time = std::chrono::high_resolution_clock::now();

	std::chrono::time_point<std::chrono::high_resolution_clock> new_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = new_time - last_time;
	last_time = new_time;
	
	 return min(1.0 / 15.0, elapsed_seconds.count());;
}
// Used for overriding the main mesh texture
// with a generated white pixel texture to
// simulate no texturing
ComPtr<ID3D11ShaderResourceView> texSRV;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
HRESULT InitContent();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Update();
void Render();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	// enable console
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}

	if (FAILED(InitContent()))
	{
		CleanupDevice();
		return 0;
	}

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Update();
			Render();
		}
	}

	CleanupDevice();

	return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 1024, 768 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Simple Viewer",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}

void InitDebugTexture()
{
	static const uint32_t s_pixel = 0xffffffff;

	D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	ComPtr<ID3D11Texture2D> tex;
	HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &initData, tex.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		hr = g_pd3dDevice->CreateShaderResourceView(tex.Get(),
			&SRVDesc, texSRV.GetAddressOf());
	}
	assert(!FAILED(hr));
}

void InitRasterizerStates()
{
	D3D11_RASTERIZER_DESC rasterDesc;
	ZeroMemory(&rasterDesc, sizeof(rasterDesc));

	rasterDesc.AntialiasedLineEnable = true;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	HRESULT hr = g_pd3dDevice->CreateRasterizerState(&rasterDesc, &rasterStateWireframe);
	assert(!FAILED(hr));

	ZeroMemory(&rasterDesc, sizeof(rasterDesc));

	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	hr = g_pd3dDevice->CreateRasterizerState(&rasterDesc, &rasterStateDefault);
	assert(!FAILED(hr));

	ZeroMemory(&rasterDesc, sizeof(rasterDesc));

	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	hr = g_pd3dDevice->CreateRasterizerState(&rasterDesc, &rasterStateFillNoCull);
	assert(!FAILED(hr));
}

void InitDepthStates()
{
	D3D11_DEPTH_STENCIL_DESC dsDesc;

	// Depth test parameters
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;// D3D11_COMPARISON_LESS;

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state
	g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSStateNoWrite);
	// g_pImmediateContext->OMSetDepthStencilState(pDSStateNoWrite, 1);

	// Depth test parameters
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state
	g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSState);
	// g_pImmediateContext->OMSetDepthStencilState(pDSStateNoWrite, 1);
}

HRESULT InitSkybox()
{
	HRESULT hr = S_OK;

	//////////////////////////////////////////
	//Create mesh render components
	//////////////////////////////////////////
	{
		// Generate the geometry
		SimpleMesh<SimpleVertex> mesh;
		MeshUtils::makeCubePNT(mesh);

		// Create the vertex buffers from the generated SimpleMesh
		hr = skyboxRenderable.CreateBuffers(
			g_pd3dDevice,
			mesh.indicesList,
			(float*)mesh.vertexList.data(),
			sizeof(SimpleVertex),
			mesh.vertexList.size());

		// Load the Texture
		std::string filename = "sunsetcube1024.dds";
		hr = skyboxRenderable.CreateTextureFromFile(g_pd3dDevice, filename);
		if (FAILED(hr))
			return hr;

		// Create the sampler state
		hr = skyboxRenderable.CreateDefaultSampler(g_pd3dDevice);

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		// Create the shaders
		hr = skyboxRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Skybox_VS.cso", layout, ARRAYSIZE(layout));
		hr = skyboxRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Skybox_PS.cso");

		// Create the shader constant buffer
		hr = skyboxRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
	}
	return hr;
}

void InitBlendState()
{
	//Define the Blending Equation
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));

	D3D11_RENDER_TARGET_BLEND_DESC rtbd;
	ZeroMemory(&rtbd, sizeof(rtbd));

	rtbd.BlendEnable = true;
	int ALPHA_MODE = 1;
	switch (ALPHA_MODE)
	{
	case 0: // object alpha
	{
		rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.AlphaToCoverageEnable = false;
		break;
	}
	case 1: // pixel alpha
	{
		rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.AlphaToCoverageEnable = true;
		break;
	}
	case 2: // additive (emissive)
	{
		rtbd.SrcBlend = D3D11_BLEND_ONE;
		rtbd.DestBlend = D3D11_BLEND_ONE;
		blendDesc.AlphaToCoverageEnable = false;
		break;
	}
	}
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	blendDesc.RenderTarget[0] = rtbd;
	g_pd3dDevice->CreateBlendState(&blendDesc, &transparencyState);
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

		if (hr == E_INVALIDARG)
		{
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		}

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
		return hr;

	// Create swap chain
	IDXGIFactory2* dxgiFactory2 = nullptr;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
	if (dxgiFactory2)
	{
		// DirectX 11.1 or later
		hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
		if (SUCCEEDED(hr))
		{
			(void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));
		}

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;

		hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1);
		if (SUCCEEDED(hr))
		{
			hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
		}

		dxgiFactory2->Release();
	}
	else
	{
		// DirectX 11.0 systems
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = g_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
	}

	// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
	dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();

	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 1000.0f);

	return hr;
}


HRESULT InitContent()
{
	InitDebugTexture();
	InitRasterizerStates();
	InitDepthStates();
	InitSkybox();
	InitBlendState();
	InitFBX();

	//modelViewProjection = new ConstantBufferTransforms();

	HRESULT hr = S_OK;
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);
	// Orbit camera
	//XMVECTOR Eye = XMVectorSet(cos(t / 2) * 10, 4.0f, sin(t / 2) * 10, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_View = XMMatrixLookAtLH(Eye, At, Up);
	PersonalMath mathproxy;
	end::float4x4 temp = convertfl(g_View);
	temp = mathproxy.MultiplyMat(mathproxy.RotationY(45), temp);
	g_View = (XMMATRIX&)temp;
	
	//////////////////////////////////////////
	//Create mesh render components
	//////////////////////////////////////////
	{
		Renderable meshRenderable;

		// Generate the geometry
		SimpleMesh<SimpleVertex> mesh;

		// filename for texture file
		//std::string filename;

		MeshUtils::makeCrossHatchPNT(mesh, 0.5f);
		std::string filename1 = "grass.dds";

		hr = meshRenderable.CreateBuffers(
			g_pd3dDevice,
			mesh.indicesList,
			(float*)mesh.vertexList.data(),
			sizeof(SimpleVertex),
			mesh.vertexList.size());

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BLENDWEIGHTS",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0},
			{ "BLENDINDICES",0,DXGI_FORMAT_R32G32B32A32_UINT,0,48,D3D11_INPUT_PER_VERTEX_DATA,0}
		};

		// Load the Texture
		hr = meshRenderable.CreateTextureFromFile(g_pd3dDevice, filename1);
		if (FAILED(hr))
			return hr;

		// Create the sampler state
		hr = meshRenderable.CreateDefaultSampler(g_pd3dDevice);

		// Create the shaders
		hr = meshRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Tutorial06_VS.cso", layout, ARRAYSIZE(layout));
		hr = meshRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Tutorial06_PS.cso");

		// Create the shader constant buffer
		hr = meshRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
		hr = meshRenderable.CreateConstantBufferPS(g_pd3dDevice, sizeof(LightsConstantBuffer));
		meshRenderable.setPosition(0.0f, 0.0f, -3.0f);
		//renderables.push_back(meshRenderable);
		meshRenderable.setPosition(2.0f, 0.0f, -3.0f);
		//renderables.push_back(meshRenderable);
		meshRenderable.setPosition(-2.0f, 0.0f, -3.0f);
		//renderables.push_back(meshRenderable);


		float scale = 1;
		// Load it!
		std::vector<std::string> filename;
		filename.push_back("");
		filename.push_back("");
		filename.push_back("");
		LoadFBX(".//Assets//Run.fbx", mesh, filename,1, AnimationClip);
		
		
		// Create the vertex buffers from the generated SimpleMesh
		hr = meshRenderable.CreateBuffers(
			g_pd3dDevice,
			mesh.indicesList,
			(float*)mesh.vertexList.data(),
			sizeof(SimpleVertex),
			mesh.vertexList.size());

		// Load the Texture when texture filename is valid
		if (filename[0] != "")
		{
			hr = meshRenderable.CreateTextureFromFile(g_pd3dDevice, ".//Assets//" + filename[0]);
			if (FAILED(hr))
				return hr;


			// Create the sampler state
			hr = meshRenderable.CreateDefaultSampler(g_pd3dDevice);
		}
		if (filename[1] != "")
		{
			hr = meshRenderable.CreateTextureFromFileEmissive(g_pd3dDevice, ".//Assets//" + filename[1]);
			if (FAILED(hr))
				return hr;


			// Create the sampler state
			hr = meshRenderable.CreateDefaultSampler(g_pd3dDevice);
		}
		if (filename[2] != "")
		{
			hr = meshRenderable.CreateTextureFromFileSpecular(g_pd3dDevice, ".//Assets//" + filename[2]);
			if (FAILED(hr))
				return hr;


			// Create the sampler state
			hr = meshRenderable.CreateDefaultSampler(g_pd3dDevice);
		}

		//Define the input layout
	   //end::debug_renderer::add_line(float3(1, 1, 1), float3(3, 3, 3), float4(1, 1, 1, 1));
		//meshRenderable.setRotation(XMMatrixRotationY(3.14f));
	   // Create the shaders
		hr = meshRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Tutorial06_VS.cso", layout, ARRAYSIZE(layout));
		hr = meshRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Tutorial06_PS.cso");

		// Create the shader constant buffer
		hr = meshRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
		hr = meshRenderable.CreateConstantBufferPS(g_pd3dDevice, sizeof(LightsConstantBuffer));
		meshRenderable.setPosition(0.0f, 0.0f, 0.0f);
		
		renderables.push_back(meshRenderable);
		meshRenderable.setPosition(2.0f, 0.0f, 0.0f);
		//renderables.push_back(meshRenderable);


		D3D11_INPUT_ELEMENT_DESC lineLayoutDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		// Create the shaders
		hr = dbRenderers.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Debug_VS.cso", lineLayoutDesc, ARRAYSIZE(lineLayoutDesc));
		hr = dbRenderers.CreatePixelShaderFromFile(g_pd3dDevice, "Debug_PS.cso");

		// Create the shader constant buffer
		hr = dbRenderers.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
		//LoadFBX(".//Assets//crate.fbx", mesh, filename, 2.0f);
		//LoadFBX1(".//Assets//Idle.fbx", mesh, filename, 0.75f);
		// Create the vertex buffers from the generated SimpleMesh
		//hr = meshRenderable.CreateBuffers(
		//	g_pd3dDevice,
		//	mesh.indicesList,
		//	(float*)mesh.vertexList.data(),
		//	sizeof(SimpleVertex),
		//	mesh.vertexList.size());
		//
		// Load the Texture when texture filename is valid
		//if (filename1 != "")
		//{
		//	hr = meshRenderable.CreateTextureFromFile(g_pd3dDevice, ".//Assets//" + filename1);
		//	if (FAILED(hr))
		//		return hr;
		//
		//	// Create the sampler state
		//	hr = meshRenderable.CreateDefaultSampler(g_pd3dDevice);
		//}
		//// Create the shaders
		//hr = meshRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Tutorial06_VS.cso", layout, ARRAYSIZE(layout));
		//hr = meshRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Tutorial06_PS.cso");
		//
		//// Create the shader constant buffer
		//hr = meshRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
		//hr = meshRenderable.CreateConstantBufferPS(g_pd3dDevice, sizeof(LightsConstantBuffer));
		//meshRenderable.setPosition(0.0f, 0.0f, 0.0f);
		////renderables.push_back(meshRenderable);
		//
		//LoadFBX1(".//Assets//chair.fbx", mesh, filename1, 1.0f/5);
		//
		//// Create the vertex buffers from the generated SimpleMesh
		//hr = meshRenderable.CreateBuffers(
		//	g_pd3dDevice,
		//	mesh.indicesList,
		//	(float*)mesh.vertexList.data(),
		//	sizeof(SimpleVertex),
		//	mesh.vertexList.size());
		//
		//// Load the Texture when texture filename is valid
		//if (filename1 != "")
		//{
		//	hr = meshRenderable.CreateTextureFromFile(g_pd3dDevice, ".//Assets//" + filename1);
		//	if (FAILED(hr))
		//		return hr;
		//
		//	// Create the sampler state
		//	hr = meshRenderable.CreateDefaultSampler(g_pd3dDevice);
		//}
		//// Create the shaders
		//hr = meshRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Tutorial06_VS.cso", layout, ARRAYSIZE(layout));
		//hr = meshRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Tutorial06_PS.cso");
		//
		//// Create the shader constant buffer
		//hr = meshRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
		//hr = meshRenderable.CreateConstantBufferPS(g_pd3dDevice, sizeof(LightsConstantBuffer));
		//meshRenderable.setPosition(-2.0f, 1.5f, -2.0f);
		////renderables.push_back(meshRenderable);
		//
		//
		//LoadFBX2(".//Assets//table.fbx", mesh, filename1, 1.0f / 5.0f);
		//
		//// Create the vertex buffers from the generated SimpleMesh
		//hr = meshRenderable.CreateBuffers(
		//	g_pd3dDevice,
		//	mesh.indicesList,
		//	(float*)mesh.vertexList.data(),
		//	sizeof(SimpleVertex),
		//	mesh.vertexList.size());
		//
		//// Load the Texture when texture filename is valid
		//if (filename1 != "")
		//{
		//	hr = meshRenderable.CreateTextureFromFile(g_pd3dDevice, ".//Assets//" + filename1);
		//	if (FAILED(hr))
		//		return hr;
		//
		//	// Create the sampler state
		//	hr = meshRenderable.CreateDefaultSampler(g_pd3dDevice);
		//}
		//// Create the shaders
		//hr = meshRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Tutorial06_VS.cso", layout, ARRAYSIZE(layout));
		//hr = meshRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Tutorial06_PS.cso");
		//
		//// Create the shader constant buffer
		//hr = meshRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
		//hr = meshRenderable.CreateConstantBufferPS(g_pd3dDevice, sizeof(LightsConstantBuffer));
		//meshRenderable.setPosition(2.0f, 1.5f, -2.0f);
		//renderables.push_back(meshRenderable);






		PersonalMath mathproxy;
		// Create grid render components
		{
			// Generate the geometry
			DebugLines lines;
			LineUtils::MakeGrid(lines);
			std::vector<end::colored_vertex> data = end::debug_renderer::get_all_data();
			for (int i = 0; i < end::debug_renderer::get_line_vert_count(); i++)
			{
				ColorVertex curr;
				curr.color = mathproxy.ConverttoXM(data[i].color);// { data[i].color.x, data[i].color.y, data[i].color.z, data[i].color.w};
				curr.pos1 = mathproxy.ConverttoXM(data[i].pos);//s { data[i].pos.x, data[i].pos.y, data[i].pos.z };
				lines.vertexList.push_back(curr);
			}

			// Create the vertex buffers from the lines vector
			hr = gridRenderable.CreateVertexBuffer(g_pd3dDevice, (float*)lines.vertexList.data(), sizeof(ColorVertex), (int)lines.vertexList.size());

			gridRenderable.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

			// Define the input layout
			D3D11_INPUT_ELEMENT_DESC lineLayoutDesc[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
			};

			// Create the shaders
			hr = gridRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Debug_VS.cso", lineLayoutDesc, ARRAYSIZE(lineLayoutDesc));
			hr = gridRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Debug_PS.cso");

			// Create the shader constant buffer
			hr = gridRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
		}

		// load and create the pixel shader for the light markers
		auto ps_blob = load_binary_blob("PSSolid.cso");
		hr = g_pd3dDevice->CreatePixelShader(ps_blob.data(), ps_blob.size(), nullptr, &g_pPixelShaderSolid);
		if (FAILED(hr))
			return hr;

		g_pImmediateContext->RSSetState(rasterStateDefault);

		return S_OK;
	}
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();
	if (rasterStateDefault) rasterStateDefault->Release();
	if (rasterStateWireframe) rasterStateWireframe->Release();
	if (rasterStateFillNoCull) rasterStateFillNoCull->Release();
	if (g_pPixelShaderSolid) g_pPixelShaderSolid->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain1) g_pSwapChain1->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext1) g_pImmediateContext1->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice1) g_pd3dDevice1->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
	if (pDSState) pDSState->Release();
	if (pDSStateNoWrite) pDSStateNoWrite->Release();
	if (transparencyState) transparencyState->Release();

}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case '1':
			std::cout << "Keypressed - 1" << endl;
			// toggle wireframe state
			RENDER_STYLE_WIREFRAME = !RENDER_STYLE_WIREFRAME;
			std::cout << "RENDER_STYLE_WIREFRAME: " << RENDER_STYLE_WIREFRAME << endl;
			break;
		case '2':
			std::cout << "Keypressed - 2" << endl;
			// toggle textured state
			RENDER_STYLE_TEXTURED = !RENDER_STYLE_TEXTURED;
			std::cout << "RENDER_STYLE_TEXTURED: " << RENDER_STYLE_TEXTURED << endl;
			break;
		case '3':
			std::cout << "Keypressed - 3" << endl;
			// toggle alpha blending state
			RENDER_STYLE_TRANSPARENCY = !RENDER_STYLE_TRANSPARENCY;
			std::cout << "RENDER_STYLE_TRANSPARENCY: " << RENDER_STYLE_TRANSPARENCY << endl;
			break;
		case '4':
			std::cout << "Keypressed - 4" << endl;
			// toggle cull front/none state
			RASTER_FILL_CULL_NONE = !RASTER_FILL_CULL_NONE;
			std::cout << "RASTER_FILL_CULL_NONE: " << RASTER_FILL_CULL_NONE << endl;
			break;
		case '5':
			std::cout << "Keypressed - 5" << endl;
			// toggle cull front/none state
			DEPTH_WRITE_ENABLED = !DEPTH_WRITE_ENABLED;
			std::cout << "DEPTH_WRITE_ENABLED: " << DEPTH_WRITE_ENABLED << endl;
			break;
		case '6':
			std::cout << "Keypressed - 6" << endl;
			// toggle wireframe state
			//currKF++;
			if (Backwards == true)
				Backwards = false;
			else
				Backwards = true;
			break;
		case '7':
			std::cout << "Keypressed - 7" << endl;
			// toggle wireframe state
			//currKF++;
			currKF--;
			break;
		case '8':
			std::cout << "Keypressed - 8" << endl;
			// toggle wireframe state
			if (ShowSkeleton)
			{
				ShowSkeleton = false;
			}
			else
			{
				ShowSkeleton = true;
			}
			break;
		case '0':
			std::cout << "Keypressed - 0" << endl;
			// toggle skybox
			SKYBOX_ENABLED = !SKYBOX_ENABLED;
			std::cout << "SKYBOX_ENABLED: " << SKYBOX_ENABLED << endl;
			break;
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	switch (message)
	{
	case WM_KEYDOWN:
		keys[wParam] = true;
		break;
	case WM_KEYUP:
		keys[wParam] = false;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_MOUSEMOVE:
		currX = GET_X_LPARAM(lParam);
		currY = GET_Y_LPARAM(lParam);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;

	return 0;
}



XMFLOAT4 vLightDirs[2] =
{
	XMFLOAT4(-0.577f, 0.577f, -0.577f, 1.0f),
	XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f),
};
XMFLOAT4 vLightColors[2] =
{
	XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
	XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
};

XMMATRIX convertXM(end::float4x4 mat)
{
	XMMATRIX ans = XMMATRIX(mat[0].x,mat[0].y,mat[0].z,mat[0].w,
							mat[1].x, mat[1].y, mat[1].z, mat[1].w, 
							mat[2].x, mat[2].y, mat[2].z, mat[2].w, 
							mat[3].x, mat[3].y, mat[3].z, mat[3].w );
	
	return ans;
}
end::float4x4 convertfl(XMMATRIX mat)
{
	end::float4x4 ans = { end::float4 {mat.r[0].m128_f32[0],mat.r[0].m128_f32[1],mat.r[0].m128_f32[2],mat.r[0].m128_f32[3]},
						  end::float4 {mat.r[1].m128_f32[0],mat.r[1].m128_f32[1],mat.r[1].m128_f32[2],mat.r[1].m128_f32[3]},
						  end::float4 {mat.r[2].m128_f32[0],mat.r[2].m128_f32[1],mat.r[2].m128_f32[2],mat.r[2].m128_f32[3]},
						  end::float4 {mat.r[3].m128_f32[0],mat.r[3].m128_f32[1],mat.r[3].m128_f32[2],mat.r[3].m128_f32[3]}
	};
	return ans;
}

//--------------------------------------------------------------------------------------
// Update
//--------------------------------------------------------------------------------------
void Update()
{
	// Update our time
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static ULONGLONG timeStart = 0;
		ULONGLONG timeCur = GetTickCount64();
		if (timeStart == 0)
			timeStart = timeCur;
		t = (timeCur - timeStart) / 1000.0f;
	}

	// Rotate cube around the origin
	//g_World = XMMatrixRotationY(t);
	// Initialize the world matrices
	//g_World = XMMatrixIdentity();

	// Initialize the view matrix
	// Stationary camera
	end::float4x4 temp = convertfl(g_View);
	UpdateCamera(temp);
	g_View = convertXM(temp);

	// Setup our lighting parameters
	XMStoreFloat4(&vLightDirs[0], { -0.577f, 0.577f, -0.577f, 1.0f });
	XMStoreFloat4(&vLightDirs[1], { 0.577f, 0.2577f, -0.577f, 1.0f });

	XMStoreFloat4(&vLightColors[0], { 0.75f, 0.75f, 0.75f, 1.0f });
	XMStoreFloat4(&vLightColors[1], { 1.0f, 1, 1, 1.0f });
	
	// Rotate the second light around the origin
	XMMATRIX mRotate = XMMatrixRotationY(-1.0f * t);
	XMVECTOR vLightDir = XMLoadFloat4(&vLightDirs[1]);
	// rotates the second light
	//vLightDir = XMVector3Transform(vLightDir, mRotate);
	XMStoreFloat4(&vLightDirs[1], vLightDir);
	
	
}

void UpdateCamera(end::float4x4 & viewMat)
{
	end::PersonalMath mathproxy;
	//currY = 180;
	//int dx = currX - x;
	//int dy = currY - y;
	//x = currX;
	//y = currY;

	//float4x4 rotationMat = mathproxy.RotationX(0);
	//viewMat = mathproxy.MultiplyMat(rotationMat, viewMat);
	//rotationMat = mathproxy.RotationY(currY);
	//viewMat = mathproxy.MultiplyMat( rotationMat,viewMat );
	if (keys['S'])
	{
		end::float4x4 trans = mathproxy.Identity();
		trans[3].z += 0.2f;
		viewMat = mathproxy.MultiplyMat(trans, viewMat);
		keys['S'] = false;
	}
	if (keys['W'])
	{
		end::float4x4 trans = mathproxy.Identity();
		trans[3].z -= 0.2f;
		viewMat = mathproxy.MultiplyMat(trans, viewMat);
		keys['W'] = false;
	}
	if (keys['D'])
	{
		end::float4x4 trans = mathproxy.Identity();
		trans[3].x -= 0.2f;
		viewMat = mathproxy.MultiplyMat(trans, viewMat);
		keys['D'] = false;
	}
	if (keys['A'])
	{
		end::float4x4 trans = mathproxy.Identity();
		trans[3].x += 0.2f;
		viewMat = mathproxy.MultiplyMat(trans, viewMat);
		keys['A'] = false;
	}
	if (keys['C'])
	{
		end::float4x4 trans = mathproxy.Identity();
		trans[3].y -= 0.2f;
		viewMat = mathproxy.MultiplyMat(trans, viewMat);
		keys['C'] = false;
	}
	if (keys['V'])
	{
		end::float4x4 trans = mathproxy.Identity();
		trans[3].y += 0.2f;
		viewMat = mathproxy.MultiplyMat(trans, viewMat);
		keys['V'] = false;
	}
}

void renderGrid(Renderable curr)
{
	// set up default render state
	// no blending
	g_pImmediateContext->OMSetBlendState(nullptr, 0, 0xffffffff);
	// solid, no wireframe
	g_pImmediateContext->RSSetState(rasterStateDefault);
	// write z
	g_pImmediateContext->OMSetDepthStencilState(pDSState, 1);
	
	TransformsConstantBuffer cbDebug;
	cbDebug.mWorld = XMMatrixIdentity();
	cbDebug.mView = XMMatrixTranspose(g_View);
	cbDebug.mProjection = XMMatrixTranspose(g_Projection);
	
	g_pImmediateContext->UpdateSubresource(curr.constantBufferVS.Get(), 0, nullptr, &cbDebug, 0, 0);
	//g_pImmediateContext->UpdateSubresource(curr.constantBufferVS.Get(), 0, nullptr, &cbDebug1, 0, 0);

	curr.Bind(g_pImmediateContext);
	curr.Draw(g_pImmediateContext);
	
}


// Mesh render routine that supports toggling texturing
// and toggling overlay wireframe
void renderMesh(Renderable meshRenderable)
{
	// copy transform to constant buffer
	modelViewProjection.mWorld = XMMatrixTranspose(meshRenderable.world);
	
	// send the constant buffers to the GPU
	g_pImmediateContext->UpdateSubresource(meshRenderable.constantBufferVS.Get(), 0, nullptr, &modelViewProjection, 0, 0);
	g_pImmediateContext->UpdateSubresource(meshRenderable.constantBufferPS.Get(), 0, nullptr, &lightsAndColor, 0, 0);
	// set all of the render states
	meshRenderable.Bind(g_pImmediateContext);

	// set the solid raster state
	if (RASTER_FILL_CULL_NONE)
		g_pImmediateContext->RSSetState(rasterStateFillNoCull);
	else
		g_pImmediateContext->RSSetState(rasterStateDefault);

	// if not textured, set the current texture to the generate white texture
	if (!RENDER_STYLE_TEXTURED)
	{
		g_pImmediateContext->PSSetShaderResources(0, 1, texSRV.GetAddressOf());
	}
	if (RENDER_STYLE_TRANSPARENCY)
	{
		g_pImmediateContext->OMSetBlendState(transparencyState, 0, 0xffffffff);
	}
	else
	{
		g_pImmediateContext->OMSetBlendState(nullptr, 0, 0xffffffff);
	}
	//renderables[0].setRotation();
	// Bind and Draw the vertices
	meshRenderable.Draw(g_pImmediateContext);

	// redraw the whole mesh in wireframe mode
	if (RENDER_STYLE_WIREFRAME)
	{
		g_pImmediateContext->OMSetBlendState(nullptr, 0, 0xffffffff);

		// Set the color of the wireframe
		//cb1.vOutputColor = { 0.9f, 0.9f, 0.9f, 1.0f };
		g_pImmediateContext->UpdateSubresource(meshRenderable.constantBufferVS.Get(), 0, nullptr, &modelViewProjection, 0, 0);

		// set the solid pixel shader
		g_pImmediateContext->PSSetShader(g_pPixelShaderSolid, nullptr, 0);

		// set the wireframe raster state
		g_pImmediateContext->RSSetState(rasterStateWireframe);

		// Draw the mesh
		meshRenderable.Draw(g_pImmediateContext);
	}
	
}

float4x4 Slerp(float4x4 Point1, float4x4 Point2, float Ratio)
{
	FXMMATRIX quat1 = convertXM(Point1);
	FXMMATRIX quat2 = convertXM(Point2);
	XMVECTOR vec1 = XMQuaternionRotationMatrix(quat1);
	XMVECTOR vec2 = XMQuaternionRotationMatrix(quat2);
	XMVECTOR ans = XMQuaternionSlerp(vec1, vec2, Ratio);
	FXMMATRIX ansM = XMMatrixRotationQuaternion(ans);
	float4 LerpPos;
	LerpPos.x= Point1[3].x + Ratio*(Point2[3].x - Point1[3].x);
	LerpPos.y = Point1[3].y + Ratio * (Point2[3].y - Point1[3].y);
	LerpPos.z = Point1[3].z + Ratio * (Point2[3].z - Point1[3].z);
	LerpPos.w = Point1[3].w + Ratio * (Point2[3].w - Point1[3].w);
	float4x4 finalMatrix = convertfl(ansM);
	finalMatrix[3] = LerpPos;
	return finalMatrix;
}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
	
	//
	// Clear the back buffer
	//
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::MidnightBlue);

	//
	// Clear the depth buffer to 1.0 (max depth)
	//
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//
	// Render the grid
	//
	end::PersonalMath mathproxy;

	delta_time = calc_delta_time();
	finalvalue += delta_time;
	if (!Backwards)
	{
		int previos = (finalvalue / AnimationClip.duration) * AnimationClip.keyframes.size();

		if (previos >= 16)
		{
			finalvalue = 0.0666;
			previos = 1;
		}
		int next = previos + 1;

		//d = (t - offset) / (second time - offset);
		double currRatio = (finalvalue - AnimationClip.keyframes[previos].time) / (AnimationClip.keyframes[next].time - AnimationClip.keyframes[previos].time);
		int n = AnimationClip.keyframes[previos].joints.size();

		for (int i = 0; i < n; i++)
		{
			float4x4 SlerpMatrix = Slerp(AnimationClip.keyframes[previos].joints[i].transform, AnimationClip.keyframes[next].joints[i].transform, currRatio);
			if (ShowSkeleton)
			{
				float3 Xaxis = mathproxy.FindNextPos(SlerpMatrix[3].xyz, SlerpMatrix[0].xyz, 1);
				end::debug_renderer::add_line(SlerpMatrix[3].xyz, Xaxis, float4(1, 0, 0, 1));
				float3 Yaxis = mathproxy.FindNextPos(SlerpMatrix[3].xyz, SlerpMatrix[1].xyz, 1);
				end::debug_renderer::add_line(SlerpMatrix[3].xyz, Yaxis, float4(0, 1, 0, 1));
				float3 Zaxis = mathproxy.FindNextPos(SlerpMatrix[3].xyz, SlerpMatrix[2].xyz, 1);
				end::debug_renderer::add_line(SlerpMatrix[3].xyz, Zaxis, float4(0, 0, 1, 1));
			}
			XMMATRIX inv_bind = XMMatrixInverse(nullptr, (XMMATRIX&)AnimationClip.keyframes.front().joints[i].transform);
			XMMATRIX tween_joint = (XMMATRIX&)SlerpMatrix;
			XMMATRIX bind_pose = (XMMATRIX&)AnimationClip.keyframes.front().joints[i].transform;
			XMMATRIX joint_delta = XMMatrixMultiply(inv_bind, tween_joint);
			//joint_delta *= joint_delta;
			JointContainers[i] =XMMatrixTranspose(joint_delta);
		}
		
	
		if (ShowSkeleton)
		{
			for (int i = n - 1; i > 0; i--)
			{
				float4x4 SlerpMatrix = Slerp(AnimationClip.keyframes[previos].joints[i].transform, AnimationClip.keyframes[next].joints[i].transform, currRatio);
				int parent = AnimationClip.keyframes[currKF].joints[i].parent;
				float4x4 ParentSlerpMatrix = Slerp(AnimationClip.keyframes[previos].joints[parent].transform, AnimationClip.keyframes[next].joints[parent].transform, currRatio);
				end::debug_renderer::add_line(SlerpMatrix[3].xyz, ParentSlerpMatrix[3].xyz, float4(1, 1, 1, 1));
			}
		}
	}
	else
	{
		//int previos = ((AnimationClip.duration - finalvalue) / AnimationClip.duration) * AnimationClip.keyframes.size();
		//int next = previos - 1;
		//if (next <= 1)
		//{
		//	next = 15;
		//}
		//if (previos <= 1)
		//{
		//	previos = 16;
		//	finalvalue = 0.066;
		//}
		//if (previos <= 1)
		//{
		//	finalvalue = 0.0666;
		//	previos = 16;
		//}
		if (currKF <= 1)
		{
			currKF = 16;
			finalvalue = 0.066;
		}

		//d = (t - offset) / (second time - offset);
		//double currRatio = (finalvalue - AnimationClip.keyframes[currKF - 1].time) / (AnimationClip.keyframes[currKF].time - AnimationClip.keyframes[currKF - 1].time);
		int n = AnimationClip.keyframes[currKF - 1].joints.size();

		for (int i = 0; i < n; i++)
		{
			//float4x4 SlerpMatrix = Slerp(AnimationClip.keyframes[currKF].joints[i].transform, AnimationClip.keyframes[currKF].joints[i].transform, currRatio);

			float3 Xaxis = mathproxy.FindNextPos(AnimationClip.keyframes[currKF].joints[i].transform[3].xyz, AnimationClip.keyframes[currKF].joints[i].transform[0].xyz, 0.25f);
			end::debug_renderer::add_line(AnimationClip.keyframes[currKF].joints[i].transform[3].xyz, Xaxis, float4(1, 0, 0, 1));
			float3 Yaxis = mathproxy.FindNextPos(AnimationClip.keyframes[currKF].joints[i].transform[3].xyz, AnimationClip.keyframes[currKF].joints[i].transform[1].xyz, 0.25f);
			end::debug_renderer::add_line(AnimationClip.keyframes[currKF].joints[i].transform[3].xyz, Yaxis, float4(0, 1, 0, 1));
			float3 Zaxis = mathproxy.FindNextPos(AnimationClip.keyframes[currKF].joints[i].transform[3].xyz, AnimationClip.keyframes[currKF].joints[i].transform[2].xyz, 0.25f);
			end::debug_renderer::add_line(AnimationClip.keyframes[currKF].joints[i].transform[3].xyz, Zaxis, float4(0, 0, 1, 1));
		}

		for (int i = n - 1; i > 0; i--)
		{
			//float4x4 SlerpMatrix = Slerp(AnimationClip.keyframes[currKF].joints[i].transform, AnimationClip.keyframes[currKF].joints[i].transform, currRatio);
			int parent = AnimationClip.keyframes[currKF].joints[i].parent;
			//float4x4 ParentSlerpMatrix = Slerp(AnimationClip.keyframes[currKF].joints[parent].transform, AnimationClip.keyframes[currKF].joints[parent].transform, currRatio);
			end::debug_renderer::add_line(AnimationClip.keyframes[currKF].joints[i].transform[3].xyz, AnimationClip.keyframes[currKF].joints[parent].transform[3].xyz, float4(1, 1, 1, 1));
		}
	}
	auto hr = dbRenderers.CreateVertexBuffer(g_pd3dDevice, (float*)debug_renderer::get_line_verts(), sizeof(colored_vertex), (int)debug_renderer::get_line_vert_count());

	dbRenderers.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	
	
	// Define the input layout
	


	renderGrid(gridRenderable);
	renderGrid(dbRenderers);
	
	//
	// Update matrix variables and lighting variables
	//
	//renderSkeleton();
	for (int i = 0; i < 65; i++)
	{
		modelViewProjection.mMat[i] = JointContainers[i];
		//modelViewProjection.mMat[i] = XMMatrixIdentity();
	}
	modelViewProjection.mView = XMMatrixTranspose(g_View);
	modelViewProjection.mProjection = XMMatrixTranspose(g_Projection);

	lightsAndColor.vLightDir[0] = vLightDirs[0];
	lightsAndColor.vLightDir[1] = vLightDirs[1];
	lightsAndColor.vLightColor[0] = vLightColors[0];
	lightsAndColor.vLightColor[1] = vLightColors[1];
	lightsAndColor.vOutputColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	//
	// Render the renderables
	//
	if (DEPTH_WRITE_ENABLED)
		g_pImmediateContext->OMSetDepthStencilState(pDSState, 1);
	else
		g_pImmediateContext->OMSetDepthStencilState(pDSStateNoWrite, 1);
	
	XMMATRIX rotationMatrix = convertXM(mathproxy.RotationY(180));
	renderables[0].setRotation(rotationMatrix);

	// Render all of the renderables in the scene
	for (auto r : renderables)
	{
		renderMesh(r);
	}

	// TODO:RASTER STATE SOLID 
	g_pImmediateContext->RSSetState(rasterStateDefault);
	g_pImmediateContext->OMSetDepthStencilState(pDSState, 1);
	g_pImmediateContext->OMSetBlendState(nullptr, 0, 0xffffffff);

	// TODO:RASTER STATE SOLID 
	//g_pImmediateContext->RSSetState(rasterStateDefault);

	//
	// Render each light
	//
	for (int m = 0; m < 2; m++)
	{
		XMMATRIX mLight = XMMatrixTranslationFromVector(5.0f * XMLoadFloat4(&vLightDirs[m]));
		XMMATRIX mLightScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
		mLight = mLightScale * mLight;

		// Update the world variable to reflect the current light
		modelViewProjection.mWorld = XMMatrixTranspose(mLight);
		lightsAndColor.vOutputColor = vLightColors[m];
		g_pImmediateContext->RSSetState(rasterStateFillNoCull);
		g_pImmediateContext->UpdateSubresource(renderables[0].constantBufferVS.Get(), 0, nullptr, &modelViewProjection, 0, 0);
		g_pImmediateContext->UpdateSubresource(renderables[0].constantBufferPS.Get(), 0, nullptr, &lightsAndColor, 0, 0);
		// inline override of the pixel shader
		g_pImmediateContext->PSSetShader(g_pPixelShaderSolid, nullptr, 0);
		//renderables[0].Draw(g_pImmediateContext);
	}

	/// Draw Skybox
	if (SKYBOX_ENABLED)
	{
		TransformsConstantBuffer cbDebug;
		cbDebug.mWorld = XMMatrixIdentity();
		// zero out the camera postion so the skybox renders 
		// AT the camera postion
		XMMATRIX tmpMat = g_View;
		tmpMat.r[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
		cbDebug.mView = XMMatrixTranspose(tmpMat);
		cbDebug.mProjection = XMMatrixTranspose(g_Projection);

		g_pImmediateContext->UpdateSubresource(skyboxRenderable.constantBufferVS.Get(), 0, nullptr, &cbDebug, 0, 0);
		g_pImmediateContext->OMSetDepthStencilState(pDSStateNoWrite, 1);

		skyboxRenderable.Bind(g_pImmediateContext);
		skyboxRenderable.Draw(g_pImmediateContext);
		//g_pImmediateContext->OMSetDepthStencilState(pDSState, 1);
	}

	//
	// Present our back buffer to our front buffer
	//
	g_pSwapChain->Present(0, 0);
	end::debug_renderer::clear_lines();
}

