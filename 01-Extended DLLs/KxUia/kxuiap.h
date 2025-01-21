#include "buildcfg.h"
#include <KexComm.h>

EXTERN PKEX_PROCESS_DATA KexData;

typedef enum _NotificationKind {
	NotificationKind_ItemAdded = 0,
	NotificationKind_ItemRemoved = 1,
	NotificationKind_ActionCompleted = 2,
	NotificationKind_ActionAborted = 3,
	NotificationKind_Other = 4
} NotificationKind;

typedef enum _NotificationProcessing {
	NotificationProcessing_ImportantAll = 0,
	NotificationProcessing_ImportantMostRecent = 1,
	NotificationProcessing_All = 2,
	NotificationProcessing_MostRecent = 3,
	NotificationProcessing_CurrentThenMostRecent = 4
} NotificationProcessing;

KXUIAAPI HRESULT WINAPI UiaRaiseNotificationEvent(
	IN	DWORD					*provider,
	NotificationKind		notificationKind,
	NotificationProcessing	notificationProcessing,
	IN	BSTR					displayString	OPTIONAL,
	IN	BSTR					activityId);