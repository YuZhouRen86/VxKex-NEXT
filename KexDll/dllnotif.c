///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllnotif.c
//
// Abstract:
//
//     Contains a routine which is called after a DLL is mapped into the
//     process, but before its imports are resolved. This is the opportune
//     moment to rewrite DLL imports.
//
// Author:
//
//     vxiiduu (18-Oct-2022)
//
// Revision History:
//
//     vxiiduu              18-Oct-2022  Initial creation.
//     vxiiduu              23-Feb-2024  Change wording from "loaded" to "mapped"
//                                       in order to better reflect reality.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

NTSYSCALLAPI NTSTATUS NTAPI NtOpenSymbolicLinkObject(
	PHANDLE				LinkHandle,
	ACCESS_MASK			DesiredAccess,
	POBJECT_ATTRIBUTES	ObjectAttributes);

NTSYSCALLAPI NTSTATUS NTAPI NtQuerySymbolicLinkObject(
	HANDLE			LinkHandle,
	PUNICODE_STRING	LinkTarget,
	PULONG			ReturnedLength);

STATIC BOOL GetDriveDevicePath(
	WCHAR   Drive,
	PWSTR   *OutDevicePath)
{
	UNICODE_STRING DosName;
	UNICODE_STRING Target;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE LinkHandle = NULL;
	PWSTR TargetBuffer = NULL;
	ULONG ReturnedLength;
	NTSTATUS Status;
	HANDLE Heap = GetProcessHeap();
	BOOL Success = FALSE;
	SIZE_T TargetLen;
	WCHAR DosNameBuffer[8];

	*OutDevicePath = NULL;

	DosNameBuffer[0] = L'\\';
	DosNameBuffer[1] = L'?';
	DosNameBuffer[2] = L'?';
	DosNameBuffer[3] = L'\\';
	DosNameBuffer[4] = Drive;
	DosNameBuffer[5] = L':';
	DosNameBuffer[6] = L'\0';

	RtlInitUnicodeString(&DosName, DosNameBuffer);
	InitializeObjectAttributes(&ObjectAttributes, &DosName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtOpenSymbolicLinkObject(&LinkHandle, GENERIC_READ, &ObjectAttributes);
	if (!NT_SUCCESS(Status))
		goto Cleanup;

	Target.Buffer = NULL;
	Target.Length = 0;
	Target.MaximumLength = 0;
	Status = NtQuerySymbolicLinkObject(LinkHandle, &Target, &ReturnedLength);
	if (Status != STATUS_BUFFER_TOO_SMALL && !NT_SUCCESS(Status))
		goto Cleanup;

	TargetBuffer = (PWSTR)RtlAllocateHeap(Heap, 0, ReturnedLength + sizeof(WCHAR));
	if (TargetBuffer == NULL)
		goto Cleanup;

	Target.Buffer = TargetBuffer;
	Target.Length = 0;
	Target.MaximumLength = (USHORT)(ReturnedLength + sizeof(WCHAR));

	Status = NtQuerySymbolicLinkObject(LinkHandle, &Target, &ReturnedLength);
	if (!NT_SUCCESS(Status))
		goto Cleanup;

	if (StringCchLength(TargetBuffer, STRSAFE_MAX_CCH, &TargetLen) != S_OK)
		goto Cleanup;

	*OutDevicePath = TargetBuffer;
	TargetBuffer = NULL;
	Success = TRUE;

Cleanup:
	if (TargetBuffer != NULL)
		RtlFreeHeap(Heap, 0, TargetBuffer);
	if (LinkHandle != NULL)
		NtClose(LinkHandle);

	return Success;
}

STATIC BOOL DevicePathToDosPath(
	PUNICODE_STRING DevicePath)
{
	HANDLE Heap = GetProcessHeap();
	PWSTR DriveDevicePath = NULL;
	PWSTR NewBuffer = NULL;
	PCWSTR SrcPtr = NULL;
	WCHAR Drive;
	INT DriveLen = 0;
	SIZE_T RemainLen = 0;
	SIZE_T NewLen = 0;
	INT Index;
	BOOL Success = FALSE;
	BOOL BestFound = FALSE;
	WCHAR BestDrive = L'\0';
	INT BestDriveLen = 0;
	PWSTR Remain = NULL;
	ULONG DevicePrefixLen = 8;	// L"\\Device\\"
	ULONG MupPrefixLen = 12;	// L"\\Device\\Mup\\"
	BOOL IsMup = FALSE;

	if (DevicePath == NULL || DevicePath->Buffer == NULL || DevicePath->Length == 0)
		return FALSE;

	SrcPtr = DevicePath->Buffer;
	for (Index = 0; Index < (INT)DevicePrefixLen; Index++) {
		WCHAR c1 = SrcPtr[Index];
		WCHAR c2 = L"\\Device\\"[Index];
		if (c1 >= L'a' && c1 <= L'z') c1 -= (L'a' - L'A');
		if (c2 >= L'a' && c2 <= L'z') c2 -= (L'a' - L'A');
		if (c1 != c2)
			return FALSE;
	}

	if (DevicePath->Length >= MupPrefixLen * sizeof(WCHAR)) {
		IsMup = TRUE;
		for (Index = 0; Index < (INT)MupPrefixLen; Index++) {
			WCHAR c1 = DevicePath->Buffer[Index];
			WCHAR c2 = L"\\Device\\Mup\\"[Index];
			if (c1 >= L'a' && c1 <= L'z') c1 -= (L'a' - L'A');
			if (c2 >= L'a' && c2 <= L'z') c2 -= (L'a' - L'A');
			if (c1 != c2) {
				IsMup = FALSE;
				break;
			}
		}

		if (IsMup) {
			Remain = DevicePath->Buffer + MupPrefixLen;
			if (StringCchLength(Remain, STRSAFE_MAX_CCH, &RemainLen) == S_OK) {
				NewLen = 2 + RemainLen + 1;
				NewBuffer = (PWSTR)RtlAllocateHeap(Heap, 0, NewLen * sizeof(WCHAR));
				if (NewBuffer != NULL) {
					if (StringCchCopy(NewBuffer, NewLen, L"\\\\") == S_OK &&
						StringCchCat(NewBuffer, NewLen, Remain) == S_OK) {
						RtlFreeHeap(Heap, 0, DevicePath->Buffer);
						DevicePath->Buffer = NewBuffer;
						DevicePath->Length = (USHORT)((NewLen - 1) * sizeof(WCHAR));
						DevicePath->MaximumLength = (USHORT)(NewLen * sizeof(WCHAR));
						NewBuffer = NULL;
						Success = TRUE;
						goto Cleanup;
					}
					RtlFreeHeap(Heap, 0, NewBuffer);
					NewBuffer = NULL;
				}
			}
		}
	}

	for (Drive = L'A'; Drive <= L'Z'; Drive++) {
		if (!GetDriveDevicePath(Drive, &DriveDevicePath))
			continue;

		if (StringCchLength(DriveDevicePath, STRSAFE_MAX_CCH, (SIZE_T*)&DriveLen) != S_OK) {
			RtlFreeHeap(Heap, 0, DriveDevicePath);
			DriveDevicePath = NULL;
			continue;
		}

		if (DriveLen * (INT)sizeof(WCHAR) <= DevicePath->Length) {
			if (_wcsnicmp(DevicePath->Buffer, DriveDevicePath, DriveLen) == 0) {
				if (!BestFound || DriveLen > BestDriveLen) {
					BestDrive = Drive;
					BestDriveLen = DriveLen;
					BestFound = TRUE;
				}
			}
		}

		RtlFreeHeap(Heap, 0, DriveDevicePath);
		DriveDevicePath = NULL;
	}

	if (!BestFound)
		goto Cleanup;

	Remain = DevicePath->Buffer + BestDriveLen;
	if (StringCchLength(Remain, STRSAFE_MAX_CCH, &RemainLen) != S_OK)
		goto Cleanup;

	NewLen = 2 + RemainLen + 1;
	NewBuffer = (PWSTR)RtlAllocateHeap(Heap, 0, NewLen * sizeof(WCHAR));
	if (NewBuffer == NULL)
		goto Cleanup;

	if (StringCchCopy(NewBuffer, NewLen, L"X:") != S_OK)
		goto Cleanup;
	NewBuffer[0] = BestDrive;

	if (StringCchCat(NewBuffer, NewLen, Remain) != S_OK)
		goto Cleanup;

	RtlFreeHeap(Heap, 0, DevicePath->Buffer);
	DevicePath->Buffer = NewBuffer;
	DevicePath->Length = (USHORT)((NewLen - 1) * sizeof(WCHAR));
	DevicePath->MaximumLength = (USHORT)(NewLen * sizeof(WCHAR));

	NewBuffer = NULL;
	Success = TRUE;

Cleanup:
	if (DriveDevicePath != NULL)
		RtlFreeHeap(Heap, 0, DriveDevicePath);
	if (NewBuffer != NULL)
		RtlFreeHeap(Heap, 0, NewBuffer);

	return Success;
}

STATIC NTSTATUS GetDllFullName(
	PVOID			DllBase,
	PUNICODE_STRING	FullDllName)
{
	PBYTE Buffer;
	HANDLE hProcess = GetCurrentProcess();
	SIZE_T ReturnLength = 0;

	NTSTATUS Status = NtQueryVirtualMemory(
		hProcess,
		DllBase,
		MemoryMappedFilenameInformation,
		NULL,
		0,
		&ReturnLength);

	if (Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_INFO_LENGTH_MISMATCH) {
		return Status;
	}

	Buffer = (PBYTE)SafeAlloc(BYTE, ReturnLength);
	if (!Buffer) {
		return STATUS_NO_MEMORY;
	}

	Status = NtQueryVirtualMemory(
		hProcess,
		DllBase,
		MemoryMappedFilenameInformation,
		Buffer,
		ReturnLength,
		&ReturnLength);

	if (NT_SUCCESS(Status)) {
		PUNICODE_STRING pResult = (PUNICODE_STRING)Buffer;
		USHORT PathLength = pResult->Length;
		USHORT PathMaxLength = PathLength + sizeof(WCHAR);

		PWCHAR PathBuffer = (PWCHAR)SafeAlloc(WCHAR, PathMaxLength / sizeof(WCHAR));
		if (PathBuffer) {
			RtlCopyMemory(PathBuffer, pResult->Buffer, PathLength);
			PathBuffer[PathLength / sizeof(WCHAR)] = L'\0';

			FullDllName->Buffer = PathBuffer;
			FullDllName->Length = PathLength;
			FullDllName->MaximumLength = PathMaxLength;
			DevicePathToDosPath(FullDllName);
		}
		else {
			Status = STATUS_NO_MEMORY;
		}
	}

	SafeFree(Buffer);
	return Status;
}

NTSTATUS NTAPI Ext_NtMapViewOfSection(
	IN		HANDLE						SectionHandle,
	IN		HANDLE						ProcessHandle,
	IN OUT	PPVOID						BaseAddress OPTIONAL,
	IN		ULONG						ZeroBits OPTIONAL,
	IN		SIZE_T						CommitSize,
	IN OUT	PLONGLONG					SectionOffset OPTIONAL,
	IN OUT	PSIZE_T						ViewSize,
	IN		SECTION_INHERIT				InheritDisposition,
	IN		ULONG						AllocationType,
	IN		ULONG						MemoryProtection)
{
	NTSTATUS Status = KexNtMapViewOfSection(
		SectionHandle,
		ProcessHandle,
		BaseAddress,
		ZeroBits,
		CommitSize,
		SectionOffset,
		ViewSize,
		InheritDisposition,
		AllocationType,
		MemoryProtection);
	try {
		if (DllLoaderInitialized && NT_SUCCESS(Status) && ProcessHandle == GetCurrentProcess() && BaseAddress && *BaseAddress) {
			PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)*BaseAddress;
			if (ViewSize && *ViewSize >= sizeof(IMAGE_DOS_HEADER) && DosHeader->e_magic == IMAGE_DOS_SIGNATURE && DosHeader->e_lfanew >= sizeof(IMAGE_DOS_HEADER) && *ViewSize >= DosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS)) {
				PVOID DllBase = *BaseAddress;
				PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader(DllBase);
				if (NtHeader && NtHeader->Signature == IMAGE_NT_SIGNATURE) {
					NTSTATUS Status2;
					NTSTATUS Status3;
					UNICODE_STRING FullDllName = {0};
					UNICODE_STRING BaseDllName;
					RtlInitEmptyUnicodeStringFromTeb(&FullDllName);
					Status2 = GetDllFullName(DllBase, &FullDllName);
					ASSERT(NT_SUCCESS(Status2));
					Status3 = KexRtlPathFindFileName(&FullDllName, &BaseDllName);
					ASSERT(NT_SUCCESS(Status3));
					if (NT_SUCCESS(Status2) && NT_SUCCESS(Status3)) {
						LDR_DLL_NOTIFICATION_DATA NotificationData;
						NotificationData.Flags = 0;
						NotificationData.DllBase = DllBase;
						NotificationData.SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
						NotificationData.FullDllName = &FullDllName;
						NotificationData.BaseDllName = &BaseDllName;
						KexDllNotificationCallback(
							LDR_DLL_NOTIFICATION_REASON_LOADED,
							(PCLDR_DLL_NOTIFICATION_DATA)&NotificationData,
							NULL);
					}
				}
			}
		}
	} except (EXCEPTION_EXECUTE_HANDLER) {}
	return Status;
}

VOID NTAPI KexDllNotificationCallbackForWindows8AndAbove(
	IN	LDR_DLL_NOTIFICATION_REASON	Reason,
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData,
	IN	PVOID						Context OPTIONAL)
{
	if (!DllLoaderInitialized) {
		DllLoaderInitialized = TRUE;
	}
}

//
// This function is called within a try/except block inside NTDLL. So if we
// fuck up here, nothing super bad is going to happen, although of course it
// should still be avoided.
//
// NotificationData->Flags contains values starting with LDR_DLL_LOADED_FLAG_
// and it can (in Windows XP) only be 0 or 1. A value of 1 means that the DLL
// has been relocated. In Windows 7, NotificationData->Flags is always zero 
// and there is no information that can be gathered through this flag.
//
// This function is not called to notify for DLL unloads if the process is
// exiting. This is because, as an optimization, the LdrUnloadDll (XP) or 
// LdrpUnloadDll (Win7) routine does not actually unmap any DLLs when the
// process is exiting.
//
VOID NTAPI KexDllNotificationCallback(
	IN	LDR_DLL_NOTIFICATION_REASON	Reason,
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData,
	IN	PVOID						Context OPTIONAL)
{
	NTSTATUS Status;
	STATIC CONST PCWSTR ReasonToStringLookup[] = {
		L"(unknown)",
		L"mapped",
		L"unmapped",
		L"(unknown)"
	};
	
	//if ((!wcsncmp(NotificationData->BaseDllName->Buffer, L"MacType.dll", MAX_PATH) || !wcsncmp(NotificationData->BaseDllName->Buffer, L"MacType64.dll", MAX_PATH)) && Reason == LDR_DLL_NOTIFICATION_REASON_LOADED) {
	//}

	KexLogDetailEvent(
		L"The DLL %wZ was %s\r\n\r\n"
		L"Full path to the DLL: \"%wZ\"\r\n"
		L"Loaded at 0x%p (size: %I32u bytes)",
		NotificationData->BaseDllName,
		ARRAY_LOOKUP_BOUNDS_CHECKED(ReasonToStringLookup, Reason),
		NotificationData->FullDllName,
		NotificationData->DllBase,
		NotificationData->SizeOfImage);

	if (Reason == LDR_DLL_NOTIFICATION_REASON_LOADED) {
		BOOLEAN ShouldRewriteImports;

		ShouldRewriteImports = KexShouldRewriteStaticImportsOfDll(
			NotificationData->FullDllName,
			NotificationData->BaseDllName);

		unless (KexData->IfeoParameters.DisableAppSpecific) {
			if (ShouldRewriteImports) {
				AshDllLoadNotification(NotificationData);
			}
		}

		if (ShouldRewriteImports) {
			Status = KexRewriteImageImportDirectory(
				NotificationData->DllBase,
				NotificationData->BaseDllName,
				NotificationData->FullDllName);
		}
	}
}