#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI BOOL WINAPI CompareObjectHandles(
	IN	HANDLE	FirstObjectHandle,
	IN	HANDLE	SecondObjectHandle)
{
	NTSTATUS Status;

	Status = NtCompareObjects(FirstObjectHandle, SecondObjectHandle);
	if (NT_SUCCESS(Status)) return TRUE;

	if (Status == STATUS_NOT_SAME_OBJECT) {
		// BaseSetLastNTError does not recognize this new NTSTATUS value, which was
		// added in Windows 10.
		RtlSetLastWin32Error(ERROR_NOT_SAME_OBJECT);
	} else {
		BaseSetLastNTError(Status);
	}

	return FALSE;
}