///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     module.c
//
// Abstract:
//
//     Thjs file contains functions related to DLL loading.
//     The main purpose of the files in here is to rewrite the names of DLLs
//     which the application requests to dynamically load.
//
// Author:
//
//     Author (10-Feb-2024)
//
// Environment:
//
//     Win32 mode.
//
// Revision History:
//
//     vxiiduu              10-Feb-2024    Initial creation.
//     vxiiduu              02-Mar-2024    Fix GetModuleHandleExW logging when
//										   GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
//										   flag is passed.
//     vxiiduu              13-Mar-2024    Move most of the code here to kexldr.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"
#include <KexW32ML.h>
#include <Shlwapi.h>

//
// These two utility functions make use of an unused field in the TEB.
// Their purpose is to set KexLdrShouldRewriteDll to 1 whenever
// Ext_GetModuleHandle(Ex)(A/W) or Ext_LoadLibrary(Ex)(A/W) is present in
// the call stack.
//
// When this happens, it means that an EXE or DLL outside of WinDir and KexDir
// has called GetModuleHandle or LoadLibrary. It signals to Ext_LdrLoadDll
// and Ext_LdrGetDllHandle so that they can avoid rewriting imports when it
// isn't desired.
//

STATIC INLINE VOID InterceptedKernelBaseLoaderCallEntry(
	OUT	PBOOLEAN	ReEntrant)
{
	PTEB Teb;

	Teb = NtCurrentTeb();
	*ReEntrant = Teb->KexLdrShouldRewriteDll;
	Teb->KexLdrShouldRewriteDll = TRUE;
}

STATIC INLINE VOID InterceptedKernelBaseLoaderCallReturn(
	IN	BOOLEAN		ReEntrant)
{
	if (!ReEntrant) {
		NtCurrentTeb()->KexLdrShouldRewriteDll = FALSE;
	}
}

STATIC NTSTATUS BasepGetDllDirectoryProcedure(
	IN		PCSTR	ProcedureName,
	IN OUT	PPVOID	ProcedureAddress)
{
	NTSTATUS Status;

	Status = STATUS_SUCCESS;

	ASSERT (ProcedureName != NULL);
	ASSERT (ProcedureAddress != NULL);

	if (!*ProcedureAddress) {
		ANSI_STRING ProcedureNameAS;

		Status = RtlInitAnsiStringEx(&ProcedureNameAS, ProcedureName);
		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		ASSUME (KexData->BaseDllBase != NULL);

		Status = LdrGetProcedureAddress(
			KexData->BaseDllBase,
			&ProcedureNameAS,
			0,
			ProcedureAddress);

		if (!NT_SUCCESS(Status)) {
			/*KexLogErrorEvent(
				L"%hs is not available on this computer\r\n\r\n"
				L"This function is only available on Windows 7 with the KB2533623 "
				L"security update.", ProcedureName);*/

			BaseSetLastNTError(Status);
		}
	}

	if (NT_SUCCESS(Status)) {
		ASSUME (*ProcedureAddress != NULL);
	}

	return Status;
}

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleA(
	IN	PCSTR	ModuleName)
{
	HMODULE ModuleHandle;
	BOOLEAN ReEntrant;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	ModuleHandle = GetModuleHandleA(ModuleName);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return ModuleHandle;
}

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleW(
	IN	PCWSTR	ModuleName)
{
	HMODULE ModuleHandle;
	BOOLEAN ReEntrant;

	//
	// APPSPECIFICHACK: Chromium-based software uses a bootleg knockoff version of
	// GetProcAddress that fails miserably and crashes the whole app when we rewrite
	// NTDLL, because their shitty implementation doesn't work properly with
	// the export forwarders in KxNt. Neither does it properly work with stubs,
	// because they actually scan the instruction code of system calls.
	//
	if (((KexData->Flags & KEXDATA_FLAG_CHROMIUM) || (KexData->Flags & KEXDATA_FLAG_FIREFOX)) &&
		ModuleName != NULL &&
		StringEqual(ModuleName, L"ntdll.dll")) {

		KexLogDebugEvent(L"Not rewriting NTDLL for Chromium or Firefox compatibility");
		return (HMODULE) KexData->SystemDllBase;
	}

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	ModuleHandle = GetModuleHandleW(ModuleName);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return ModuleHandle;
}

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExA(
	IN	ULONG	Flags,
	IN	PCSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut)
{
	BOOL Success;
	BOOLEAN ReEntrant;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	Success = GetModuleHandleExA(Flags, ModuleName, ModuleHandleOut);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return Success;
}

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExW(
	IN	ULONG	Flags,
	IN	PCWSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut)
{
	BOOL Success;
	BOOLEAN ReEntrant;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	Success = GetModuleHandleExW(Flags, ModuleName, ModuleHandleOut);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return Success;
}

STATIC PWSTR CopyPcstrToPwstr(
	PCSTR Source)
{
	PWSTR Dest;
	INT BufferLength = MultiByteToWideChar(CP_THREAD_ACP, 0, Source, -1, NULL, 0);
	if (BufferLength == 0) return NULL;
	Dest = SafeAlloc(WCHAR, BufferLength);
	if (!Dest) return NULL;
	if (MultiByteToWideChar(CP_THREAD_ACP, 0, Source, -1, Dest, BufferLength) == 0) {
		SafeFree(Dest);
		return NULL;
	}
	return Dest;
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryA(
	IN	PCSTR	FileName)
{
	return Ext_LoadLibraryExA(FileName, NULL, 0);
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryW(
	IN	PCWSTR	FileName)
{
	return Ext_LoadLibraryExW(FileName, NULL, 0);
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExA(
	IN	PCSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags)
{
	HMODULE ModuleHandle;
	BOOLEAN ReEntrant;
	STATIC BOOL (WINAPI *SetDefaultDllDirectories) (ULONG) = NULL;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	BasepGetDllDirectoryProcedure("SetDefaultDllDirectories", (PPVOID) &SetDefaultDllDirectories);
	if (SetDefaultDllDirectories) {
		ModuleHandle = LoadLibraryExA(FileName, FileHandle, Flags);
	} else {
		ULONG OriginalFlags = Flags;
		DWORD LoadLibrarySearchMarks = 0xFFFFFF00ul;

		ModuleHandle = NULL;
		if (((0xFFFFE000 | 0x00000004) & Flags) || ((LOAD_WITH_ALTERED_SEARCH_PATH & Flags) && (LoadLibrarySearchMarks & Flags)) || FileName == NULL || FileHandle) {
			SetLastError(ERROR_INVALID_PARAMETER);
		} else {
			Flags &= ~LoadLibrarySearchMarks;
			ModuleHandle = LoadLibraryExA(FileName, FileHandle, Flags);
			if (!ModuleHandle &&
				(OriginalFlags & LOAD_LIBRARY_SEARCH_USER_DIRS || (!(OriginalFlags & LoadLibrarySearchMarks) && (DefaultDllDirectoryFlags & LOAD_LIBRARY_SEARCH_USER_DIRS))) &&
				FileName &&
				FileName[0] != '\0' &&
				FileName[0] != '\\' &&
				FileName[0] != '/' &&
				GetLastError() == ERROR_MOD_NOT_FOUND) {

				PWSTR FileName_U = CopyPcstrToPwstr(FileName);
				if (FileName_U) {
					PDLL_DIRECTORY_DATA Directory;
					for (Directory = (PDLL_DIRECTORY_DATA)DllDirectoryData.Next; Directory && Directory->String != NULL; Directory = (PDLL_DIRECTORY_DATA)Directory = (PDLL_DIRECTORY_DATA)Directory->Next) {
						SIZE_T NewFileNameLength = wcslen(FileName_U) + wcslen(Directory->String) + 2;
						PWSTR NewFileName = SafeAlloc(WCHAR, NewFileNameLength);
						StringCchCopy(NewFileName, NewFileNameLength, Directory->String);
						StringCchCat(NewFileName, NewFileNameLength, L"\\");
						StringCchCat(NewFileName, NewFileNameLength, FileName_U);
						ModuleHandle = LoadLibraryExW(NewFileName, FileHandle, Flags);
						if (ModuleHandle || GetLastError() != ERROR_MOD_NOT_FOUND) break;
					}
					SafeFree(FileName_U);
				}

				if (!ModuleHandle) SetLastError(ERROR_MOD_NOT_FOUND);
			}
		}
	}
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return ModuleHandle;
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExW(
	IN	PCWSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags)
{
	HMODULE ModuleHandle;
	BOOLEAN ReEntrant;
	STATIC BOOL (WINAPI *SetDefaultDllDirectories) (ULONG) = NULL;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	BasepGetDllDirectoryProcedure("SetDefaultDllDirectories", (PPVOID) &SetDefaultDllDirectories);
	if (SetDefaultDllDirectories) {
		ModuleHandle = LoadLibraryExW(FileName, FileHandle, Flags);
	} else {
		ULONG OriginalFlags = Flags;
		DWORD LoadLibrarySearchMarks = 0xFFFFFF00ul;

		ModuleHandle = NULL;
		if (((0xFFFFE000 | 0x00000004) & Flags) || ((LOAD_WITH_ALTERED_SEARCH_PATH & Flags) && (LoadLibrarySearchMarks & Flags)) || FileName == NULL || FileHandle) {
			SetLastError(ERROR_INVALID_PARAMETER);
		} else {
			Flags &= ~LoadLibrarySearchMarks;
			ModuleHandle = LoadLibraryExW(FileName, FileHandle, Flags);
			if (!ModuleHandle &&
				(OriginalFlags & LOAD_LIBRARY_SEARCH_USER_DIRS || (!(OriginalFlags & LoadLibrarySearchMarks) && (DefaultDllDirectoryFlags & LOAD_LIBRARY_SEARCH_USER_DIRS))) &&
				FileName &&
				FileName[0] != L'\0' &&
				FileName[0] != L'\\' &&
				FileName[0] != L'/' &&
				GetLastError() == ERROR_MOD_NOT_FOUND) {

				PDLL_DIRECTORY_DATA Directory;
				for (Directory = (PDLL_DIRECTORY_DATA)DllDirectoryData.Next; Directory && Directory->String != NULL; Directory = (PDLL_DIRECTORY_DATA)Directory->Next) {
					SIZE_T NewFileNameLength = wcslen(FileName) + wcslen(Directory->String) + 2;
					PWSTR NewFileName = SafeAlloc(WCHAR, NewFileNameLength);
					StringCchCopy(NewFileName, NewFileNameLength, Directory->String);
					StringCchCat(NewFileName, NewFileNameLength, L"\\");
					StringCchCat(NewFileName, NewFileNameLength, FileName);
					ModuleHandle = LoadLibraryExW(NewFileName, FileHandle, Flags);
					if (ModuleHandle || GetLastError() != ERROR_MOD_NOT_FOUND) break;
				}

				if (!ModuleHandle) SetLastError(ERROR_MOD_NOT_FOUND);
			}
		}
	}
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return ModuleHandle;
}

KXBASEAPI HMODULE WINAPI LoadPackagedLibrary(
	IN	PCWSTR	LibFileName,
	IN	ULONG	Reserved)
{
	RTL_PATH_TYPE PathType;
	ULONG Index;

	if (Reserved) {
		BaseSetLastNTError(STATUS_INVALID_PARAMETER);
		return NULL;
	}

	PathType = RtlDetermineDosPathNameType_U(LibFileName);

	if (PathType != RtlPathTypeRelative) {
		BaseSetLastNTError(STATUS_INVALID_PARAMETER);
		return NULL;
	}

	for (Index = 0; LibFileName[Index] != '\0'; ++Index) {
		if (LibFileName[Index] == '.' && LibFileName[Index+1] == '.' &&
			(LibFileName[Index+2] == '\\' || LibFileName[Index+2] == '/')) {

			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return NULL;
		}

		do {
			++Index;
		} until (LibFileName[Index] == '\0' ||
				 LibFileName[Index] == '\\' ||
				 LibFileName[Index] == '/');
	}

	// On Windows 8 this would be the point where this function would call
	// LoadLibraryExW with the undocumented flag 0x04. However, this flag and
	// its underlying implementation inside LdrLoadDll is not present on Windows
	// 7, so we will just return an error straight away (as documented).
	RtlSetLastWin32Error(APPMODEL_ERROR_NO_PACKAGE);
	return NULL;
}

KXBASEAPI DLL_DIRECTORY_COOKIE WINAPI Ext_AddDllDirectory(
	IN	PCWSTR	NewDirectory)
{
	STATIC DLL_DIRECTORY_COOKIE (WINAPI *AddDllDirectory) (PCWSTR) = NULL;

	BasepGetDllDirectoryProcedure("AddDllDirectory", (PPVOID) &AddDllDirectory);

	if (AddDllDirectory) {
		return AddDllDirectory(NewDirectory);
	} else {
		PDLL_DIRECTORY_DATA NewDllDirectoryData, LastDllDirectoryData;
		DWORD Attributes;
		RTL_PATH_TYPE PathType;

		if (NewDirectory == NULL) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		}

		PathType = RtlDetermineDosPathNameType_U(NewDirectory);
		if (PathType == RtlPathTypeUnknown || PathType == RtlPathTypeDriveRelative || PathType == RtlPathTypeRooted || PathType == RtlPathTypeRelative) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		}

		Attributes = GetFileAttributesW(NewDirectory);
		if (Attributes == INVALID_FILE_ATTRIBUTES || !(Attributes & FILE_ATTRIBUTE_DIRECTORY)) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		}

		NewDllDirectoryData = SafeAlloc(DLL_DIRECTORY_DATA, 1);
		if (!NewDllDirectoryData) {
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return 0;
		}
		NewDllDirectoryData->VerificationCode = DLL_DIRECTORY_DATA_VERIFICATION_CODE;
		NewDllDirectoryData->String = SafeAlloc(WCHAR, wcslen(NewDirectory) + 1);
		if (!NewDllDirectoryData->String) {
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return 0;
		}
		StringCchCopy(NewDllDirectoryData->String, wcslen(NewDirectory) + 1, NewDirectory);
		if (!DllDirectoryData.Next || !DllDirectoryData.Previous) {
			DllDirectoryData.Previous = &DllDirectoryData;
			DllDirectoryData.Next = &DllDirectoryData;
		}
		LastDllDirectoryData = (PDLL_DIRECTORY_DATA)DllDirectoryData.Previous;
		LastDllDirectoryData->Next = NewDllDirectoryData;
		NewDllDirectoryData->Previous = LastDllDirectoryData;
		NewDllDirectoryData->Next = &DllDirectoryData;
		DllDirectoryData.Previous = NewDllDirectoryData;
		return (DLL_DIRECTORY_COOKIE) NewDllDirectoryData;
	}
}

KXBASEAPI BOOL WINAPI Ext_RemoveDllDirectory(
	IN	DLL_DIRECTORY_COOKIE	Cookie)
{
	STATIC BOOL (WINAPI *RemoveDllDirectory) (DLL_DIRECTORY_COOKIE) = NULL;

	BasepGetDllDirectoryProcedure("RemoveDllDirectory", (PPVOID) &RemoveDllDirectory);

	if (RemoveDllDirectory) {
		return RemoveDllDirectory(Cookie);
	} else {
		PDLL_DIRECTORY_DATA GarbageDllDirectoryData, PreviousDllDirectoryData, NextDllDirectoryData;
		GarbageDllDirectoryData = (PDLL_DIRECTORY_DATA)Cookie;
		if (
			!Cookie ||
			GarbageDllDirectoryData->VerificationCode != DLL_DIRECTORY_DATA_VERIFICATION_CODE ||
			!GarbageDllDirectoryData->String ||
			!GarbageDllDirectoryData->Next ||
			!GarbageDllDirectoryData->Previous) {

			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}
		PreviousDllDirectoryData = (PDLL_DIRECTORY_DATA)GarbageDllDirectoryData->Previous;
		NextDllDirectoryData = (PDLL_DIRECTORY_DATA)GarbageDllDirectoryData->Next;
		PreviousDllDirectoryData->Next = GarbageDllDirectoryData->Next;
		NextDllDirectoryData->Previous = GarbageDllDirectoryData->Previous;
		GarbageDllDirectoryData->Next = NULL;
		GarbageDllDirectoryData->Previous = NULL;
		SafeFree(GarbageDllDirectoryData->String);
		GarbageDllDirectoryData->String = NULL;
		SafeFree(GarbageDllDirectoryData);
		if (&DllDirectoryData == DllDirectoryData.Next || &DllDirectoryData == DllDirectoryData.Previous) {
			DllDirectoryData.Next = NULL;
			DllDirectoryData.Previous = NULL;
		}
		return TRUE;
	}
}

KXBASEAPI BOOL WINAPI Ext_SetDefaultDllDirectories(
	IN	ULONG	DirectoryFlags)
{
	STATIC BOOL (WINAPI *SetDefaultDllDirectories) (ULONG) = NULL;

	BasepGetDllDirectoryProcedure("SetDefaultDllDirectories", (PPVOID) &SetDefaultDllDirectories);

	DirectoryFlags |= LOAD_LIBRARY_SEARCH_USER_DIRS;
	if (SetDefaultDllDirectories) {
		return SetDefaultDllDirectories(DirectoryFlags);
	} else {
		if (DirectoryFlags & ~(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}
		DefaultDllDirectoryFlags = DirectoryFlags;
		return TRUE;
	}
}