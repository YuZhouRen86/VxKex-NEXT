#include "buildcfg.h"
#include "kexdllp.h"

DWORD SSN_NtQuerySystemTime;
DWORD SSN_NtCreateUserProcess;
DWORD SSN_NtProtectVirtualMemory;
DWORD SSN_NtAllocateVirtualMemory;
DWORD SSN_NtQueryVirtualMemory;
DWORD SSN_NtFreeVirtualMemory;
DWORD SSN_NtQueryObject;
DWORD SSN_NtOpenFile;
DWORD SSN_NtWriteFile;
DWORD SSN_NtRaiseHardError;
DWORD SSN_NtQueryInformationThread;
DWORD SSN_NtSetInformationThread;
DWORD SSN_NtNotifyChangeKey;
DWORD SSN_NtNotifyChangeMultipleKeys;
DWORD SSN_NtCreateSection;
DWORD SSN_NtQueryInformationProcess;
DWORD SSN_NtAssignProcessToJobObject;
DWORD SSN_NtMapViewOfSection;

// Get SSN from the original ntdll.dll file on disk.
// Uses only ntdll exported APIs. Immune to in-memory hooks.
BOOL GetSsnByName(
	PCSTR	FuncName,
	PDWORD	SsnPtr)
{
	NTSTATUS Status;
	HANDLE FileHandle;
	HANDLE SectionHandle;
	PVOID ImageBase;
	SIZE_T ViewSize;
	BOOL Result;
	UNICODE_STRING UnicodePath;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	PIMAGE_NT_HEADERS NtHeaders;
	ULONG ExportSize;
	PIMAGE_EXPORT_DIRECTORY ExportDir;
	PDWORD NameArray;
	PWORD OrdinalArray;
	PDWORD FuncArray;
	DWORD Index;
	DWORD FuncRva;
	PBYTE FuncBytes;
	INT Index1;
	INT Index2;
	DWORD Ssn = 0;

	// Initialize.
	Result = FALSE;
	FileHandle = NULL;
	SectionHandle = NULL;
	ImageBase = NULL;
	ViewSize = 0;
	ExportSize = 0;
	if (!SsnPtr || !FuncName) return FALSE;

	// 1. Open ntdll.dll via NT object path (no drive letter).
	RtlInitUnicodeString(&UnicodePath, L"\\SystemRoot\\System32\\ntdll.dll");
	InitializeObjectAttributes(&ObjectAttributes, &UnicodePath, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = NtCreateFile(
		&FileHandle,
		SYNCHRONIZE | FILE_READ_DATA,
		&ObjectAttributes,
		&IoStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);
	if (!NT_SUCCESS(Status)) goto Cleanup;

	// 2. Create a section with SEC_IMAGE flag.
	//    This maps the original disk file bytes, not the hooked in-memory copy.
	Status = NtCreateSection(
		&SectionHandle,
		SECTION_MAP_READ | SECTION_MAP_EXECUTE,
		NULL,
		NULL,
		PAGE_READONLY,
		SEC_IMAGE,
		FileHandle);
	if (!NT_SUCCESS(Status)) goto Cleanup;

	// 3. Map the section view into current process.
	Status = NtMapViewOfSection(
		SectionHandle,
		NtCurrentProcess(),
		&ImageBase,
		0,
		0,
		NULL,
		&ViewSize,
		ViewUnmap,
		0,
		PAGE_READONLY);
	if (!NT_SUCCESS(Status)) goto Cleanup;

	// 4. Validate PE header.
	NtHeaders = RtlImageNtHeader(ImageBase);
	if (!NtHeaders) goto Cleanup;

	// 5. Get export directory.
	ExportDir = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
		ImageBase,
		TRUE,
		IMAGE_DIRECTORY_ENTRY_EXPORT,
		&ExportSize);
	if (!ExportDir) goto Cleanup;

	// 6. Walk export names to find the target function.
	NameArray = (PDWORD)((PBYTE)ImageBase + ExportDir->AddressOfNames);
	OrdinalArray = (PWORD)((PBYTE)ImageBase + ExportDir->AddressOfNameOrdinals);
	FuncArray = (PDWORD)((PBYTE)ImageBase + ExportDir->AddressOfFunctions);

	for (Index = 0; Index < ExportDir->NumberOfNames; ++Index) {
		PCSTR CurrentName = (PCSTR)((PBYTE)ImageBase + NameArray[Index]);
		if (!StringEqualIA(CurrentName, FuncName)) continue;

		// 7. Found the function. Get its raw code bytes from the disk image.
		FuncRva = FuncArray[OrdinalArray[Index]];
		FuncBytes = RVA_TO_VA(ImageBase, FuncRva);

		// 8. Scan the first 32 bytes for "B8 imm32" + "0F 05" (syscall).
		for (Index1 = 0; Index1 < 32; ++Index1) {
			if (FuncBytes[Index1] == 0xB8) {
				Ssn = *(PDWORD)(FuncBytes + Index1 + 1);
#ifdef KEX_ARCH_X64
				// Verify that the syscall instruction (0F 05) follows within 20 bytes.
				for (Index2 = Index1 + 5; Index2 < Index1 + 20; ++Index2) {
					if (FuncBytes[Index2] == 0x0F && FuncBytes[Index2 + 1] == 0x05) {
						*SsnPtr = Ssn;
						Result = TRUE;
						goto Cleanup;
					}
				}
#else
				// In 32-bit Windows, the actual syscall stub (sysenter/int2e) is not
				// placed inside each service function; only "mov eax, SSN" is present.
				// Therefore we accept the SSN immediately after finding the B8 opcode.
				*SsnPtr = Ssn;
				Result = TRUE;
				goto Cleanup;
				UNREFERENCED_PARAMETER(Index2);
#endif
			}
		}
		break;  // matched name but pattern not found -> exit loop
	}

Cleanup:
	// Cleanup all resources.
	if (ImageBase) {
		NtUnmapViewOfSection(NtCurrentProcess(), ImageBase);
	}
	if (SectionHandle) {
		NtClose(SectionHandle);
	}
	if (FileHandle) {
		NtClose(FileHandle);
	}
	return Result;
}

// Structure to map function names to SSN storage pointers.
typedef struct _SsnEntry {
	PCSTR	Name;
	PDWORD	SsnPtr;
} TYPEDEF_TYPE_NAME(SsnEntry);

// Global cache flag (PascalCase).
BOOL SsnInitialized = FALSE;

// Initialize all required SSNs.
BOOL InitializeSsnForAllSyscallFunctions(VOID) {
	BOOL Success;
	INT Index;
	SsnEntry Entries[] = {
		{"NtQuerySystemTime",			&SSN_NtQuerySystemTime},
		{"NtCreateUserProcess",			&SSN_NtCreateUserProcess},
		{"NtProtectVirtualMemory",		&SSN_NtProtectVirtualMemory},
		{"NtAllocateVirtualMemory",		&SSN_NtAllocateVirtualMemory},
		{"NtQueryVirtualMemory",		&SSN_NtQueryVirtualMemory},
		{"NtFreeVirtualMemory",			&SSN_NtFreeVirtualMemory},
		{"NtQueryObject",				&SSN_NtQueryObject},
		{"NtOpenFile",					&SSN_NtOpenFile},
		{"NtWriteFile",					&SSN_NtWriteFile},
		{"NtRaiseHardError",			&SSN_NtRaiseHardError},
		{"NtQueryInformationThread",	&SSN_NtQueryInformationThread},
		{"NtSetInformationThread",		&SSN_NtSetInformationThread},
		{"NtNotifyChangeKey",			&SSN_NtNotifyChangeKey},
		{"NtNotifyChangeMultipleKeys",	&SSN_NtNotifyChangeMultipleKeys},
		{"NtCreateSection",				&SSN_NtCreateSection},
		{"NtQueryInformationProcess",	&SSN_NtQueryInformationProcess},
		{"NtAssignProcessToJobObject",	&SSN_NtAssignProcessToJobObject},
		{"NtMapViewOfSection",			&SSN_NtMapViewOfSection}
	};

	if (SsnInitialized) return TRUE;

	Success = TRUE;
	for (Index = 0; Index < ARRAYSIZE(Entries); ++Index) {
		if (!GetSsnByName(Entries[Index].Name, Entries[Index].SsnPtr)) {
			Success = FALSE;
			break;
		}
	}

	if (Success) {
		SsnInitialized = TRUE;
	}
	return Success;
}

#ifdef KEX_ARCH_X64

#define CALL_SYSCALL(SyscallName, ...) \
do { \
	try { \
		if (SsnInitialized) return Kex##SyscallName##_ASM(__VA_ARGS__); \
		return SyscallName(__VA_ARGS__); \
	} except (GetExceptionCode() == STATUS_ACCESS_VIOLATION) { \
		return STATUS_ACCESS_VIOLATION; \
	} \
} while (0)

#else

#define KEXNTSYSCALLAPI __declspec(naked)

#define GENERATE_SYSCALL(SyscallName, Retn, ...) \
KEXNTSYSCALLAPI NTSTATUS NTAPI Kex##SyscallName##_Native32(__VA_ARGS__) { asm { \
	asm mov eax, [SSN_##SyscallName] \
	asm mov edx, 0x7FFE0300 \
	asm call [edx] /* Native 32 bit call */ \
	asm ret Retn \
}} \
KEXNTSYSCALLAPI NTSTATUS NTAPI Kex##SyscallName##_Wow64_Legacy(__VA_ARGS__) { asm { \
	asm mov eax, [SSN_##SyscallName] \
	asm mov ecx, 0 /* ECX value should always be 0 */ \
	asm lea edx, [esp+4] \
	asm call fs:0xC0 \
	asm add esp, 4 \
	asm ret Retn \
}} \
KEXNTSYSCALLAPI NTSTATUS NTAPI Kex##SyscallName##_Wow64_Modern(__VA_ARGS__) { asm { \
	asm mov eax, [SSN_##SyscallName] \
	asm call fs:0xC0 \
	asm ret Retn \
}}

GENERATE_SYSCALL(NtQuerySystemTime, 0x04,
	OUT		PLONGLONG	CurrentTime);

GENERATE_SYSCALL(NtCreateUserProcess, 0x2C,
	OUT		PHANDLE							ProcessHandle,
	OUT		PHANDLE							ThreadHandle,
	IN		ACCESS_MASK						ProcessDesiredAccess,
	IN		ACCESS_MASK						ThreadDesiredAccess,
	IN		POBJECT_ATTRIBUTES				ProcessObjectAttributes OPTIONAL,
	IN		POBJECT_ATTRIBUTES				ThreadObjectAttributes OPTIONAL,
	IN		ULONG							ProcessFlags,
	IN		ULONG							ThreadFlags,
	IN		PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,
	IN OUT	PPS_CREATE_INFO					CreateInfo,
	IN		PPS_ATTRIBUTE_LIST				AttributeList OPTIONAL);

GENERATE_SYSCALL(NtProtectVirtualMemory, 0x14,
	IN		HANDLE		ProcessHandle,
	IN OUT	PPVOID		BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		NewProtect,
	OUT		PULONG		OldProtect);

GENERATE_SYSCALL(NtAllocateVirtualMemory, 0x18,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN		ULONG_PTR	ZeroBits,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		AllocationType,
	IN		ULONG		Protect);

GENERATE_SYSCALL(NtQueryVirtualMemory, 0x18,
	IN		HANDLE			ProcessHandle,
	IN		PVOID			BaseAddress OPTIONAL,
	IN		MEMINFOCLASS	MemoryInformationClass,
	OUT		PVOID			MemoryInformation,
	IN		SIZE_T			MemoryInformationLength,
	OUT		PSIZE_T			ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtFreeVirtualMemory, 0x10,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		FreeType);

GENERATE_SYSCALL(NtQueryObject, 0x14,
	IN		HANDLE						ObjectHandle,
	IN		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
	OUT		PVOID						ObjectInformation,
	IN		ULONG						Length,
	OUT		PULONG						ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtOpenFile, 0x18,
	OUT		PHANDLE				FileHandle,
	IN		ACCESS_MASK			DesiredAccess,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		ULONG				ShareAccess,
	IN		ULONG				OpenOptions);

GENERATE_SYSCALL(NtWriteFile, 0x24,
	IN		HANDLE				FileHandle,
	IN		HANDLE				Event OPTIONAL,
	IN		PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN		PVOID				ApcContext OPTIONAL,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		PVOID				Buffer,
	IN		ULONG				Length,
	IN		PLONGLONG			ByteOffset OPTIONAL,
	IN		PULONG				Key OPTIONAL);

GENERATE_SYSCALL(NtRaiseHardError, 0x18,
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response);

GENERATE_SYSCALL(NtQueryInformationThread, 0x14,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtSetInformationThread, 0x10,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength);

GENERATE_SYSCALL(NtNotifyChangeKey, 0x28,
	IN	HANDLE				KeyHandle,
	IN	HANDLE				Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID				ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				CompletionFilter,
	IN	BOOLEAN				WatchTree,
	OUT	PVOID				Buffer OPTIONAL,
	IN	ULONG				BufferSize,
	IN	BOOLEAN				Asynchronous);

GENERATE_SYSCALL(NtNotifyChangeMultipleKeys, 0x30,
	IN	HANDLE				MasterKeyHandle,
	IN	ULONG				Count OPTIONAL,
	IN	OBJECT_ATTRIBUTES	SlaveObjects[] OPTIONAL,
	IN	HANDLE				Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID				ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				CompletionFilter,
	IN	BOOLEAN				WatchTree,
	OUT	PVOID				Buffer OPTIONAL,
	IN	ULONG				BufferSize,
	IN	BOOLEAN				Asynchronous);

GENERATE_SYSCALL(NtCreateSection, 0x1C,
	OUT	PHANDLE				SectionHandle,
	IN	ULONG				DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
	IN	PLONGLONG			MaximumSize OPTIONAL,
	IN	ULONG				PageAttributes,
	IN	ULONG				SectionAttributes,
	IN	HANDLE				FileHandle OPTIONAL);

GENERATE_SYSCALL(NtQueryInformationProcess, 0x14,
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtAssignProcessToJobObject, 0x08,
	IN	HANDLE				JobHandle,
	IN	HANDLE				ProcessHandle);

GENERATE_SYSCALL(NtMapViewOfSection, 0x28,
	IN		HANDLE						SectionHandle,
	IN		HANDLE						ProcessHandle,
	IN OUT	PPVOID						BaseAddress OPTIONAL,
	IN		ULONG						ZeroBits OPTIONAL,
	IN		SIZE_T						CommitSize,
	IN OUT	PLONGLONG					SectionOffset OPTIONAL,
	IN OUT	PSIZE_T						ViewSize,
	IN		SECTION_INHERIT				InheritDisposition,
	IN		ULONG						AllocationType,
	IN		ULONG						MemoryProtection);

#define CALL_SYSCALL(SyscallName, ...) \
do { \
	try { \
		if (SsnInitialized) { \
			if (KexRtlCurrentProcessBitness() != KexRtlOperatingSystemBitness()) { \
				if (KexShouldUseWorkaroundsForNewerWindows()) return Kex##SyscallName##_Wow64_Modern(__VA_ARGS__); \
				return Kex##SyscallName##_Wow64_Legacy(__VA_ARGS__); \
			} else { \
				return Kex##SyscallName##_Native32(__VA_ARGS__); \
			} \
		} \
		return SyscallName(__VA_ARGS__); \
	} except (GetExceptionCode() == STATUS_ACCESS_VIOLATION) { \
		return STATUS_ACCESS_VIOLATION; \
	} \
} while (0)

#endif

KEXAPI NTSTATUS NTAPI KexNtQuerySystemTime(
	OUT		PLONGLONG	CurrentTime)
{
	CALL_SYSCALL(NtQuerySystemTime, CurrentTime);
}

KEXAPI NTSTATUS NTAPI KexNtCreateUserProcess(
	OUT		PHANDLE							ProcessHandle,
	OUT		PHANDLE							ThreadHandle,
	IN		ACCESS_MASK						ProcessDesiredAccess,
	IN		ACCESS_MASK						ThreadDesiredAccess,
	IN		POBJECT_ATTRIBUTES				ProcessObjectAttributes OPTIONAL,
	IN		POBJECT_ATTRIBUTES				ThreadObjectAttributes OPTIONAL,
	IN		ULONG							ProcessFlags,
	IN		ULONG							ThreadFlags,
	IN		PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,
	IN OUT	PPS_CREATE_INFO					CreateInfo,
	IN		PPS_ATTRIBUTE_LIST				AttributeList OPTIONAL)
{
	CALL_SYSCALL(NtCreateUserProcess, ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
}

KEXAPI NTSTATUS NTAPI KexNtProtectVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN OUT	PPVOID		BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		NewProtect,
	OUT		PULONG		OldProtect)
{
	CALL_SYSCALL(NtProtectVirtualMemory, ProcessHandle, BaseAddress, RegionSize, NewProtect, OldProtect);
}

KEXAPI NTSTATUS NTAPI KexNtAllocateVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN		ULONG_PTR	ZeroBits,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		AllocationType,
	IN		ULONG		Protect)
{
	CALL_SYSCALL(NtAllocateVirtualMemory, ProcessHandle, BaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
}

KEXAPI NTSTATUS NTAPI KexNtQueryVirtualMemory(
	IN		HANDLE			ProcessHandle,
	IN		PVOID			BaseAddress OPTIONAL,
	IN		MEMINFOCLASS	MemoryInformationClass,
	OUT		PVOID			MemoryInformation,
	IN		SIZE_T			MemoryInformationLength,
	OUT		PSIZE_T			ReturnLength OPTIONAL)
{
	CALL_SYSCALL(NtQueryVirtualMemory, ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
}

KEXAPI NTSTATUS NTAPI KexNtFreeVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		FreeType)
{
	CALL_SYSCALL(NtFreeVirtualMemory, ProcessHandle, BaseAddress, RegionSize, FreeType);
}

KEXAPI NTSTATUS NTAPI KexNtQueryObject(
	IN		HANDLE						ObjectHandle,
	IN		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
	OUT		PVOID						ObjectInformation,
	IN		ULONG						Length,
	OUT		PULONG						ReturnLength OPTIONAL)
{
	CALL_SYSCALL(NtQueryObject, ObjectHandle, ObjectInformationClass, ObjectInformation, Length, ReturnLength);
}

KEXAPI NTSTATUS NTAPI KexNtOpenFile(
	OUT		PHANDLE				FileHandle,
	IN		ACCESS_MASK			DesiredAccess,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		ULONG				ShareAccess,
	IN		ULONG				OpenOptions)
{
	CALL_SYSCALL(NtOpenFile, FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

KEXAPI NTSTATUS NTAPI KexNtWriteFile(
	IN		HANDLE				FileHandle,
	IN		HANDLE				Event OPTIONAL,
	IN		PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN		PVOID				ApcContext OPTIONAL,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		PVOID				Buffer,
	IN		ULONG				Length,
	IN		PLONGLONG			ByteOffset OPTIONAL,
	IN		PULONG				Key OPTIONAL)
{
	CALL_SYSCALL(NtWriteFile, FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

KEXAPI NTSTATUS NTAPI KexNtRaiseHardError(
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response)
{
	CALL_SYSCALL(NtRaiseHardError, ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);
}

KEXAPI NTSTATUS NTAPI KexNtQueryInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL)
{
	CALL_SYSCALL(NtQueryInformationThread, ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
}

KEXAPI NTSTATUS NTAPI KexNtSetInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength)
{
	CALL_SYSCALL(NtSetInformationThread, ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
}

KEXAPI NTSTATUS NTAPI KexNtNotifyChangeKey(
	IN	HANDLE				KeyHandle,
	IN	HANDLE				Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID				ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				CompletionFilter,
	IN	BOOLEAN				WatchTree,
	OUT	PVOID				Buffer OPTIONAL,
	IN	ULONG				BufferSize,
	IN	BOOLEAN				Asynchronous)
{
	CALL_SYSCALL(NtNotifyChangeKey, KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
}

KEXAPI NTSTATUS NTAPI KexNtNotifyChangeMultipleKeys(
	IN	HANDLE				MasterKeyHandle,
	IN	ULONG				Count OPTIONAL,
	IN	OBJECT_ATTRIBUTES	SlaveObjects[] OPTIONAL,
	IN	HANDLE				Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID				ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				CompletionFilter,
	IN	BOOLEAN				WatchTree,
	OUT	PVOID				Buffer OPTIONAL,
	IN	ULONG				BufferSize,
	IN	BOOLEAN				Asynchronous)
{
	CALL_SYSCALL(NtNotifyChangeMultipleKeys, MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
}

KEXAPI NTSTATUS NTAPI KexNtCreateSection(
	OUT	PHANDLE				SectionHandle,
	IN	ULONG				DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
	IN	PLONGLONG			MaximumSize OPTIONAL,
	IN	ULONG				PageAttributes,
	IN	ULONG				SectionAttributes,
	IN	HANDLE				FileHandle OPTIONAL)
{
	CALL_SYSCALL(NtCreateSection, SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, PageAttributes, SectionAttributes, FileHandle);
}

KEXAPI NTSTATUS NTAPI KexNtQueryInformationProcess(
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL)
{
	CALL_SYSCALL(NtQueryInformationProcess, ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
}

KEXAPI NTSTATUS NTAPI KexNtAssignProcessToJobObject(
	IN	HANDLE				JobHandle,
	IN	HANDLE				ProcessHandle)
{
	CALL_SYSCALL(NtAssignProcessToJobObject, JobHandle, ProcessHandle);
}

KEXAPI NTSTATUS NTAPI KexNtMapViewOfSection(
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
	CALL_SYSCALL(NtMapViewOfSection, SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, MemoryProtection);
}