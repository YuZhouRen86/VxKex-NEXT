#include "buildcfg.h"
#include "kxuiap.h"

KXUIAAPI HRESULT WINAPI UiaRaiseNotificationEvent(
	IN	DWORD					*provider,
	NotificationKind			notificationKind,
	NotificationProcessing		notificationProcessing,
	IN	BSTR					displayString	OPTIONAL,
	IN	BSTR					activityId)
{
	return E_NOTIMPL;
}

KXUIAAPI HRESULT WINAPI UiaDisconnectProvider(
	IN		IUnknown					*Provider)
{
	return E_NOTIMPL;
}

//
// Some Unity games require this.
//
KXUIAAPI HRESULT WINAPI UiaDisconnectAllProviders(
	VOID)
{
	return E_NOTIMPL;
}