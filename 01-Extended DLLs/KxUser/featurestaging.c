#include "buildcfg.h"
#include "kxuserp.h"

VOID SubscribeFeatureStateChangeNotification(
	OUT	FEATURE_STATE_CHANGE_SUBSCRIPTION	*subscription,
	IN	PFEATURE_STATE_CHANGE_CALLBACK		callback,
	IN	void								*context	OPTIONAL)
{
	return;
}

VOID UnsubscribeFeatureStateChangeNotification(
	IN	FEATURE_STATE_CHANGE_SUBSCRIPTION	*subscription)
{
	return;
}