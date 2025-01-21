#include "buildcfg.h"
#include "kxdxp.h"
#include <d3d11.h>

#if defined(KEX_TARGET_TYPE_EXE) || defined(KEX_TARGET_TYPE_DLL)
#  if defined(KEX_ARCH_X64)
#    pragma comment(lib, "d3d11_x64.lib")
#  elif defined(KEX_ARCH_X86)
#    pragma comment(lib, "d3d11_x86.lib")
#  endif
#endif

KXDXAPI HRESULT WINAPI D3D11On12CreateDevice(
	IN	IUnknown				*pDevice,
	UINT						Flags,
	IN	const D3D_FEATURE_LEVEL	*pFeatureLevels	OPTIONAL,
	UINT						FeatureLevels,
	IN	IUnknown				* const	*ppCommandQueues	OPTIONAL,
	UINT						NumQueues,
	UINT						NodeMask,
	OUT	ID3D11Device			**ppDevice OPTIONAL,
	OUT	ID3D11DeviceContext		**ppImmediateContext	OPTIONAL,
	OUT	D3D_FEATURE_LEVEL		*pChosenFeatureLevel	OPTIONAL)
{
	return D3D11CreateDevice((IDXGIAdapter*)pDevice, D3D_DRIVER_TYPE_HARDWARE, NULL, Flags, pChosenFeatureLevel, FeatureLevels, D3D11_SDK_VERSION, ppDevice, pChosenFeatureLevel, ppImmediateContext);
}