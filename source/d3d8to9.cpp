/**
 * Copyright (C) 2015 Patrick Mours
 * License: https://github.com/crosire/d3d8to9#license
 */

#include "d3d8to9.hpp"

// Very simple logging for the purpose of debugging only.
std::ofstream LOG;

extern "C" Direct3D8 *WINAPI Direct3DCreate8(UINT SDKVersion)
{
	LOG.open("d3d8.log", std::ios::trunc);

	if (!LOG.is_open())
	{
		MessageBoxA(nullptr, "Failed to open debug log file \"d3d8.log\"!", nullptr, MB_ICONWARNING);
	}

	LOG << "Redirecting '" << "Direct3DCreate8" << "(" << SDKVersion << ")' ..." << std::endl;
	LOG << "> Passing on to 'Direct3DCreate9':" << std::endl;

	IDirect3D9 *const d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (d3d == nullptr)
	{
		return nullptr;
	}

	return new Direct3D8(d3d);
}
