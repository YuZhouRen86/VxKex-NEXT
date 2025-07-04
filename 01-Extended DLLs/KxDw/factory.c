#include "buildcfg.h"
#include "kxdwp.h"

KXDWAPI HRESULT WINAPI Ext_DWriteCreateFactory(
	IN	DWRITE_FACTORY_TYPE	factoryType,
	IN	REFIID              iid,
	OUT	IUnknown            **factory)
{
	if (IsEqualIID(iid, &IID_IDWriteFactory)) {
		unless (KexData->IfeoParameters.DisableAppSpecific) {
			if ((KexData->Flags & KEXDATA_FLAG_CHROMIUM)) {
				return DWriteCoreCreateFactory(factoryType, iid, factory);
			}
		}
		return DWriteCreateFactory(factoryType, iid, factory);
	} else {
		unless (KexData->IfeoParameters.DisableAppSpecific) {
			if (AshExeBaseNameIs(L"Zps.exe")) {
				return DWriteCreateFactory(factoryType, iid, factory);
			}
		}
		return DWriteCoreCreateFactory(factoryType, iid, factory);
	}
}