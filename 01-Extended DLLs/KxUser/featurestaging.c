#include "buildcfg.h"
#include "kxuserp.h"

KXUSERAPI FEATURE_ENABLED_STATE WINAPI GetFeatureEnabledState(
	UINT32				featureId,
	FEATURE_CHANGE_TIME	changeTime)
{
	return FEATURE_ENABLED_STATE_DEFAULT;
}

KXUSERAPI UINT32 WINAPI GetFeatureVariant(
	UINT32				featureId,
	FEATURE_CHANGE_TIME	changeTime,
	OUT	UINT32			*payloadId,
	OUT	BOOL			*hasNotification)
{
	return FEATURE_CHANGE_TIME_READ;
}

KXUSERAPI VOID WINAPI RecordFeatureError(
	UINT32					featureId,
	IN	const FEATURE_ERROR	*error)
{
	return;
}

KXUSERAPI VOID WINAPI RecordFeatureUsage(
	UINT32		featureId,
	UINT32		kind,
	UINT32		addend,
	IN	PCSTR	originName)
{
	return;
}

KXUSERAPI VOID WINAPI SubscribeFeatureStateChangeNotification(
	OUT	FEATURE_STATE_CHANGE_SUBSCRIPTION	*subscription,
	IN	PFEATURE_STATE_CHANGE_CALLBACK		callback,
	IN	void								*context	OPTIONAL)
{
	return;
}

KXUSERAPI VOID WINAPI UnsubscribeFeatureStateChangeNotification(
	IN	FEATURE_STATE_CHANGE_SUBSCRIPTION	*subscription)
{
	return;
}