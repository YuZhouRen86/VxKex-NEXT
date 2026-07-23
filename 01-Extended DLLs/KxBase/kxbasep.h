///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kxbasep.h
//
// Abstract:
//
//     Private header file for KxBase.
//
// Author:
//
//     vxiiduu (07-Nov-2022)
//
// Revision History:
//
//     vxiiduu              07-Nov-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>
#include <KexDll.h>
#include <KxBase.h>

#define DLL_DIRECTORY_DATA_VERIFICATION_CODE 'V' + 'x' + 'K' + 'e' + 'x' + 'N' + 'E' + 'X' + 'T'

typedef struct _DLL_DIRECTORY_DATA {
	USHORT	VerificationCode;
	PWSTR	String;
	PVOID	Previous;
	PVOID	Next;
} TYPEDEF_TYPE_NAME(DLL_DIRECTORY_DATA);

EXTERN PKEX_PROCESS_DATA KexData;
EXTERN HANDLE KsecDD;
EXTERN ULONG OriginalMajorVersion, OriginalMinorVersion, OriginalBuildNumber;
EXTERN DLL_DIRECTORY_DATA DllDirectoryData;
EXTERN ULONG DefaultDllDirectoryFlags;
EXTERN BOOL IsVMwareInstallationLauncher;

#if defined(KEX_TARGET_TYPE_EXE) || defined(KEX_TARGET_TYPE_DLL)
#  if defined(KEX_ARCH_X64)
#    pragma comment(lib, "version_x64.lib")
#  elif defined(KEX_ARCH_X86)
#    pragma comment(lib, "version_x86.lib")
#  endif
#endif

//
// module.c
//

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleA(
	IN	PCSTR	ModuleName);

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleW(
	IN	PCWSTR	ModuleName);

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExA(
	IN	ULONG	Flags,
	IN	PCSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut);

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExW(
	IN	ULONG	Flags,
	IN	PCWSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut);

KXBASEAPI ULONG WINAPI Ext_GetModuleFileNameA(
	IN	HMODULE	ModuleHandle,
	OUT	PSTR	FileName,
	IN	ULONG	FileNameCch);

KXBASEAPI ULONG WINAPI Ext_GetModuleFileNameW(
	IN	HMODULE	ModuleHandle,
	OUT	PWSTR	FileName,
	IN	ULONG	FileNameCch);

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryA(
	IN	PCSTR	FileName);

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryW(
	IN	PCWSTR	FileName);

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExA(
	IN	PCSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags);

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExW(
	IN	PCWSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags);

//
// time.c
//

KXBASEAPI VOID WINAPI KxBasepGetSystemTimeAsFileTimeHook(
	OUT	PFILETIME	SystemTimeAsFileTime);

KXBASEAPI VOID WINAPI KxBasepGetSystemTimeHook(
	OUT	PSYSTEMTIME	SystemTime);

//
// support.c
//

PLARGE_INTEGER BaseFormatTimeOut(
	OUT	PLARGE_INTEGER	TimeOut,
	IN	ULONG			Milliseconds);

HANDLE WINAPI BaseGetNamedObjectDirectory(
	VOID);

HANDLE WINAPI BaseGetUntrustedNamedObjectDirectory(
	VOID);

PVOID BaseGetBaseDllHandle(
	VOID);

//
// dllpath.c
//

VOID KxBaseAddKex3264ToBaseDefaultPath(
	VOID);

//
// crypto.c
//

NTSTATUS BaseInitializeCrypto(
	VOID);

//
// conansi.c
//

BOOL IsConsoleHandle(
	IN	HANDLE	Handle);

BOOLEAN IsConsoleOutputHandle(
	IN	HANDLE	Handle);

BOOLEAN BaseIsConsoleAnsiSupportEnabled(
	IN	HANDLE	ConsoleHandle);

BOOLEAN BaseEnableConsoleAnsiSupport(
	IN	HANDLE	ConsoleHandle);

BOOLEAN BaseDisableConsoleAnsiSupport(
	IN	HANDLE	ConsoleHandle);

BOOL WriteConsoleWithEscapeSequencesAorW(
	IN	HANDLE	ConsoleHandle,
	IN	PCVOID	Buffer,
	IN	ULONG	CchToWrite,
	OUT	PULONG	CchWrittenOut OPTIONAL,
	IN	PVOID	Reserved OPTIONAL,
	IN	BOOLEAN	Unicode);

//
// consup.c
//

BOOL WriteConsoleAorW(
	IN	HANDLE	ConsoleHandle,
	IN	PCVOID	Buffer,
	IN	ULONG	CchToWrite,
	OUT	PULONG	CchWritten OPTIONAL,
	IN	PVOID	Reserved OPTIONAL,
	IN	BOOLEAN	Unicode);

BOOLEAN VTGetCursorPosition(
	IN	HANDLE	ConsoleHandle,
	OUT	PCOORD	Position);

BOOLEAN GetConsoleTextAttribute(
	IN	HANDLE	ConsoleHandle,
	OUT	PWORD	Attribute);

BOOLEAN VTMoveCursorRelative(
	IN	HANDLE	ConsoleHandle,
	IN	COORD	RelativeMovement);

BOOLEAN VTMoveCursorVerticalRelativeWithHorizontalReset(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	RelativeMovement);

BOOLEAN VTSetCursorHorizontalPosition(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	HorizontalPosition);

BOOLEAN VTSetCursorVerticalPosition(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	VerticalPosition);

BOOLEAN VTSetCursorPosition(
	IN	HANDLE	ConsoleHandle,
	IN	COORD	AbsolutePosition);

BOOLEAN InjectStringToConsoleInput(
	IN	HANDLE	ConsoleHandle,
	IN	PCWSTR	String);