/**
 * Copyright (C) 2015 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/d3d8to9#license
 */

#include "d3d8to9.hpp"
#include <mge/configuration.h>

static const D3DFORMAT AdapterFormats[] = {
	D3DFMT_A8R8G8B8,
	D3DFMT_X8R8G8B8,
	D3DFMT_R5G6B5,
	D3DFMT_X1R5G5B5,
	D3DFMT_A1R5G5B5
};

Direct3D8::Direct3D8(IDirect3D9 *ProxyInterface) :
	ProxyInterface(ProxyInterface)
{
	D3DDISPLAYMODE pMode;

	CurrentAdapterCount = ProxyInterface->GetAdapterCount();
	if (CurrentAdapterCount > MaxAdapters)
		CurrentAdapterCount = MaxAdapters;

	for (UINT Adapter = 0; Adapter < CurrentAdapterCount; Adapter++)
	{
		for (D3DFORMAT Format : AdapterFormats)
		{
			const UINT ModeCount = ProxyInterface->GetAdapterModeCount(Adapter, Format);

			for (UINT Mode = 0; Mode < ModeCount; Mode++)
			{
				ProxyInterface->EnumAdapterModes(Adapter, Format, Mode, &pMode);
				CurrentAdapterModes[Adapter].push_back(pMode);
				CurrentAdapterModeCount[Adapter]++;
			}
		}
	}
}
Direct3D8::~Direct3D8()
{
}

HRESULT STDMETHODCALLTYPE Direct3D8::QueryInterface(REFIID riid, void **ppvObj)
{
	if (ppvObj == nullptr)
		return E_POINTER;

	if (riid == __uuidof(IDirect3D8) ||
		riid == __uuidof(IUnknown))
	{
		AddRef();
		*ppvObj = static_cast<IDirect3D8 *>(this);

		return S_OK;
	}

	return ProxyInterface->QueryInterface(ConvertREFIID(riid), ppvObj);
}
ULONG STDMETHODCALLTYPE Direct3D8::AddRef()
{
	return ProxyInterface->AddRef();
}
ULONG STDMETHODCALLTYPE Direct3D8::Release()
{
	const ULONG LastRefCount = ProxyInterface->Release();

	if (LastRefCount == 0)
		delete this;

	return LastRefCount;
}

HRESULT STDMETHODCALLTYPE Direct3D8::RegisterSoftwareDevice(void *pInitializeFunction)
{
	return ProxyInterface->RegisterSoftwareDevice(pInitializeFunction);
}
UINT STDMETHODCALLTYPE Direct3D8::GetAdapterCount()
{
	return CurrentAdapterCount;
}
HRESULT STDMETHODCALLTYPE Direct3D8::GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8 *pIdentifier)
{
	if (pIdentifier == nullptr)
		return D3DERR_INVALIDCALL;

	D3DADAPTER_IDENTIFIER9 AdapterIndentifier;

	if ((Flags & D3DENUM_NO_WHQL_LEVEL) == 0)
	{
		Flags |= D3DENUM_WHQL_LEVEL;
	}
	else
	{
		Flags ^= D3DENUM_NO_WHQL_LEVEL;
	}

	const HRESULT hr = ProxyInterface->GetAdapterIdentifier(Adapter, Flags, &AdapterIndentifier);
	if (FAILED(hr))
		return hr;

	ConvertAdapterIdentifier(AdapterIndentifier, *pIdentifier);

	return D3D_OK;
}
UINT STDMETHODCALLTYPE Direct3D8::GetAdapterModeCount(UINT Adapter)
{
	return CurrentAdapterModeCount[Adapter];
}
HRESULT STDMETHODCALLTYPE Direct3D8::EnumAdapterModes(UINT Adapter, UINT Mode, D3DDISPLAYMODE *pMode)
{
	if (pMode == nullptr || !(Adapter < CurrentAdapterCount && Mode < CurrentAdapterModeCount[Adapter]))
		return D3DERR_INVALIDCALL;

	pMode->Format = CurrentAdapterModes[Adapter].at(Mode).Format;
	pMode->Height = CurrentAdapterModes[Adapter].at(Mode).Height;
	pMode->RefreshRate = CurrentAdapterModes[Adapter].at(Mode).RefreshRate;
	pMode->Width = CurrentAdapterModes[Adapter].at(Mode).Width;

	return D3D_OK;
}
HRESULT STDMETHODCALLTYPE Direct3D8::GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE *pMode)
{
	return ProxyInterface->GetAdapterDisplayMode(Adapter, pMode);
}
HRESULT STDMETHODCALLTYPE Direct3D8::CheckDeviceType(UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed)
{
	return ProxyInterface->CheckDeviceType(Adapter, CheckType, DisplayFormat, BackBufferFormat, bWindowed);
}
HRESULT STDMETHODCALLTYPE Direct3D8::CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat)
{
	if (CheckFormat == D3DFMT_UYVY ||
		CheckFormat == D3DFMT_YUY2 ||
		CheckFormat == MAKEFOURCC('Y', 'V', '1', '2') ||
		CheckFormat == MAKEFOURCC('N', 'V', '1', '2'))
	{
		return D3DERR_NOTAVAILABLE;
	}

	return ProxyInterface->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);
}
HRESULT STDMETHODCALLTYPE Direct3D8::CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType)
{
	return ProxyInterface->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, nullptr);
}
HRESULT STDMETHODCALLTYPE Direct3D8::CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat)
{
	return ProxyInterface->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);
}
HRESULT STDMETHODCALLTYPE Direct3D8::GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8 *pCaps)
{
	if (pCaps == nullptr)
		return D3DERR_INVALIDCALL;

	D3DCAPS9 DeviceCaps;

	const HRESULT hr = ProxyInterface->GetDeviceCaps(Adapter, DeviceType, &DeviceCaps);
	if (FAILED(hr))
		return hr;

	ConvertCaps(DeviceCaps, *pCaps);

	return D3D_OK;
}
HMONITOR STDMETHODCALLTYPE Direct3D8::GetAdapterMonitor(UINT Adapter)
{
	return ProxyInterface->GetAdapterMonitor(Adapter);
}
HRESULT STDMETHODCALLTYPE Direct3D8::CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS8 *pPresentationParameters, IDirect3DDevice8 **ppReturnedDeviceInterface)
{
	if (pPresentationParameters == nullptr || ppReturnedDeviceInterface == nullptr)
		return D3DERR_INVALIDCALL;

	*ppReturnedDeviceInterface = nullptr;

	// Window positioning
	if (pPresentationParameters->Windowed) {
		HWND hMainWnd = GetParent(hFocusWindow);
		int wx = std::max(0, (GetSystemMetrics(SM_CXSCREEN) - (int)pPresentationParameters->BackBufferWidth) / 2);
		int wy = std::max(0, (GetSystemMetrics(SM_CYSCREEN) - (int)pPresentationParameters->BackBufferHeight) / 2);

		if (Configuration.Borderless) {
			// Remove non-client window parts and move window flush to screen edge / centre if smaller than display
			SetWindowLong(hMainWnd, GWL_STYLE, WS_VISIBLE);
			SetWindowPos(hMainWnd, NULL, wx, wy, pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
		}
		else {
			// Move window to top, with client area centred on one axis
			RECT rect = { wx, wy, int(pPresentationParameters->BackBufferWidth), int(pPresentationParameters->BackBufferHeight) };
			AdjustWindowRect(&rect, GetWindowLong(hMainWnd, GWL_STYLE), FALSE);
			SetWindowPos(hMainWnd, NULL, rect.left, 0, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
		}

		// Ensure that the render window appears on the taskbar, as it is a child window that may become hidden
		LONG style = GetWindowLong(hMainWnd, GWL_EXSTYLE);
		SetWindowLong(hMainWnd, GWL_EXSTYLE, style | WS_EX_APPWINDOW);

		// Windowed mode does not allow multiple frame vsync
		if (Configuration.VWait >= D3DPRESENT_INTERVAL_TWO && Configuration.VWait <= D3DPRESENT_INTERVAL_FOUR) {
			Configuration.VWait = D3DPRESENT_INTERVAL_ONE;
			//LOG::logline("VWait greater than one is not supported in windowed mode.");
		}
	}

	// Convert presentation parameters to DX9
	D3DPRESENT_PARAMETERS pp;

#ifdef MGE_XE
	// MSAA parameters
	D3DMULTISAMPLE_TYPE msaaSamples = (D3DMULTISAMPLE_TYPE)Configuration.AALevel;
	DWORD msaaQuality = 0;

	// Override device parameters
	// Note that Morrowind will look at the modified parameters
	if (pPresentationParameters->Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER) {
		pPresentationParameters->Flags ^= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	}

	pPresentationParameters->MultiSampleType = msaaSamples;
	pPresentationParameters->AutoDepthStencilFormat = (D3DFORMAT)Configuration.ZBufFormat;
	pPresentationParameters->FullScreen_RefreshRateInHz = (!pPresentationParameters->Windowed) ? Configuration.RefreshRate : 0;
	pPresentationParameters->FullScreen_PresentationInterval = (Configuration.VWait == 255) ? D3DPRESENT_INTERVAL_IMMEDIATE : Configuration.VWait;

	pp.BackBufferWidth = pPresentationParameters->BackBufferWidth;
	pp.BackBufferHeight = pPresentationParameters->BackBufferHeight;
	pp.BackBufferFormat = pPresentationParameters->BackBufferFormat;
	pp.BackBufferCount = pPresentationParameters->BackBufferCount;
	pp.MultiSampleType = pPresentationParameters->MultiSampleType;
	pp.MultiSampleQuality = msaaQuality;
	pp.SwapEffect = pPresentationParameters->SwapEffect;
	pp.hDeviceWindow = pPresentationParameters->hDeviceWindow;
	pp.Windowed = pPresentationParameters->Windowed;
	pp.Flags = pPresentationParameters->Flags;
	pp.EnableAutoDepthStencil = pPresentationParameters->EnableAutoDepthStencil;
	pp.AutoDepthStencilFormat = pPresentationParameters->AutoDepthStencilFormat;
	pp.FullScreen_RefreshRateInHz = pPresentationParameters->FullScreen_RefreshRateInHz;
	pp.PresentationInterval = pPresentationParameters->FullScreen_PresentationInterval;

#else

#ifndef D3D8TO9NOLOG
	LOG << "Redirecting '" << "IDirect3D8::CreateDevice" << "(" << this << ", " << Adapter << ", " << DeviceType << ", " << hFocusWindow << ", " << BehaviorFlags << ", " << pPresentationParameters << ", " << ppReturnedDeviceInterface << ")' ..." << std::endl;
#endif

	ConvertPresentParameters(*pPresentationParameters, pp);

	// Get multisample quality level
	if (pp.MultiSampleType != D3DMULTISAMPLE_NONE)
	{
		DWORD QualityLevels = 0;
		if (ProxyInterface->CheckDeviceMultiSampleType(Adapter,
			DeviceType, pp.BackBufferFormat, pp.Windowed,
			pp.MultiSampleType, &QualityLevels) == S_OK &&
			ProxyInterface->CheckDeviceMultiSampleType(Adapter,
				DeviceType, pp.AutoDepthStencilFormat, pp.Windowed,
				pp.MultiSampleType, &QualityLevels) == S_OK)
		{
			pp.MultiSampleQuality = (QualityLevels != 0) ? QualityLevels - 1 : 0;
		}
	}
#endif // MGE_XE
	// Create device in the same manner as the proxy
	IDirect3DDevice9* DeviceInterface = nullptr;
	const HRESULT hr = ProxyInterface->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, &pp, &DeviceInterface);
	if (FAILED(hr))
		return hr;

	*ppReturnedDeviceInterface = factoryProxyDevice(DeviceInterface, (pp.Flags & D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL) != 0);

///////////////////////////////////////

	// Set up default render states
	Configuration.ScaleFilter = (Configuration.AnisoLevel > 0) ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR;

	for (int i = 0; i != 8; ++i) {
		DeviceInterface->SetSamplerState(i, D3DSAMP_MINFILTER, Configuration.ScaleFilter);
		DeviceInterface->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		DeviceInterface->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, Configuration.AnisoLevel);
	}
#ifdef MGE_RTX
	// Set variables dependent on configuration
	DWORD FogPixelMode, FogVertexMode, RangedFog;
	if (Configuration.FogMode == 2) {
		FogVertexMode = D3DFOG_LINEAR;
		FogPixelMode = D3DFOG_NONE;
		RangedFog = 1;
	}
	else if (Configuration.FogMode == 1) {
		FogVertexMode = D3DFOG_LINEAR;
		FogPixelMode = D3DFOG_NONE;
		RangedFog = 0;
	}
	else {
		FogVertexMode = D3DFOG_NONE;
		FogPixelMode = D3DFOG_LINEAR;
		RangedFog = 0;
	}

	DeviceInterface->SetRenderState(D3DRS_FOGVERTEXMODE, FogVertexMode);
	DeviceInterface->SetRenderState(D3DRS_FOGTABLEMODE, FogPixelMode);
	DeviceInterface->SetRenderState(D3DRS_RANGEFOGENABLE, RangedFog);
#endif // MGE_FOG
	DeviceInterface->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, (Configuration.AALevel > 0));

///////////////////////////////////////

	// Set default vertex declaration
	DeviceInterface->SetFVF(D3DFVF_XYZ);

	return D3D_OK;

}

#ifdef MGE_XE
#include "mge/mged3d8device.h"
#endif // MGE_XE


IDirect3DDevice8* Direct3D8::factoryProxyDevice(IDirect3DDevice9* d, bool EnableZBufferDiscarding) {
#ifdef MGE_XE
	return new MGEProxyDevice(d, this, EnableZBufferDiscarding);
#else
	return new Direct3DDevice8(this, d, EnableZBufferDiscarding);
#endif // MGE_XE
}