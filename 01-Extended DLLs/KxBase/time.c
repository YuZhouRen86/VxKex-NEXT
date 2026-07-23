#include "buildcfg.h"
#include "kxbasep.h"

//
// If strong SharedUserData spoofing is enabled, this function
// supersedes KernelBase!GetSystemTimeAsFileTime because the original
// function reads system time from SharedUserData.
//
KXBASEAPI VOID WINAPI KxBasepGetSystemTimeAsFileTimeHook(
	OUT	PFILETIME	SystemTimeAsFileTime)
{
	ASSERT (KexData->IfeoParameters.StrongVersionSpoof & KEX_STRONGSPOOF_SHAREDUSERDATA);
	NtQuerySystemTime((PLONGLONG) SystemTimeAsFileTime);
}

//
// Same as above but this function supersedes GetSystemTime when doing
// SharedUserData-based version spoofing.
//
KXBASEAPI VOID WINAPI KxBasepGetSystemTimeHook(
	OUT	PSYSTEMTIME	SystemTime)
{
	LONGLONG Time;
	TIME_FIELDS TimeFields;

	ASSERT (KexData->IfeoParameters.StrongVersionSpoof & KEX_STRONGSPOOF_SHAREDUSERDATA);

	NtQuerySystemTime(&Time);
	RtlTimeToTimeFields(&Time, &TimeFields);

	//
	// Annoyingly, the TIME_FIELDS structure is not directly compatible with
	// the SYSTEMTIME structure...
	//

	SystemTime->wYear			= TimeFields.Year;
	SystemTime->wMonth			= TimeFields.Month;
	SystemTime->wDay			= TimeFields.Day;
	SystemTime->wDayOfWeek		= TimeFields.Weekday;
	SystemTime->wHour			= TimeFields.Hour;
	SystemTime->wMinute			= TimeFields.Minute;
	SystemTime->wSecond			= TimeFields.Second;
	SystemTime->wMilliseconds	= TimeFields.Milliseconds;
}

KXBASEAPI VOID WINAPI GetSystemTimePreciseAsFileTime(
	OUT	PFILETIME	SystemTimeAsFileTime)
{
	*(PLONGLONG) SystemTimeAsFileTime = KexRtlGetSystemTimePrecise();
}

KXBASEAPI VOID WINAPI QueryUnbiasedInterruptTimePrecise(
	OUT	PULONGLONG	UnbiasedInterruptTimePrecise)
{
	QueryUnbiasedInterruptTime(UnbiasedInterruptTimePrecise);
}