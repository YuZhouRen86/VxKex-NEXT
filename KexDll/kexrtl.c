///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexrtl.c
//
// Abstract:
//
//     Various useful run-time routines.
//
// Author:
//
//     vxiiduu (17-Oct-2022)
//
// Revision History:
//
//     vxiiduu              17-Oct-2022  Initial creation.
//     vxiiduu              29-Oct-2022  Fix bug in KexRtlPathFindFileName
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

KEXAPI INT NTAPI KexRtlOperatingSystemBitness(
	VOID)
{
#ifdef KEX_ARCH_X64
	return 64;
#else
	STATIC ULONG Bitness = 0;
	if (!Bitness) {
		ULONG_PTR Wow64Information = 0;
		NtQueryInformationProcess(
			NtCurrentProcess(),
			ProcessWow64Information,
			&Wow64Information,
			sizeof(Wow64Information),
			NULL);
		Bitness = Wow64Information ? 64 : 32;
	}
	return Bitness;
#endif
}

KEXAPI VOID NTAPI KexRtlGetNtVersionNumbers(
	OUT	PULONG	MajorVersion OPTIONAL,
	OUT	PULONG	MinorVersion OPTIONAL,
	OUT	PULONG	BuildNumber OPTIONAL)
{
    if (MajorVersion) *MajorVersion = OriginalMajorVersion; 
    if (MinorVersion) *MinorVersion = OriginalMinorVersion; 
    if (BuildNumber) *BuildNumber = OriginalBuildNumber;
}

// Examples:
// C:\Windows\system32\notepad.exe -> notepad.exe
// notepad.exe -> notepad.exe
// dir1\dir2\notepad.exe -> dir1\dir2\notepad.exe
//
// (As you can see, this function only works on FULL paths - otherwise,
// the output path is unchanged.)
//
// It's ok to specify the same pointer for Path and FileName, if you want to remove
// the file path in-place.
KEXAPI NTSTATUS NTAPI KexRtlPathFindFileName(
	IN	PCUNICODE_STRING Path,
	OUT	PUNICODE_STRING FileName)
{
	ULONG LengthWithoutLastElement;

	if (!Path) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!FileName) {
		return STATUS_INVALID_PARAMETER_2;
	}

	//
	// If Path->Buffer contains a path with no backslashes, this function
	// will fail and set LengthWithoutLastElement to zero. This is desired
	// and that's why the return value is not checked.
	//
	RtlGetLengthWithoutLastFullDosOrNtPathElement(0, Path, &LengthWithoutLastElement);

	FileName->Buffer = Path->Buffer + LengthWithoutLastElement;
	FileName->Length = Path->Length - (USHORT) (LengthWithoutLastElement * sizeof(WCHAR));
	FileName->MaximumLength = Path->MaximumLength - (USHORT) (LengthWithoutLastElement * sizeof(WCHAR));

	return STATUS_SUCCESS;
}

// Examples:
// C:\Windows\system32\notepad.exe -> C:\Windows\system32\notepad
// C:\Users\bob.smith\Videos -> C:\Users\bob.smith\Videos
// C:\Users\bob.smith\ -> C:\Users\bob.smith\
// C:\Users\bob.smith -> C:\Users\bob
// file.txt -> file
// file -> file
// file. -> file
// .file -> <empty string>
//
// Returns STATUS_SUCCESS if the extension was successfully removed,
// or STATUS_NOT_FOUND if no extension was removed.

KEXAPI NTSTATUS NTAPI KexRtlPathRemoveExtension(
	IN	PCUNICODE_STRING	Path,
	OUT	PUNICODE_STRING		PathWithoutExtension)
{
	NTSTATUS Status;
	UNICODE_STRING Stops;
	USHORT PrefixLength;
	
	if (!Path || !PathWithoutExtension || Path->Length == 0) {
		return STATUS_INVALID_PARAMETER;
	}

	*PathWithoutExtension = *Path;

	RtlInitConstantUnicodeString(&Stops, L".\\/");
	Status = RtlFindCharInUnicodeString(
		RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
		PathWithoutExtension,
		&Stops,
		&PrefixLength);

	if (NT_SUCCESS(Status)) {
		if (PathWithoutExtension->Buffer[PrefixLength / sizeof(WCHAR)] == '.') {
			PathWithoutExtension->Length = PrefixLength;
		}
	}

	return Status;
}

KEXAPI BOOLEAN NTAPI KexRtlPathReplaceIllegalCharacters(
	IN OUT	PUNICODE_STRING		Path,
	IN		WCHAR				ReplacementCharacter OPTIONAL,
	IN		BOOLEAN				AllowPathSeparators)
{
	PWSTR PathBuffer;
	PCWSTR PathEnd;
	BOOLEAN AtLeastOneCharacterWasReplaced;

	ASSERT (Path != NULL);
	ASSERT (Path->Length != 0);
	ASSERT (Path->Buffer != NULL);

	if (!Path || !Path->Length || !Path->Buffer) {
		return FALSE;
	}

	if (!ReplacementCharacter) {
		ReplacementCharacter = '_';
	}

	AtLeastOneCharacterWasReplaced = FALSE;
	PathBuffer = Path->Buffer;
	PathEnd = KexRtlEndOfUnicodeString(Path);

	until (PathBuffer == PathEnd) {
		switch (*PathBuffer) {
		case '<':
		case '>':
		case ':':
		case '"':
		case '|':
		case '?':
		case '*':
			*PathBuffer = ReplacementCharacter;
			AtLeastOneCharacterWasReplaced = TRUE;
			break;
		case '/':
		case '\\':
			unless (AllowPathSeparators) {
				*PathBuffer = ReplacementCharacter;
				AtLeastOneCharacterWasReplaced = TRUE;
			}
			break;
		}

		PathBuffer++;
	}

	return AtLeastOneCharacterWasReplaced;
}

KEXAPI NTSTATUS NTAPI KexRtlGetProcessImageBaseName(
	OUT	PUNICODE_STRING	FileName)
{
	return KexRtlPathFindFileName(&NtCurrentPeb()->ProcessParameters->ImagePathName, FileName);
}

//
// NtQueryKeyValue is too annoying to use in everyday code, RtlQueryRegistryValues
// is unsafe, and RtlpNtQueryKeyValue only supports the default/unnamed key. So
// here is an API that essentially mimics the function of win32 RegGetValue.
//
//   KeyHandle - Handle to an open registry key.
//
//   ValueName - Name of the value to query.
//
//   ValueDataCb - Points to size, in bytes, of the buffer indicated by ValueData.
//                 Upon successful return, contains the size of the data retrieved
//                 from the registry.
//                 If this value is 0 before the function is called, the function
//                 will return with STATUS_INSUFFICIENT_BUFFER, not check the
//                 ValueData parameter, and place the correct buffer size required
//                 to store the requested registry data in *ValueDataCb.
//
//   ValueData - Buffer which holds the returned data. If NULL, the function will
//               fail with STATUS_INVALID_PARAMETER (unless ValueDataCb is zero).
//
//   ValueDataTypeRestrict - Indicates which data types are allowed to be returned.
//                           One or more flags from the REG_RESTRICT_* set can be
//                           passed. If the data type of the value in the registry
//                           does not match these filters, the function will return
//                           STATUS_OBJECT_TYPE_MISMATCH and *ValueDataType will
//                           contain the type of the registry data.
//
//   ValueDataType - If function returns successfully, contains the data type of the
//                   data read from the registry.
//
// If this function returns with a failure code, the buffer pointed to by ValueData
// is unmodified.
//
KEXAPI NTSTATUS NTAPI KexRtlQueryKeyValueData(
	IN		HANDLE				KeyHandle,
	IN		PCUNICODE_STRING	ValueName,
	IN OUT	PULONG				ValueDataCb,
	OUT		PVOID				ValueData OPTIONAL,
	IN		ULONG				ValueDataTypeRestrict,
	OUT		PULONG				ValueDataType OPTIONAL)
{
	NTSTATUS Status;
	PVOID KeyValueBuffer;
	ULONG KeyValueBufferCb;
	PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;

	//
	// Validate parameters.
	//
	if (!KeyHandle || KeyHandle == INVALID_HANDLE_VALUE) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!ValueName) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!ValueDataCb) {
		return STATUS_INVALID_PARAMETER_3;
	}

	if (ValueData != NULL && *ValueDataCb == 0) {
		return STATUS_INVALID_PARAMETER_MIX;
	}

	if (!ValueData && *ValueDataCb != 0) {
		return STATUS_INVALID_PARAMETER_MIX;
	}

	if (!ValueDataTypeRestrict || (ValueDataTypeRestrict & (~LEGAL_REG_RESTRICT_MASK))) {
		return STATUS_INVALID_PARAMETER_5;
	}

	//
	// First of all, check if the caller just wants to know the length
	// of buffer required.
	//

	if (*ValueDataCb == 0) {
		Status = NtQueryValueKey(
			KeyHandle,
			ValueName,
			KeyValuePartialInformation,
			NULL,
			0,
			ValueDataCb);

		if (Status == STATUS_BUFFER_TOO_SMALL) {
			*ValueDataCb -= sizeof(KEY_VALUE_PARTIAL_INFORMATION);
		}

		return Status;
	}

	//
	// Now we allocate a buffer to store the KEY_VALUE_PARTIAL_INFORMATION
	// structure in addition to any data read from the registry.
	//
	
	KeyValueBufferCb = *ValueDataCb + sizeof(KEY_VALUE_PARTIAL_INFORMATION);
	KeyValueBuffer = SafeAlloc(BYTE, KeyValueBufferCb);

	if (!KeyValueBuffer) {
		return STATUS_NO_MEMORY;
	}

	Status = NtQueryValueKey(
		KeyHandle,
		ValueName,
		KeyValuePartialInformation,
		KeyValueBuffer,
		KeyValueBufferCb,
		ValueDataCb);

	if (!NT_SUCCESS(Status)) {
		goto Exit;
	}

	*ValueDataCb -= sizeof(KEY_VALUE_PARTIAL_INFORMATION);
	KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) KeyValueBuffer;

	//
	// Now, we check the data type of the returned value to make sure it
	// is matched by the ValueDataTypeRestrict filter.
	//

	unless (ValueDataTypeRestrict & (1 << KeyValueInformation->Type)) {
		Status = STATUS_OBJECT_TYPE_MISMATCH;
		goto Exit;
	}

	//
	// Copy the result into the caller's buffer.
	//

	RtlCopyMemory(ValueData, KeyValueInformation->Data, KeyValueInformation->DataLength);

Exit:
	if (NT_SUCCESS(Status) || Status == STATUS_OBJECT_TYPE_MISMATCH) {
		if (ValueDataType) {
			*ValueDataType = KeyValueInformation->Type;
		}
	}

	SafeFree(KeyValueBuffer);
	return Status;
}

//
// Query multiple values of a key.
//
// KeyHandle
//   Handle to an open registry key under which to query values.
//
// QueryTable
//   Pointer to an array of KEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY
//   structures which provide space to store input and output parameters to
//   the KexRtlQueryKeyValueData routine.
//
// NumberOfQueryTableElements
//   Pointer to number of elements in the array pointed to by QueryTable.
//   Upon return, the number pointed to by this parameter contains the number
//   of values successfully queried.
//
// Flags
//   Valid "Flags" parameters start with QUERY_KEY_MULTIPLE_VALUE_:
//
//   QUERY_KEY_MULTIPLE_VALUE_FAIL_FAST (1)
//     Fail and return a failure code if one of the values in the query
//     table cannot be queried. By default, on failure to query a value
//     this function will simply record failure status inside the query
//     table entry, continue to the next entry and return success once
//     all values have been queried.
//
KEXAPI NTSTATUS NTAPI KexRtlQueryKeyMultipleValueData(
	IN		HANDLE												KeyHandle,
	IN		PKEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY	QueryTable,
	IN OUT	PULONG												NumberOfQueryTableElements,
	IN		ULONG												Flags)
{
	ULONG Counter;

	if (!QueryTable) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!NumberOfQueryTableElements || *NumberOfQueryTableElements == 0) {
		return STATUS_INVALID_PARAMETER_3;
	}

	Counter = *NumberOfQueryTableElements;
	*NumberOfQueryTableElements = 0;

	if (Flags & ~(QUERY_KEY_MULTIPLE_VALUE_FAIL_FAST)) {
		return STATUS_INVALID_PARAMETER_4;
	}

	do {
		QueryTable->Status = KexRtlQueryKeyValueData(
			KeyHandle,
			&QueryTable->ValueName,
			&QueryTable->ValueDataCb,
			QueryTable->ValueData,
			QueryTable->ValueDataTypeRestrict,
			&QueryTable->ValueDataType);

		if (Flags & QUERY_KEY_MULTIPLE_VALUE_FAIL_FAST) {
			if (!NT_SUCCESS(QueryTable->Status)) {
				return STATUS_UNSUCCESSFUL;
			}
		}

		++QueryTable;
		++*NumberOfQueryTableElements;
	} while (--Counter);

	return STATUS_SUCCESS;
}

//
// Check whether a string ends with another string.
// For example, you can use this to see if a filename has a particular
// extension.
//
KEXAPI BOOLEAN NTAPI KexRtlUnicodeStringEndsWith(
	IN	PCUNICODE_STRING	String,
	IN	PCUNICODE_STRING	EndsWith,
	IN	BOOLEAN				CaseInsensitive)
{
	UNICODE_STRING EndOfString;

	//
	// Create a subset of the String that just contains the
	// end of it (with the number of characters that EndsWith
	// contains).
	//

	EndOfString.Buffer = String->Buffer + KexRtlUnicodeStringCch(String) - KexRtlUnicodeStringCch(EndsWith);
	EndOfString.Length = EndsWith->Length;
	EndOfString.MaximumLength = EndsWith->Length;

	if (EndOfString.Buffer < String->Buffer) {
		// EndsWith length greater than String length
		return FALSE;
	}

	//
	// Now perform the actual check.
	//

	return RtlEqualUnicodeString(&EndOfString, EndsWith, CaseInsensitive);
}

//
// Similar to RtlFindUnicodeSubstring in Win10 NTDLL (but does not
// respect NLS).
// Returns the address of the character in Haystack where Needle starts,
// or NULL if Needle could not be found.
//
KEXAPI PWCHAR NTAPI KexRtlFindUnicodeSubstring(
	PCUNICODE_STRING	Haystack,
	PCUNICODE_STRING	Needle,
	BOOLEAN				CaseInsensitive)
{
	ULONG LengthOfNeedle;
	ULONG LengthOfHaystack;
	PWCHAR NeedleBuffer;
	PWCHAR NeedleBufferEnd;
	PWCHAR HaystackBuffer;
	PWCHAR HaystackBufferEnd;
	PWCHAR HaystackBufferRealEnd;
	PWCHAR StartOfNeedleInHaystack;
	WCHAR NeedleFirst;

	LengthOfNeedle = Needle->Length & ~1;
	LengthOfHaystack = Haystack->Length & ~1;

	if (LengthOfNeedle > LengthOfHaystack || !LengthOfHaystack || !LengthOfNeedle) {
		return NULL;
	}

	NeedleBuffer = Needle->Buffer;
	NeedleBufferEnd = (PWCHAR) (((PBYTE) NeedleBuffer) + LengthOfNeedle);
	HaystackBuffer = Haystack->Buffer;
	HaystackBufferEnd = (PWCHAR) (((PBYTE) HaystackBuffer) + LengthOfHaystack - LengthOfNeedle);
	HaystackBufferRealEnd = (PWCHAR) (((PBYTE) HaystackBufferEnd) + LengthOfNeedle);

	if (CaseInsensitive) {
		NeedleFirst = ToUpper(*NeedleBuffer);

		while (TRUE) {
			NeedleBuffer = Needle->Buffer + 1;

			while (ToUpper(*HaystackBuffer) != NeedleFirst) {
				++HaystackBuffer; // Multiple evaluation. Can't increment inside macro

				if (HaystackBuffer > HaystackBufferEnd) {
					return NULL;
				}
			}

			StartOfNeedleInHaystack = HaystackBuffer++;

			while (ToUpper(*HaystackBuffer) == ToUpper(*NeedleBuffer)) {
				++HaystackBuffer;
				++NeedleBuffer;

				if (HaystackBuffer > HaystackBufferRealEnd) {
					break;
				} else if (NeedleBuffer >= NeedleBufferEnd) {
					return StartOfNeedleInHaystack;
				}
			}
		}
	} else {
		NeedleFirst = *NeedleBuffer;

		while (TRUE) {
			NeedleBuffer = Needle->Buffer + 1;

			while (*HaystackBuffer++ != NeedleFirst) {
				if (HaystackBuffer > HaystackBufferEnd) {
					return NULL;
				}
			}

			StartOfNeedleInHaystack = HaystackBuffer - 1;

			while (*HaystackBuffer++ == *NeedleBuffer++) {
				if (HaystackBuffer > HaystackBufferRealEnd) {
					break;
				} else if (NeedleBuffer >= NeedleBufferEnd) {
					return StartOfNeedleInHaystack;
				}
			}
		}
	}
}

KEXAPI VOID NTAPI KexRtlAdvanceUnicodeString(
	OUT	PUNICODE_STRING	String,
	IN	USHORT			AdvanceCb)
{
	String->Buffer += (AdvanceCb / sizeof(WCHAR));
	String->Length -= AdvanceCb;
	String->MaximumLength -= AdvanceCb;
}

KEXAPI VOID NTAPI KexRtlRetreatUnicodeString(
	OUT	PUNICODE_STRING	String,
	IN	USHORT			RetreatCb)
{
	String->Buffer -= (RetreatCb / sizeof(WCHAR));
	String->Length += RetreatCb;
	String->MaximumLength += RetreatCb;
}

// note: once this function returns, NewEnd will point to the first character
// which is outside the bounds of the String's buffer.
KEXAPI NTSTATUS NTAPI KexRtlSetUnicodeStringBufferEnd(
	OUT	PUNICODE_STRING	String,
	IN	PCWCHAR			NewEnd)
{
	if (!WELL_FORMED_UNICODE_STRING(String) || String->Buffer == NULL) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!NewEnd) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (String->Buffer > NewEnd) {
		return STATUS_INTEGER_OVERFLOW;
	}

	if (KexRtlEndOfUnicodeString(String) > NewEnd) {
		return STATUS_INVALID_PARAMETER;
	}

	String->MaximumLength = ((USHORT) (NewEnd - String->Buffer)) * sizeof(WCHAR);
	ASSERT (VALID_UNICODE_STRING(String));

	return STATUS_SUCCESS;
}

KEXAPI NTSTATUS NTAPI KexRtlShiftUnicodeString(
	IN OUT	PUNICODE_STRING	String,
	IN		USHORT			ShiftCch,
	IN		WCHAR			LeftFillCharacter OPTIONAL)
{
	USHORT ShiftCb;
	NTSTATUS Status;

	if (!String || !ShiftCch) {
		return STATUS_INVALID_PARAMETER;
	}

	ShiftCb = ShiftCch * sizeof(WCHAR);

	if (ShiftCb > String->MaximumLength) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	if (!LeftFillCharacter) {
		LeftFillCharacter = ' ';
	}

	Status = STATUS_SUCCESS;

	if (String->Length + ShiftCb > String->MaximumLength) {
		String->Length = String->MaximumLength - ShiftCb;
		Status = STATUS_BUFFER_OVERFLOW;
	}

	RtlMoveMemory(String->Buffer + ShiftCch, String->Buffer, String->Length);
	__stosw((PUSHORT) String->Buffer, LeftFillCharacter, ShiftCch);
	String->Length += ShiftCb;

	return Status;
}

KEXAPI ULONG NTAPI KexRtlRemoteProcessBitness(
	IN	HANDLE	ProcessHandle)
{
	NTSTATUS Status;
	ULONG_PTR Peb32;

	if (KexRtlOperatingSystemBitness() == 32) {
		return 32;
	}

	Status = NtQueryInformationProcess(
		ProcessHandle,
		ProcessWow64Information,
		&Peb32,
		sizeof(Peb32),
		NULL);

	ASSERT (NT_SUCCESS(Status));

	if (NT_SUCCESS(Status) && Peb32) {
		return 32;
	} else {
		return 64;
	}
}

#ifndef KEX_ARCH_X64

#define EMIT(a) __asm __emit (a)

#define X64_Start_with_CS(_cs) \
    { \
    EMIT(0x6A) EMIT(_cs)                         /*  push   _cs             */ \
    EMIT(0xE8) EMIT(0) EMIT(0) EMIT(0) EMIT(0)   /*  call   $+5             */ \
    EMIT(0x83) EMIT(4) EMIT(0x24) EMIT(5)        /*  add    dword [esp], 5  */ \
    EMIT(0xCB)                                   /*  retf                   */ \
    }

#define X64_End_with_CS(_cs) \
    { \
    EMIT(0xE8) EMIT(0) EMIT(0) EMIT(0) EMIT(0)                                 /*  call   $+5                   */ \
    EMIT(0xC7) EMIT(0x44) EMIT(0x24) EMIT(4) EMIT(_cs) EMIT(0) EMIT(0) EMIT(0) /*  mov    dword [rsp + 4], _cs  */ \
    EMIT(0x83) EMIT(4) EMIT(0x24) EMIT(0xD)                                    /*  add    dword [rsp], 0xD      */ \
    EMIT(0xCB)                                                                 /*  retf                         */ \
    }

#define X64_Start() X64_Start_with_CS(0x33)
#define X64_End() X64_End_with_CS(0x23)

#define _RAX  0
#define _RCX  1
#define _RDX  2
#define _RBX  3
#define _RSP  4
#define _RBP  5
#define _RSI  6
#define _RDI  7
#define _R8   8
#define _R9   9
#define _R10 10
#define _R11 11
#define _R12 12
#define _R13 13
#define _R14 14
#define _R15 15

#define X64_Push(r) EMIT(0x48 | ((r) >> 3)) EMIT(0x50 | ((r) & 7))
#define X64_Pop(r) EMIT(0x48 | ((r) >> 3)) EMIT(0x58 | ((r) & 7))

#define REX_W EMIT(0x48) __asm

//to fool M$ inline asm compiler I'm using 2 DWORDs instead of DWORD64
//use of DWORD64 will generate wrong 'pop word ptr[]' and it will break stack
typedef union _reg64 {
	DWORD64 v;
	DWORD dw[2];
} TYPEDEF_TYPE_NAME(reg64);

#define PTR_TO_DWORD64(p) ((DWORD64)(ULONG_PTR)(p))

#pragma warning(push)
#pragma warning(disable : 4409)
DWORD64 __cdecl X64Call(
	DWORD64	func,
	int		argC, ...)
{
	// All variable declarations must appear at the beginning of the block
	va_list args;
	reg64 _rcx;
	reg64 _rdx;
	reg64 _r8;
	reg64 _r9;
	reg64 _rax;
	reg64 restArgs;
	reg64 _argC;
	DWORD back_esp;
	WORD back_fs;

	// Process variable arguments
	va_start(args, argC);

	// Load first four arguments (x64 calling convention: rcx, rdx, r8, r9)
	_rcx.v = (argC > 0) ? (argC--, va_arg(args, DWORD64)) : 0;
	_rdx.v = (argC > 0) ? (argC--, va_arg(args, DWORD64)) : 0;
	_r8.v = (argC > 0) ? (argC--, va_arg(args, DWORD64)) : 0;
	_r9.v = (argC > 0) ? (argC--, va_arg(args, DWORD64)) : 0;

	_rax.v = 0;   // will hold return value

				  // Get address of remaining arguments (to be pushed on stack)
	restArgs.v = PTR_TO_DWORD64(&va_arg(args, DWORD64));

	// Store number of remaining arguments for stack cleanup
	_argC.v = (DWORD64)argC;

	back_esp = 0;
	back_fs = 0;

	// Inline assembly (MSVC extension)
	__asm
	{
		;// reset FS segment, to properly handle RFG
		mov    back_fs, fs
			mov    eax, 0x2B
			mov    fs, ax

			;// keep original esp in back_esp variable
		mov    back_esp, esp

			;// align esp to 0x10 (required for SSE instructions)
		and    esp, 0xFFFFFFF0

			X64_Start();

		;// The following code is compiled as x86 inline asm but executed as x64 code.
		;// Fill first four arguments into registers
		REX_W mov    ecx, _rcx.dw[0];// mov rcx, qword ptr [_rcx]
		REX_W mov    edx, _rdx.dw[0];// mov rdx, qword ptr [_rdx]
		push   _r8.v;// push qword ptr [_r8]
		X64_Pop(_R8); ;// pop  r8
		push   _r9.v;// push qword ptr [_r9]
		X64_Pop(_R9); ;// pop  r9

		REX_W mov    eax, _argC.dw[0];// mov rax, qword ptr [_argC]

		;// Adjust stack so that number of arguments above 4 is even (for alignment)
		test   al, 1
			jnz    _no_adjust
			sub    esp, 8;// sub rsp, 8
	_no_adjust:

		push   edi;// push rdi
		REX_W mov    edi, restArgs.dw[0];// mov rdi, qword ptr [restArgs]

		;// Push remaining arguments onto the stack (right-to-left)
		REX_W test   eax, eax
			jz     _ls_e
			REX_W lea    edi, dword ptr[edi + 8 * eax - 8];// lea rdi, [rdi + rax*8 - 8]
	_ls:
		REX_W test   eax, eax
			jz     _ls_e
			push   dword ptr[edi];// push qword ptr [rdi]
		REX_W sub    edi, 8;// sub rdi, 8
		REX_W sub    eax, 1;// sub rax, 1
		jmp    _ls
			_ls_e :

		;// Reserve 32-byte shadow space (home space) for the callee
		REX_W sub    esp, 0x20;// sub rsp, 20h

		call   func;// call qword ptr [func]

		;// Cleanup: remove pushed arguments and shadow space
		REX_W mov    ecx, _argC.dw[0];// mov rcx, qword ptr [_argC]
		REX_W lea    esp, dword ptr[esp + 8 * ecx + 0x20];// lea rsp, [rsp + rcx*8 + 20h]

		pop    edi;// pop rdi

		;// Save return value
		REX_W mov    _rax.dw[0], eax;// mov qword ptr [_rax], rax

		X64_End();

		;// Restore original stack and segment registers
		mov    ax, ds
			mov    ss, ax
			mov    esp, back_esp

			mov    ax, back_fs
			mov    fs, ax
	}

	return _rax.v;
}

KEXAPI NTSTATUS NTAPI KexRtlWow64WriteProcessMemory64(
	IN	HANDLE		ProcessHandle,
	IN	ULONGLONG	Destination,
	IN	PVOID		Source,
	IN	SIZE_T		Cb)
{
	PVOID64 DestinationPageAddress;
	ULONGLONG DestinationPageSize;
	ULONG OldProtect;
	NTSTATUS Status;
	NTSTATUS Status2;
	DWORD64 NativeNtProtectVirtualMemoryAddress;
	NT_WOW64_WRITE_VIRTUAL_MEMORY64 NtWow64WriteVirtualMemory64;
	ANSI_STRING NtWow64WriteVirtualMemory64Name;

	DestinationPageAddress = (PVOID64) Destination;
	DestinationPageSize = Cb;

	//
	// Note to future self: NtProtectVirtualMemory can return
	// STATUS_CONFLICTING_ADDRESSES when the address specified in the remote
	// process is not allocated.
	//

	RtlInitConstantAnsiString(&NtWow64WriteVirtualMemory64Name, "NtWow64WriteVirtualMemory64");

	Status = LdrGetProcedureAddress(
		KexLdrGetSystemDllBase(),
		&NtWow64WriteVirtualMemory64Name,
		0,
		(PPVOID) &NtWow64WriteVirtualMemory64);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (NtWow64WriteVirtualMemory64 != NULL);

	Status = KexLdrMiniGetProcedureAddress(
		KexLdrGetNativeSystemDllBase(),
		"NtProtectVirtualMemory",
		(PVOID64*) &NativeNtProtectVirtualMemoryAddress);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (NativeNtProtectVirtualMemoryAddress != 0);

	Status = (NTSTATUS)X64Call(
		NativeNtProtectVirtualMemoryAddress,
		5, // Number of arguments
		(DWORD64)ProcessHandle,
		(DWORD64)&DestinationPageAddress,
		(DWORD64)&DestinationPageSize,
		(DWORD64)PAGE_READWRITE,
		(DWORD64)&OldProtect);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status = NtWow64WriteVirtualMemory64(
		ProcessHandle,
		(PVOID64)Destination,
		Source,
		Cb,
		NULL);

	ASSERT(NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status2 = (NTSTATUS)X64Call(
		NativeNtProtectVirtualMemoryAddress,
		5, // Number of arguments
		(DWORD64)ProcessHandle,
		(DWORD64)&DestinationPageAddress,
		(DWORD64)&DestinationPageSize,
		(DWORD64)OldProtect,
		(DWORD64)&OldProtect);

	ASSERT (NT_SUCCESS(Status2));

	return Status;
}

#endif

//
// This API will automatically change the memory protections for you
// and then set them back to what they originally were.
//
KEXAPI NTSTATUS NTAPI KexRtlWriteProcessMemory(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	Destination,
	IN	PVOID		Source,
	IN	SIZE_T		Cb)
{
	PVOID DestinationPageAddress;
	SIZE_T DestinationPageSize;
	ULONG OldProtect;
	NTSTATUS Status;
	NTSTATUS Status2;

	DestinationPageAddress = (PVOID) Destination;
	DestinationPageSize = Cb;

	//
	// Note to future self: NtProtectVirtualMemory can return
	// STATUS_CONFLICTING_ADDRESSES when the address specified in the remote
	// process is not allocated.
	//

	Status = NtProtectVirtualMemory(
		ProcessHandle,
		&DestinationPageAddress,
		&DestinationPageSize,
		PAGE_READWRITE,
		&OldProtect);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status = NtWriteVirtualMemory(
		ProcessHandle,
		(PVOID) Destination,
		Source,
		Cb,
		NULL);

	ASSERT (NT_SUCCESS(Status));

	Status2 = NtProtectVirtualMemory(
		ProcessHandle,
		&DestinationPageAddress,
		&DestinationPageSize,
		OldProtect,
		&OldProtect);

	ASSERT (NT_SUCCESS(Status2));

	return Status;
}

//
// Recursively create or open a directory.
//
KEXAPI NTSTATUS NTAPI KexRtlCreateDirectoryRecursive(
	OUT	PHANDLE				DirectoryHandle,
	IN	ACCESS_MASK			DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	ULONG				ShareAccess)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;
	BOOLEAN AlreadyRetried;

	AlreadyRetried = FALSE;

	//
	// Attempt to create the directory.
	//

Retry:
	Status = NtCreateFile(
		DirectoryHandle,
		DesiredAccess | SYNCHRONIZE,
		ObjectAttributes,
		&IoStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		ShareAccess,
		FILE_OPEN_IF,
		FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);

	if (!NT_SUCCESS(Status) && !AlreadyRetried) {
		HANDLE TemporaryHandle;
		OBJECT_ATTRIBUTES NewObjectAttributes;
		UNICODE_STRING ShorterPath;
		ULONG NewLength;

		//
		// If failed, chop off the last path element and try again.
		//

		NewObjectAttributes = *ObjectAttributes;
		ShorterPath = *ObjectAttributes->ObjectName;
		NewObjectAttributes.ObjectName = &ShorterPath;

		if (!ShorterPath.Length) {
			//
			// Already chopped off all path elements, so that means the root
			// of the path must not exist.
			//

			return STATUS_OBJECT_PATH_NOT_FOUND;
		}

		Status = RtlGetLengthWithoutLastFullDosOrNtPathElement(
			0,
			&ShorterPath,
			&NewLength);

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		NewLength *= sizeof(WCHAR);
		ASSERT (NewLength < ShorterPath.Length);

		ShorterPath.Length = (USHORT) NewLength;

		Status = KexRtlCreateDirectoryRecursive(
			&TemporaryHandle,
			0,
			&NewObjectAttributes,
			0);

		if (NT_SUCCESS(Status)) {
			//
			// If we succeeded, now go back and retry creating the original
			// directory again.
			//

			NtClose(TemporaryHandle);
			AlreadyRetried = TRUE;
			goto Retry;
		}
	}

	return Status;
}

//
// See ntdll!RtlSectionTableFromVirtualAddress for more info.
// It's a shame that function wasn't exported because I could've avoided
// rewriting a function that does the same thing.
//
// Returns a pointer to an IMAGE_SECTION_HEADER structure on success.
// Returns NULL on failure.
//
KEXAPI PIMAGE_SECTION_HEADER NTAPI KexRtlSectionTableFromRva(
	IN	PIMAGE_NT_HEADERS	NtHeaders,
	IN	ULONG				ImageRva)
{
	PIMAGE_SECTION_HEADER SectionHeader;
	ULONG NumberOfSections;
	ULONG SectionIndex;
	ULONG SectionRva;

	SectionIndex = 0;
	NumberOfSections = NtHeaders->FileHeader.NumberOfSections;

	if (NumberOfSections == 0) {
		// There are no sections in the image.
		return NULL;
	}

	//
	// The first section header in the section table starts at the byte
	// directly after the optional header.
	//

	SectionHeader = (PIMAGE_SECTION_HEADER) RVA_TO_VA(
		&NtHeaders->OptionalHeader,
		NtHeaders->FileHeader.SizeOfOptionalHeader);

	//
	// Search through all the sections and find one that contains our RVA.
	//

	while (SectionIndex < NumberOfSections) {
		SectionRva = SectionHeader->VirtualAddress;

		if (ImageRva >= SectionRva && ImageRva < SectionRva + SectionHeader->SizeOfRawData) {
			return SectionHeader;
		}

		++SectionIndex;
		++SectionHeader;
	}

	// The section could not be found.
	return NULL;
}

//
// Find a section header from the name of the section, e.g. .qtmimed or .text
// The section name is case sensitive.
//
KEXAPI PIMAGE_SECTION_HEADER NTAPI KexRtlSectionTableFromName(
	IN	PIMAGE_NT_HEADERS	NtHeaders,
	IN	PCANSI_STRING		SectionName)
{
	PIMAGE_SECTION_HEADER SectionHeader;
	BYTE DesiredName[IMAGE_SIZEOF_SHORT_NAME];
	ULONG NumberOfSections;
	ULONG SectionIndex;

	SectionIndex = 0;
	NumberOfSections = NtHeaders->FileHeader.NumberOfSections;

	if (NumberOfSections == 0) {
		// No sections in this image.
		return NULL;
	}

	if (SectionName->Length > IMAGE_SIZEOF_SHORT_NAME) {
		// There will never be a section which matches this name, since section
		// names are limited to 8 ASCII characters.
		return NULL;
	}

	//
	// Form the DesiredName array by zero-padding the input SectionName string.
	// Section names are zero-padded.
	//

	KexRtlZeroMemory(DesiredName, sizeof(DesiredName));
	KexRtlCopyMemory(DesiredName, SectionName->Buffer, SectionName->Length);

	//
	// Scan each section and check if the name matches.
	//

	SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);

	while (SectionIndex < NumberOfSections) {
		STATIC_ASSERT (sizeof(DesiredName) == RTL_FIELD_SIZE(IMAGE_SECTION_HEADER, Name));

		if (RtlEqualMemory(SectionHeader->Name, DesiredName, sizeof(DesiredName))) {
			// Found it.
			return SectionHeader;
		}

		++SectionIndex;
		++SectionHeader;
	}

	return NULL;
}

// This function ensures that String->Buffer member is suitable for passing to
// functions that expect C-strings.
KEXAPI NTSTATUS NTAPI KexRtlNullTerminateUnicodeString(
	IN	PUNICODE_STRING	String)
{
	if (!String || !String->Buffer) {
		return STATUS_INVALID_PARAMETER;
	}

	if (String->MaximumLength < String->Length) {
		return STATUS_INVALID_PARAMETER;
	}

	if (String->MaximumLength - String->Length < sizeof(WCHAR)) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	*KexRtlEndOfUnicodeString(String) = '\0';
	return STATUS_SUCCESS;
}

// This function returns TRUE if a UNICODE_STRING contains embedded null characters
// and FALSE if it does not.
KEXAPI BOOLEAN NTAPI KexRtlUnicodeStringContainsEmbeddedNull(
	IN	PUNICODE_STRING	String)
{
	ULONG Index;

	ASSERT (VALID_UNICODE_STRING(String));

	for (Index = 0; Index < KexRtlUnicodeStringCch(String); ++Index) {
		// ensure we don't go past the end of the buffer
		ASSERT (&String->Buffer[Index] < KexRtlEndOfUnicodeString(String));

		if (String->Buffer[Index] == '\0') {
			return TRUE;
		}
	}

	return FALSE;
}

// Create an object directory which is accessible to untrusted processes.
KEXAPI NTSTATUS NTAPI KexRtlCreateUntrustedDirectoryObject(
	OUT	PHANDLE				DirectoryHandle,
	IN	ACCESS_MASK			DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes)
{
	NTSTATUS Status;
	SECURITY_DESCRIPTOR SecurityDescriptor;
	BYTE UntrustedSidBuffer[] = {1, 1, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0};
	BYTE SaclBuffer[sizeof(UntrustedSidBuffer) + sizeof(ACL) + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK)];
	PSID UntrustedSid;
	PACL Sacl;

	ASSERT (DirectoryHandle != NULL);
	ASSERT (ObjectAttributes != NULL);
	ASSERT (ObjectAttributes->SecurityDescriptor == NULL);

	UntrustedSid = (PSID)UntrustedSidBuffer;
	Sacl = (PACL) SaclBuffer;

	Status = RtlCreateAcl(Sacl, sizeof(SaclBuffer), ACL_REVISION);
	ASSERT (NT_SUCCESS(Status));
	Status = RtlAddMandatoryAce(Sacl, ACL_REVISION, 0, UntrustedSid, SYSTEM_MANDATORY_LABEL_ACE_TYPE, 0);
	ASSERT (NT_SUCCESS(Status));

	Status = RtlCreateSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
	ASSERT (NT_SUCCESS(Status));
	Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, NULL, FALSE);
	ASSERT (NT_SUCCESS(Status));
	Status = RtlSetSaclSecurityDescriptor(&SecurityDescriptor, TRUE, Sacl, FALSE);
	ASSERT (NT_SUCCESS(Status));

	ObjectAttributes->SecurityDescriptor = &SecurityDescriptor;

	Status = NtCreateDirectoryObject(
		DirectoryHandle,
		DesiredAccess,
		ObjectAttributes);

	ObjectAttributes->SecurityDescriptor = NULL;

	return Status;
}

// Compatible with RtlSetBit from win8+.
KEXAPI VOID NTAPI KexRtlSetBit(
	IN	PRTL_BITMAP	BitmapHeader,
	IN	ULONG		BitNumber)
{
	_bittestandset((PLONG) BitmapHeader->Buffer, BitNumber);
}

// Compatible with RtlClearBit from win8+.
KEXAPI VOID NTAPI KexRtlClearBit(
	IN	PRTL_BITMAP	BitmapHeader,
	IN	ULONG		BitNumber)
{
	_bittestandreset((PLONG) BitmapHeader->Buffer, BitNumber);
}

KEXAPI VOID NTAPI KexRtlGetDeviceFamilyInfoEnum(
	OUT	PULONGLONG	UAPInfo OPTIONAL,
	OUT	PULONG		DeviceFamily OPTIONAL,
	OUT	PULONG		DeviceForm OPTIONAL)
{
	if (UAPInfo != NULL) {
		// The 3570 is an approximate number from Win10 build 19042
		// UBR = update build revision
		*UAPInfo = (NtCurrentPeb()->OSBuildNumber << 16) + 3570;
	}

	if (DeviceFamily) {
		*DeviceFamily =	DEVICEFAMILYINFOENUM_DESKTOP;
	}

	if (DeviceForm) {
		*DeviceForm = DEVICEFAMILYDEVICEFORM_DESKTOP;
	}
}

// Microsoft documentation incorrectly lists TargetPath as an IN parameter, but it
// is actually an OUT parameter.
KEXAPI NTSTATUS NTAPI KexRtlGetPersistedStateLocation(
	IN	PCWSTR				SourceID,
	IN	PCWSTR				CustomValue OPTIONAL,
	IN	PCWSTR				DefaultPath OPTIONAL,
	IN	STATE_LOCATION_TYPE	StateLocationType,
	OUT	PWCHAR				TargetPath,
	IN	ULONG				BufferCbIn,
	OUT	PULONG				BufferCbOut OPTIONAL)
{
	if (StateLocationType >= StateLocationTypeMaximum) {
		return STATUS_INVALID_PARAMETER_3;
	}

	if (DefaultPath) {
		ULONG DefaultPathCbWithNullTerminator;

		//
		// Fun fact, the Win11 NTDLL checks for integer overflow while doing these
		// string length calculations and can return STATUS_INTEGER_OVERFLOW.
		//
		// Of course we won't be doing that because any app that passes in a >2GB
		// string which is supposed to be representing a registry path is already
		// broken beyond repair.
		//

		DefaultPathCbWithNullTerminator = ((ULONG) wcslen(DefaultPath) + 1) * sizeof(WCHAR);

		if (BufferCbOut) {
			*BufferCbOut = DefaultPathCbWithNullTerminator;
		}

		if (DefaultPathCbWithNullTerminator > BufferCbIn) {
			return STATUS_BUFFER_OVERFLOW;
		}

		RtlMoveMemory(TargetPath, DefaultPath, DefaultPathCbWithNullTerminator);
		return STATUS_SUCCESS;
	}

	KexLogDebugEvent(
		L"Failed attempt to find a persisted state location\r\n\r\n"
		L"SourceID = %s\r\n"
		L"CustomValue = %s\r\n"
		L"DefaultPath = %s\r\n"
		L"StateLocationType = %d\r\n"
		L"TargetPath = 0x%p\r\n"
		L"BufferCbIn = %d\r\n"
		L"BufferCbOut = 0x%p",
		SourceID,
		CustomValue,
		DefaultPath,
		StateLocationType,
		TargetPath,
		BufferCbIn,
		BufferCbOut);

	//
	// This is the error code that Win11 NTDLL returns when DefaultPath is not
	// supplied and it cannot find \Registry\Machine\System\CurrentControlSet\
	// Control\StateSeparation\RedirectionMap\{Keys,Files} where Keys or Files
	// is based on the StateLocationType parameter.
	//
	// If it CAN find this registry location, then it will give a target path
	// based on a subkey read out of this location. Windows 7 of course does not
	// have this redirection map, so we don't even try to find it.
	//

	return STATUS_OBJECT_NAME_NOT_FOUND;
}

#ifndef _M_X64
typedef PVOID TYPEDEF_TYPE_NAME(RUNTIME_FUNCTION);
#endif

KEXAPI NTSTATUS NTAPI KexRtlAddGrowableFunctionTable(
	OUT	PPVOID				DynamicTable,
	IN	PRUNTIME_FUNCTION	FunctionTable,
	IN	ULONG				EntryCount,
	IN	ULONG				MaximumEntryCount,
	IN	ULONG_PTR			RangeBase,
	IN	ULONG_PTR			RangeEnd)
{
#ifdef _M_X64
	BOOLEAN Success;

	Success = RtlAddFunctionTable(FunctionTable, EntryCount, RangeBase);

	if (Success) {
		*DynamicTable = NULL;
		return STATUS_SUCCESS;
	} else {
		return STATUS_UNSUCCESSFUL;
	}
#else
	ASSERT (FALSE);
	return STATUS_NOT_IMPLEMENTED;
#endif
}

KEXAPI VOID NTAPI KexRtlDeleteGrowableFunctionTable(
	IN	PVOID		DynamicTable)
{
#ifdef _M_X64
	RtlDeleteFunctionTable(DynamicTable);
	return;
#else
	ASSERT (FALSE);
	return;
#endif
}

KEXAPI LONGLONG NTAPI KexRtlGetSystemTimePrecise(
	VOID)
{
	LONGLONG CurrentTime;
	NtQuerySystemTime(&CurrentTime);
	return CurrentTime;
}

//
// Full implementation based on Win10 NTDLL decompilation.
//
KEXAPI NTSTATUS NTAPI KexRtlCanonicalizeDomainName(
	OUT	PUNICODE_STRING		DestinationString,
	IN	PCUNICODE_STRING	SourceString,
	IN	BOOLEAN				Strict)
{
	BOOLEAN Success;
	NTSTATUS Status;
	ULONG Index;
	ULONG ScopeId;
	UNICODE_STRING RawName;
	USHORT Port;
	IN_ADDR Ipv4Address;
	IN6_ADDR Ipv6Address;
	ULONG CanonicalNameLength;
	ULONG PunycodedNameLength;
	WCHAR CanonicalNameBuffer[256];
	WCHAR PunycodedNameBuffer[256];
	WCHAR RawNameBuffer[256];

	CanonicalNameLength = ARRAYSIZE(CanonicalNameBuffer);
	PunycodedNameLength = ARRAYSIZE(PunycodedNameBuffer);

	RtlInitEmptyUnicodeString(&RawName, RawNameBuffer, sizeof(RawNameBuffer));
	RtlCopyUnicodeString(&RawName, SourceString);

	if (RawName.Length == RawName.MaximumLength) {
		return STATUS_INVALID_IDN_NORMALIZATION;
	}

	//
	// Try parse as IPv6.
	//

	Status = RtlIpv6StringToAddressExW(
		RawName.Buffer,
		&Ipv6Address,
		&ScopeId,
		&Port);

	if (NT_SUCCESS(Status) && Port == 0) {
		//
		// We could parse this address as IPv6.
		// Convert the IPv6 struct back into a string - as an IPv6 string if it
		// is a true IPv6 address, or an IPv4 string if it is a mapped IPv4 address.
		//

		if (IN6_IS_ADDR_V4MAPPED(&Ipv6Address) && ScopeId == 0) {
			// Convert the IPv6-formatted IPv4 address into a real IPv4 address
			RtlCopyMemory(
				&Ipv4Address,
				IN6_GET_ADDR_V4MAPPED(&Ipv6Address),
				sizeof(Ipv4Address));

			Status = RtlIpv4AddressToStringExW(
				&Ipv4Address,
				Port,
				CanonicalNameBuffer,
				&CanonicalNameLength);
		} else {
			Status = RtlIpv6AddressToStringExW(
				&Ipv6Address,
				ScopeId,
				Port,
				CanonicalNameBuffer,
				&CanonicalNameLength);
		}

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		Success = RtlCreateUnicodeString(DestinationString, CanonicalNameBuffer);
		Status = Success ? STATUS_SUCCESS : STATUS_NO_MEMORY;
		return Status;
	}

	//
	// Try parse as IPv4.
	//

	Status = RtlIpv4StringToAddressExW(
		RawName.Buffer,
		Strict,
		&Ipv4Address,
		&Port);

	if (NT_SUCCESS(Status) && Port == 0) {
		//
		// We could parse the string as IPv4. Convert it back to a string.
		//

		Status = RtlIpv4AddressToStringExW(
			&Ipv4Address,
			Port,
			CanonicalNameBuffer,
			&CanonicalNameLength);

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		Success = RtlCreateUnicodeString(DestinationString, CanonicalNameBuffer);
		Status = Success ? STATUS_SUCCESS : STATUS_NO_MEMORY;
		return Status;
	}

	//
	// Try parse as IDN (internationalized domain name), and convert to punycode
	//

	Status = RtlIdnToAscii(
		0,
		SourceString->Buffer,
		KexRtlUnicodeStringCch(SourceString),
		PunycodedNameBuffer,
		&PunycodedNameLength);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Lowercase the Punycode representation
	//

	for (Index = 0; Index < PunycodedNameLength; ++Index) {
		// Note: we're using towlower (ntdll CRT) instead of ToLower (vxkex macro)
		// because ToLower does not handle non-ASCII characters.
		PunycodedNameBuffer[Index] = towlower(PunycodedNameBuffer[Index]);
	}

	//
	// Convert it back to proper Unicode
	//

	Status = RtlIdnToUnicode(
		0,
		PunycodedNameBuffer,
		PunycodedNameLength,
		CanonicalNameBuffer,
		&CanonicalNameLength);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	if (CanonicalNameLength >= ARRAYSIZE(CanonicalNameBuffer)) {
		// potential buffer overflow
		return STATUS_INVALID_IDN_NORMALIZATION;
	}

	// Ensure null termination.
	// I'm not sure whether RtlIdnToUnicode guarantees a null terminated buffer,
	// but since it works with explicit length variables, it probably doesn't.
	// Win10 code does do this so it's probably required.
	CanonicalNameBuffer[CanonicalNameLength] = '\0';

	Success = RtlCreateUnicodeString(DestinationString, CanonicalNameBuffer);
	Status = Success ? STATUS_SUCCESS : STATUS_NO_MEMORY;
	return Status;
}

//
// This is just kernel32!IsProcessorFeaturePresent on win7, but they renamed it
// and moved it to NTDLL on Windows 10 and higher.
//
KEXAPI BOOLEAN NTAPI KexRtlIsProcessorFeaturePresent(
	IN	ULONG	ProcessorFeature)
{
	if (ProcessorFeature >= PROCESSOR_FEATURE_MAX) {
		return FALSE;
	}

	return SharedUserData->ProcessorFeatures[ProcessorFeature];
}

//
// Stubs.
//

KEXAPI NTSTATUS NTAPI KexRtlQueryPackageIdentity(
	IN		PVOID		TokenObject,
	OUT		PWSTR		PackageFullName,
	IN OUT	PSIZE_T		PackageSize,
	OUT		PWSTR		AppId,
	IN OUT	PSIZE_T		AppIdSize,
	OUT		PBOOLEAN	Packaged)
{
	return STATUS_NOT_FOUND;
}

KEXAPI NTSTATUS NTAPI KexRtlQueryPackageIdentityEx(
	IN		PVOID		TokenObject,
	OUT		PWSTR		PackageFullName,
	IN OUT	PSIZE_T		PackageSize,
	OUT		PWSTR		AppId,
	IN OUT	PSIZE_T		AppIdSize,
	OUT		LPGUID		DynamicId OPTIONAL,
	OUT		PULONG64	Flags)
{
	return STATUS_NOT_FOUND;
}

KEXAPI NTSTATUS NTAPI KexRtlCheckPortableOperatingSystem(
	OUT	PBOOLEAN	IsPortable)
{
	*IsPortable = FALSE;
	return STATUS_SUCCESS;
}

KEXAPI NTSTATUS NTAPI KexRtlUnsubscribeWnfNotificationWaitForCompletion(
	IN	PVOID	Subscription)
{
	return STATUS_SUCCESS;
}

KEXAPI NTSTATUS NTAPI KexRtlUnsubscribeWnfStateChangeNotification(
	IN	PVOID	Subscription)
{
	return STATUS_NOT_IMPLEMENTED;
}

KEXAPI NTSTATUS NTAPI KexRtlQueryWnfStateData(
	PULONG		ChangeStamp,
	ULONGLONG	StateName,
	PVOID		Callback,
	PVOID		CallbackContext,
	PULONG		TypeId)
{
	return STATUS_NOT_IMPLEMENTED;
}

KEXAPI NTSTATUS NTAPI KexRtlPublishWnfStateData(
	ULONGLONG	StateName,
	PVOID		TypeId,
	PVOID		StateData,
	ULONG		StateDataLength,
	PCVOID		ExplicitScope)
{
	return STATUS_NOT_IMPLEMENTED;
}

KEXAPI NTSTATUS NTAPI KexRtlSubscribeWnfStateChangeNotification(
	PVOID		Subscription,
	ULONGLONG	StateName,
	ULONG		ChangeStamp,
	PVOID		Callback,
	PVOID		CallbackContext,
	PVOID		TypeId,
	ULONG		SerializationGroupIndex)
{
	return STATUS_NOT_IMPLEMENTED;
}