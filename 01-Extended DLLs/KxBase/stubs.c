///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     stubs.c
//
// Abstract:
//
//     Forwarder stubs that do nothing except for calling the original function.
//     These exist because some stupid software such as Chromium is not
//     compatible with export forwarders.
//
// Author:
//
//     vxiiduu (09-Mar-2024)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu              09-Mar-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"

//
// _ReturnAddress is declared by <intrin.h>, which cannot be included here: this
// tree shadows the compiler's ammintrin.h with an old AMD copy that the modern
// Intrin.h rejects, so pulling it in breaks the build. The intrinsic itself is
// always available, so declare it directly.
//
EXTERN_C PVOID _ReturnAddress(VOID);
#pragma intrinsic(_ReturnAddress)

KXBASEAPI BOOL WINAPI Stub_DuplicateHandle(
	IN	HANDLE		SourceProcessHandle,
	IN	HANDLE		SourceHandle,
	IN	HANDLE		TargetProcessHandle,
	OUT	PHANDLE		TargetHandle,
	IN	ACCESS_MASK	DesiredAccess,
	IN	BOOL		Inherit,
	IN	ULONG		Options)
{
	return DuplicateHandle(
		SourceProcessHandle,
		SourceHandle,
		TargetProcessHandle,
		TargetHandle,
		DesiredAccess,
		Inherit,
		Options);
}

//
// Base address of kxuser.dll if it is already loaded, otherwise NULL.
//
// This walks the loader module list rather than calling GetModuleHandle or
// LdrGetDllHandle, because both of those *load* the DLL when it is absent, and
// loading runs the loader, which resolves imports, which performs more
// GetProcAddress calls. Answering those is exactly what this code is for, so a
// loading API here would close a loop:
//
//   GetProcAddress (fails)
//     -> pointer recovery
//       -> GetModuleHandle("kxuser.dll")   [loads the DLL]
//         -> loader resolves imports
//           -> GetProcAddress (fails)
//             -> ...
//
// That recursed until the stack was exhausted; two earlier attempts to recover
// pointer lookups died with c00000fd inside ntdll!RtlpLocateActivationContextSection,
// in one case reached from KexLdrGetProcedureAddressEx and in the other from
// KexLdrLoadDll.
//
// Enumerating modules cannot load anything, so it cannot re-enter the loader.
// When kxuser is not loaded yet the answer is "unavailable" and the lookup keeps
// its original failure, which is the correct outcome anyway: nothing has asked
// for kxuser, so nothing can be using it.
//
STATIC PVOID GetLoadedKxUserBase(
	VOID)
{
	STATIC PVOID CachedBase = NULL;
	PPEB Peb = NtCurrentPeb();
	PPEB_LDR_DATA Ldr;
	LIST_ENTRY *Head;
	LIST_ENTRY *Entry;
	UNICODE_STRING KxUserName;

	if (CachedBase) return CachedBase;
	if (Peb == NULL || Peb->Ldr == NULL) return NULL;

	Ldr = Peb->Ldr;
	Head = &Ldr->InLoadOrderModuleList;

	RtlInitConstantUnicodeString(&KxUserName, L"kxuser.dll");

	for (Entry = Head->Flink; Entry != Head; Entry = Entry->Flink) {
		PLDR_DATA_TABLE_ENTRY Module;

		Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (Module->BaseDllName.Buffer == NULL) continue;
		if (Module->DllBase == NULL) continue;

		if (RtlEqualUnicodeString(&Module->BaseDllName, &KxUserName, TRUE)) {
			CachedBase = Module->DllBase;
			return CachedBase;
		}
	}

	return NULL;
}

//
// Pointer API names that were added in Windows 8 and are therefore missing from
// the Windows 7 user32.
//
// The list has to be an allow-list rather than "any name that kxuser exports":
// kxuser re-exports a large part of user32, and redirecting a failed lookup for
// an ordinary user32 function to whichever signature kxuser happens to have
// would be far more dangerous than leaving the lookup failed.
//
// RegisterTouchWindow and UnregisterTouchWindow are deliberately absent: Windows
// 7 exports both, so a lookup for either never fails and the recovery code could
// never fire for them.
//
// Chromium resolves GetPointerTouchInfo with GetProcAddress against user32 and
// caches the result in a static, giving up permanently if the first lookup
// returns NULL. That lookup does not travel through an import table, so VxKex's
// import rewriting never sees it, and on Windows 7 it always fails - even though
// kxuser exports a working implementation backed by its touch point cache.
//
// Whether kxuser really exports the name is decided by its export table below,
// so a stale entry in this list can never fabricate a function.
//
STATIC BOOLEAN IsPointerApiName(
	IN	PCSTR	ProcedureName)
{
	STATIC PCSTR Names[] = {
		"GetPointerType",
		"GetPointerInfo",
		"GetPointerTouchInfo",
		"GetPointerTouchInfoHistory",
		"GetPointerFrameTouchInfo",
		"GetPointerFrameTouchInfoHistory",
		"GetPointerPenInfo",
		"GetPointerPenInfoHistory",
		"GetPointerDevices",
		"GetPointerDeviceRects",
		"SkipPointerFrameMessages",
		"EnableMouseInPointer",
		"IsMouseInPointerEnabled",
		"RegisterPointerDeviceNotifications",
		"InitializeTouchInjection",
		"InjectTouchInput"
	};
	ULONG Index;

	if (ProcedureName == NULL) return FALSE;

	for (Index = 0; Index < ARRAYSIZE(Names); ++Index) {
		if (StringEqualA(Names[Index], ProcedureName)) return TRUE;
	}

	return FALSE;
}

//
// Returns the base address of the module whose image contains Address, or NULL
// if no loaded module contains it.
//
STATIC PVOID FindModuleContainingAddress(
	IN	PVOID	Address)
{
	PPEB Peb = NtCurrentPeb();
	PPEB_LDR_DATA Ldr;
	LIST_ENTRY *Head;
	LIST_ENTRY *Entry;

	if (Peb == NULL || Peb->Ldr == NULL || Address == NULL) return NULL;

	Ldr = Peb->Ldr;
	Head = &Ldr->InLoadOrderModuleList;

	for (Entry = Head->Flink; Entry != Head; Entry = Entry->Flink) {
		PLDR_DATA_TABLE_ENTRY Module;
		PBYTE Start;
		PBYTE End;

		Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (Module->DllBase == NULL || Module->SizeOfImage == 0) continue;

		Start = (PBYTE) Module->DllBase;
		End = Start + Module->SizeOfImage;

		if ((PBYTE) Address >= Start && (PBYTE) Address < End) {
			return Module->DllBase;
		}
	}

	return NULL;
}

//
// Answers a failed user32 lookup for a pointer API out of kxuser.
//
// Called only from GetProcAddress, which runs in ordinary user mode and never
// from inside the loader, so the reentrancy hazard described above does not
// apply here.
//
STATIC FARPROC TryResolvePointerApiFromKxUser(
	IN	HMODULE	ModuleHandle,
	IN	PCSTR	ProcedureName)
{
	PVOID KxUserBase;
	PVOID ProcedureAddress = NULL;

	//
	// An ordinal lookup passes MAKEINTRESOURCE(n) here, which is a small
	// integer dressed up as a pointer, not a string. Dereferencing it would
	// fault, so such lookups are left alone: they cannot name a pointer API
	// anyway.
	//
	if (IS_INTRESOURCE(ProcedureName)) return NULL;

	if (!IsPointerApiName(ProcedureName)) return NULL;

	if (KexData == NULL) return NULL;
	if (!(KexData->Flags & KEXDATA_FLAG_CHROMIUM)) return NULL;
	if (KexData->Flags & KEXDATA_FLAG_QT6) return NULL;

	KxUserBase = GetLoadedKxUserBase();
	if (KxUserBase == NULL) return NULL;

	// Never resolve out of the module that is asking.
	if ((PVOID) ModuleHandle == KxUserBase) return NULL;

	//
	// Never answer a probe made by kxuser itself.
	//
	// kxuser's pointer functions are written to look for a real implementation
	// first:
	//
	//   Handle = LoadSystemLibrary(L"user32.dll");
	//   pGetPointerType = GetProcAddress(Handle, "GetPointerType");
	//   if (pGetPointerType) return pGetPointerType(...);   // real system one
	//   ...otherwise fall back to the touch point cache
	//
	// That GetProcAddress is compiled as an import from the rewritten kernel32,
	// so it lands here, and on Windows 7 the lookup against the real user32
	// always fails. Handing back kxuser's own GetPointerType would make the
	// caller invoke itself and recurse until the stack is gone:
	//
	//   kxuser!GetPointerType -> probe -> stub -> kxuser!GetPointerType -> ...
	//
	// Checking the return address against kxuser's image range keeps that probe
	// meaningful: it still sees "no real implementation", which is exactly what
	// it is trying to find out.
	//
	if (FindModuleContainingAddress(_ReturnAddress()) == KxUserBase) return NULL;

	if (!NT_SUCCESS(KexLdrMiniGetProcedureAddress(
			KxUserBase,
			ProcedureName,
			(PVOID64 *) &ProcedureAddress))) {

		return NULL;
	}

	return (FARPROC) ProcedureAddress;
}

KXBASEAPI FARPROC WINAPI Stub_GetProcAddress(
	IN	HMODULE		ModuleHandle,
	IN	PCSTR		ProcedureName)
{
	FARPROC Result;

	Result = GetProcAddress(
		ModuleHandle,
		ProcedureName);

	//
	// Recover pointer API lookups that the system DLL cannot satisfy.
	//
	// Only a lookup that genuinely failed is redirected, so a name the system
	// DLL does export keeps its original answer and behaves exactly as before.
	//
	if (Result == NULL && ProcedureName != NULL) {
		Result = TryResolvePointerApiFromKxUser(ModuleHandle, ProcedureName);
	}

	return Result;
}