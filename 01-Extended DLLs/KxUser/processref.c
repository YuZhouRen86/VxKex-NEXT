#include "buildcfg.h"
#include "kxuserp.h"

STATIC IUnknown *ProcessReference = NULL;

KXUSERAPI VOID WINAPI GetProcessReference(
	OUT	IUnknown	**punk)
{
	if (!punk) return;
	*punk = ProcessReference;
	if (*punk) IUnknown_AddRef(*punk);
}

KXUSERAPI VOID WINAPI SetProcessReference(
	IN	IUnknown	*punk OPTIONAL)
{
	ProcessReference = punk;
}