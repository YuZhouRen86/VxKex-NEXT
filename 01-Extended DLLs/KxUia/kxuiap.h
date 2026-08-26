#include "buildcfg.h"
#include <KexComm.h>

EXTERN PKEX_PROCESS_DATA KexData;

typedef enum _NotificationKind {
	NotificationKind_ItemAdded,
	NotificationKind_ItemRemoved,
	NotificationKind_ActionCompleted,
	NotificationKind_ActionAborted,
	NotificationKind_Other
} TYPEDEF_TYPE_NAME(NotificationKind);

typedef enum _NotificationProcessing {
	NotificationProcessing_ImportantAll,
	NotificationProcessing_ImportantMostRecent,
	NotificationProcessing_All,
	NotificationProcessing_MostRecent,
	NotificationProcessing_CurrentThenMostRecent
} TYPEDEF_TYPE_NAME(NotificationProcessing);

typedef struct _UiaChangeInfo {
	int		uiaId;
	VARIANT	payload;
	VARIANT	extraInfo;
} TYPEDEF_TYPE_NAME(UiaChangeInfo);