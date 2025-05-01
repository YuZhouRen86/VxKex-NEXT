#include "buildcfg.h"
#include "kxdxp.h"

KXDXAPI HRESULT WINAPI DXGIGetDebugInterface1(
	UINT		Flags,
	REFIID		riid,
	OUT	PPVOID	pDebug)
{
	return E_NOINTERFACE;
}
