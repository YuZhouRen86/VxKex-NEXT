#include "buildcfg.h"
#include "kexdllp.h"

KEXAPI NTSTATUS NTAPI NtQueryWnfStateData(
	PULONG		ChangeStamp,
	ULONGLONG	StateName,
	PVOID		Callback,
	PVOID		CallbackContext,
	PULONG		TypeId)
{
	return STATUS_NOT_IMPLEMENTED;
}