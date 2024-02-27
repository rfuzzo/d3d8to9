/**
 * Copyright (C) 2015 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/d3d8to9#license
 */

#include "d3dx9.hpp"
#include "d3d8to9.hpp"
#ifdef MGE_XE
#include "mge/configuration.h"
#include "mge/mgeversion.h"
#include "mge/mwinitpatch.h"
#include "mwse/mgebridge.h"
#include "support/winheader.h"
#include "support/log.h"
#endif // MGE_XE


#ifndef D3DX9
PFN_D3DXAssembleShader D3DXAssembleShader = nullptr;
PFN_D3DXDisassembleShader D3DXDisassembleShader = nullptr;
PFN_D3DXLoadSurfaceFromSurface D3DXLoadSurfaceFromSurface = nullptr;
#endif // D3DX9

#ifndef D3D8TO9NOLOG
 // Very simple logging for the purpose of debugging only.
std::ofstream LOG;
#endif

#ifdef MGE_XE
extern void* CreateD3DWrapper(UINT);
extern void* CreateInputWrapper(void*);

static FARPROC getProc1(const char* lib, const char* funcname);
static void setDPIScalingAware();

static const char* welcomeMessage = XE_VERSION_STRING;
static bool isMW;

extern "C" BOOL _stdcall DllMain(HANDLE hModule, DWORD reason, void* unused) {
    if (reason != DLL_PROCESS_ATTACH) {
        return true;
    }

    // Check if MW is in-process, avoid hooking the CS
    isMW = bool(GetModuleHandle("Morrowind.exe"));

    if (isMW) {
        LOG::open("mgeXE.log");
        LOG::logline(welcomeMessage);

        setDPIScalingAware();

        if (!Configuration.LoadSettings()) {
            LOG::logline("Error: MGE XE is not configured. MGE XE will be disabled for this session.");
            isMW = false;
            return true;
        }

        if (Configuration.MGEFlags & MGE_DISABLED) {
            // Signal that DirectX proxies should not load
            isMW = false;
        }

        if ((Configuration.MGEFlags & MGE_DISABLED) || Configuration.OnlyProxyD3D8To9) {
            // Make Morrowind apply UI scaling, as the D3D proxy is not available to do it
            //MWInitPatch::patchUIInit();
        }

        if (~Configuration.MGEFlags & MWSE_DISABLED) {
            // Load MWSE dll, it injects by itself
            HMODULE dll = LoadLibraryA("MWSE.dll");

            if (dll) {
                if ((~Configuration.MGEFlags & MGE_DISABLED) && !Configuration.OnlyProxyD3D8To9) {
                    // MWSE-MGE integration
                    MWSE_MGEPlugin::init(dll);
                }
                LOG::logline("MWSE.dll injected.");
            }
            else {
                LOG::logline("MWSE.dll failed to load.");
            }
        }
        else {
            LOG::logline("MWSE is disabled.");
        }

        // Early startup patches
        if (Configuration.MGEFlags & SKIP_INTRO) {
            //MWInitPatch::disableIntroMovies();
        }

        if (Configuration.UseDefaultTexturePool) {
            MWInitPatch::patchTextureLoading();
        }

        MWInitPatch::patchFrameTimer();
    }

    // Load extender for CS, if Construction Set detected
    bool isCS = bool(GetModuleHandle("TES Construction Set.exe"));
    if (isCS) {
        // Load CSSE dll, it injects by itself
        LoadLibraryA("CSSE.dll");
    }

    return true;
}

extern "C" HRESULT _stdcall FakeDirectInputCreate(HINSTANCE a, DWORD b, REFIID c, void** d, void* e) {
    typedef HRESULT(_stdcall* DInputProc) (HINSTANCE, DWORD, REFIID, void**, void*);
    DInputProc func = (DInputProc)getProc1("dinput8.dll", "DirectInput8Create");

    void* dinput = 0;
    HRESULT hr = (func)(a, b, c, &dinput, e);

    if (hr == S_OK) {
        if (isMW) {
            *d = CreateInputWrapper(dinput);
        }
        else {
            *d = dinput;
        }
    }

    return hr;
}

FARPROC getProc1(const char* lib, const char* funcname) {
    // Get the address of a single function from a dll
    char syspath[MAX_PATH], path[MAX_PATH];
    GetSystemDirectoryA(syspath, sizeof(syspath));

    int str_sz = std::snprintf(path, sizeof(path), "%s\\%s", syspath, lib);
    if (str_sz >= sizeof(path)) {
        return NULL;
    }

    HMODULE dll = LoadLibraryA(path);
    if (dll == NULL) {
        return NULL;
    }

    return GetProcAddress(dll, funcname);
}

void setDPIScalingAware() {
    // Prevent DPI scaling from affecting chosen window size
    typedef BOOL(WINAPI* dpiProc)();
    dpiProc SetProcessDPIAware = (dpiProc)getProc1("user32.dll", "SetProcessDPIAware");

    if (SetProcessDPIAware) {
        SetProcessDPIAware();
    }
}

#endif // MGE_XE


extern "C" IDirect3D8 *WINAPI Direct3DCreate8(UINT SDKVersion)
{
#ifndef D3D8TO9NOLOG
	static bool LogMessageFlag = true;

	if (!LOG.is_open())
	{
		LOG.open("d3d8.log", std::ios::trunc);
	}

	if (!LOG.is_open() && LogMessageFlag)
	{
		LogMessageFlag = false;
		MessageBox(nullptr, TEXT("Failed to open debug log file \"d3d8.log\"!"), nullptr, MB_ICONWARNING);
	}

	LOG << "Redirecting '" << "Direct3DCreate8" << "(" << SDKVersion << ")' ..." << std::endl;
	LOG << "> Passing on to 'Direct3DCreate9':" << std::endl;
#endif

	IDirect3D9 *const d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (d3d == nullptr)
	{
		return nullptr;
	}

	// Load D3DX
#ifndef D3DX9
	if (!D3DXAssembleShader || !D3DXDisassembleShader || !D3DXLoadSurfaceFromSurface)
	{
		const HMODULE module = LoadLibrary(TEXT("d3dx9_43.dll"));

		if (module != nullptr)
		{
			D3DXAssembleShader = reinterpret_cast<PFN_D3DXAssembleShader>(GetProcAddress(module, "D3DXAssembleShader"));
			D3DXDisassembleShader = reinterpret_cast<PFN_D3DXDisassembleShader>(GetProcAddress(module, "D3DXDisassembleShader"));
			D3DXLoadSurfaceFromSurface = reinterpret_cast<PFN_D3DXLoadSurfaceFromSurface>(GetProcAddress(module, "D3DXLoadSurfaceFromSurface"));
		}
		else
		{
#ifndef D3D8TO9NOLOG
			LOG << "Failed to load d3dx9_43.dll! Some features will not work correctly." << std::endl;
#endif
			if (MessageBox(nullptr, TEXT(
					"Failed to load d3dx9_43.dll! Some features will not work correctly.\n\n"
					"It's required to install the \"Microsoft DirectX End-User Runtime\" in order to use d3d8to9, or alternatively get the DLLs from this NuGet package:\nhttps://www.nuget.org/packages/Microsoft.DXSDK.D3DX\n\n"
					"Please click \"OK\" to open the official download page or \"Cancel\" to continue anyway."), nullptr, MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND | MB_OKCANCEL | MB_DEFBUTTON1) == IDOK)
			{
				ShellExecute(nullptr, TEXT("open"), TEXT("https://www.microsoft.com/download/details.aspx?id=35"), nullptr, nullptr, SW_SHOW);

				return nullptr;
			}
		}
	}
#endif // D3DX9

	return new Direct3D8(d3d);
}

//-----------------------------------------------------------------------------
// Proxy methods
//IDirect3DTexture8* Direct3DDevice8::factoryProxyTexture(IDirect3DTexture9* tex) {
//    return new ProxyTexture(tex, this);
//}
//IDirect3DSurface8* Direct3DDevice8::factoryProxySurface(IDirect3DSurface9* surface) {
//    return new ProxySurface(surface, this);
//}