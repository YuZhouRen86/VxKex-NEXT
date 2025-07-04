#include "buildcfg.h"
#include "kxcomp.h"
#include <ShlObj.h>

HRESULT WINAPI Ext_CoCreateInstance(
	IN	REFCLSID	rclsid,
	IN	LPUNKNOWN	pUnkOuter,
	IN	DWORD		dwClsContext,
	IN	REFIID		riid,
	OUT	LPVOID		*ppv)
{
	unless (KexData->IfeoParameters.DisableAppSpecific) {
		if (!NtCurrentTeb()->ReservedForOle &&
			(KexData->Flags & KEXDATA_FLAG_FIREFOX) &&
			pUnkOuter == NULL &&
			dwClsContext == CLSCTX_INPROC_SERVER &&
			((IsEqualCLSID(rclsid, &CLSID_DestinationList) &&
			IsEqualIID(riid, &IID_ICustomDestinationList)) ||
			IsEqualIID(riid, &IID_IObjectCollection))) {

			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		}
	}
	return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}