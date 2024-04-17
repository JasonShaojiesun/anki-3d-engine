// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Gr/Common.h>
#include <AnKi/Util/Logger.h>
#include <AnKi/Gr/Common/BackendCommon.h>
#include <string>
#include <locale>
#include <codecvt>

#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <AnKi/Util/CleanupWindows.h>

using Microsoft::WRL::ComPtr;

namespace anki {

/// @addtogroup directx
/// @{

#define ANKI_D3D_LOGI(...) ANKI_LOG("D3D", kNormal, __VA_ARGS__)
#define ANKI_D3D_LOGE(...) ANKI_LOG("D3D", kError, __VA_ARGS__)
#define ANKI_D3D_LOGW(...) ANKI_LOG("D3D", kWarning, __VA_ARGS__)
#define ANKI_D3D_LOGF(...) ANKI_LOG("D3D", kFatal, __VA_ARGS__)
#define ANKI_D3D_LOGV(...) ANKI_LOG("D3D", kVerbose, __VA_ARGS__)

#define ANKI_D3D_SELF(class_) class_& self = *static_cast<class_*>(this)
#define ANKI_D3D_SELF_CONST(class_) const class_& self = *static_cast<const class_*>(this)

#define ANKI_D3D_CHECKF(x) \
	do \
	{ \
		HRESULT rez; \
		if((rez = (x)) < 0) [[unlikely]] \
		{ \
			ANKI_D3D_LOGF("D3D function failed (HRESULT: %d): %s", rez, #x); \
		} \
	} while(0)

#define ANKI_D3D_CHECK(x) \
	do \
	{ \
		HRESULT rez; \
		if((rez = (x)) < 0) [[unlikely]] \
		{ \
			ANKI_D3D_LOGE("D3D function failed (HRESULT: %d): %s", rez, #x); \
			return Error::kFunctionFailed; \
		} \
	} while(0)

/// Backend specific constants
constexpr U32 kMaxRtvDescriptors = 128;

inline std::string ws2s(const std::wstring& wstr)
{
	const int slength = int(wstr.length());
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), slength, 0, 0, 0, 0);
	std::string r(len, '\0');
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), slength, &r[0], len, 0, 0);
	return r;
}

template<typename T>
void safeRelease(T*& p)
{
	if(p)
	{
		p->Release();
		p = nullptr;
	}
}

ID3D12Device& getDevice();

GrManagerImpl& getGrManagerImpl();
/// @}

} // end namespace anki
