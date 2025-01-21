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