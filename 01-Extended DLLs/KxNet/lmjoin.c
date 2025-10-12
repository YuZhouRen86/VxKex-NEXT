#include "buildcfg.h"
#include "kxnetp.h"

KXNETAPI VOID WINAPI NetFreeAadJoinInformation(
	IN	PVOID	JoinInfo OPTIONAL)
{
	return;
}

KXNETAPI HRESULT WINAPI NetGetAadJoinInformation(
	IN	LPCWSTR		TenantId OPTIONAL,
	OUT PPVOID		JoinInfo)
{
	if (!JoinInfo) return E_INVALIDARG;
	*JoinInfo = NULL;
	return S_OK;
}