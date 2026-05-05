#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI BOOL WINAPI Ext_InitializeCriticalSectionEx(
	IN	PRTL_CRITICAL_SECTION	CriticalSection,
	IN	ULONG					SpinCount,
	IN	ULONG					Flags)
{
	NTSTATUS Status = Ext_RtlInitializeCriticalSectionEx(CriticalSection, SpinCount, Flags);
	if (NT_SUCCESS(Status)) return TRUE;
	else {
        SetLastError(RtlNtStatusToDosError(Status));
		return FALSE;
	}
}

KXBASEAPI NTSTATUS NTAPI Ext_InitializeCriticalSection(
	IN	PRTL_CRITICAL_SECTION	CriticalSection)
{
	return Ext_InitializeCriticalSectionEx(
		CriticalSection,
		0,
		0);
}

KXBASEAPI NTSTATUS NTAPI Ext_InitializeCriticalSectionAndSpinCount(
	IN	PRTL_CRITICAL_SECTION	CriticalSection,
	IN	ULONG					SpinCount)
{
	return Ext_InitializeCriticalSectionEx(
		CriticalSection,
		SpinCount & 0xFFFFFF,
		0);
}