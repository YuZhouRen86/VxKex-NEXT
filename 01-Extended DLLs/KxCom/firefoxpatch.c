#include "buildcfg.h"
#include "kxcomp.h"
#include <ShlObj.h>
#include <KexDll.h>

HRESULT WINAPI Ext_CoCreateInstance(
	IN	REFCLSID	rclsid,
	IN	LPUNKNOWN	pUnkOuter,
	IN	DWORD		dwClsContext,
	IN	REFIID		riid,
	OUT	LPVOID		*ppv)
{
	unless (KexData->IfeoParameters.DisableAppSpecific){
		if (AshExeBaseNameIs(L"firefox.exe")) {
			if ((IsEqualCLSID(rclsid, &CLSID_DestinationList) && pUnkOuter == NULL && dwClsContext == CLSCTX_INPROC_SERVER && IsEqualIID(riid, &IID_ICustomDestinationList)) || (pUnkOuter == NULL && dwClsContext == CLSCTX_INPROC_SERVER && IsEqualIID(riid, &IID_IObjectCollection))) return E_NOTIMPL;
		}
	}
	return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}