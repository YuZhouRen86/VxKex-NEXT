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

KXBASEAPI BOOL WINAPI Ext_SetHandleInformation(
	HANDLE	Object,
	DWORD	Mask,
	DWORD	Flags)
{
	BOOL Result = SetHandleInformation(Object, Mask, Flags);
	if (!Result) {
		DWORD Error = GetLastError();
		if (Error == ERROR_INVALID_PARAMETER) {
			DWORD FileType = GetFileType(Object);
			if (FileType == FILE_TYPE_CHAR && Mask == HANDLE_FLAG_INHERIT && Flags == 0) return TRUE;
		}
		SetLastError(Error);
	}
	return Result;
}