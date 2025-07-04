#include "buildcfg.h"
#include "kexdllp.h"

#if defined(KEX_ARCH_X64)

#define CALL_SYSCALL(SyscallName, ...) \
do { \
	try { \
		if (OriginalMajorVersion == 6 && OriginalMinorVersion == 1) return Kex##SyscallName##_Win7(__VA_ARGS__); \
		else if (OriginalMajorVersion == 6 && OriginalMinorVersion == 3) return Kex##SyscallName##_Win81(__VA_ARGS__); \
		return SyscallName(__VA_ARGS__); \
	} except (GetExceptionCode() == STATUS_ACCESS_VIOLATION) { \
		return STATUS_ACCESS_VIOLATION; \
	} \
} while (0)

#else

#define KEXNTSYSCALLAPI __declspec(naked)

#define GENERATE_SYSCALL_WIN7(SyscallName, SyscallNumber32, SyscallNumber64, EcxValue, Retn, ...) \
KEXNTSYSCALLAPI NTSTATUS NTAPI Kex##SyscallName##_Native32(__VA_ARGS__) { asm { \
	asm mov eax, SyscallNumber32 \
	asm call [edx] /* Native 32 bit call */ \
	asm ret Retn \
}} \
KEXNTSYSCALLAPI NTSTATUS NTAPI Kex##SyscallName##_Wow64(__VA_ARGS__) { asm { \
	asm mov eax, SyscallNumber64 \
	asm mov ecx, EcxValue \
	asm lea edx, [esp+4] \
	asm call fs:0xC0 \
	asm add esp, 4 \
	asm ret Retn \
}}

#define GENERATE_SYSCALL_WIN81(SyscallName, SyscallNumber32, SyscallNumber64, Retn, ...) \
KEXNTSYSCALLAPI NTSTATUS NTAPI Kex##SyscallName##_Native32(__VA_ARGS__) { asm { \
	asm mov eax, SyscallNumber32 \
	asm call [edx] /* Native 32 bit call */ \
	asm ret Retn \
}} \
KEXNTSYSCALLAPI NTSTATUS NTAPI Kex##SyscallName##_Wow64(__VA_ARGS__) { asm { \
	asm mov eax, SyscallNumber64 \
	asm call fs:0xC0 \
	asm ret Retn \
}}

GENERATE_SYSCALL_WIN7(NtQuerySystemTime_Win7,			0x0107, 0x0057, 0x18, 0x04,
	OUT		PLONGLONG	CurrentTime);

GENERATE_SYSCALL_WIN7(NtCreateUserProcess_Win7,			0x005D, 0x00AA, 0x00, 0x2C,
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

GENERATE_SYSCALL_WIN7(NtProtectVirtualMemory_Win7,		0x00D7, 0x004D, 0x00, 0x14,
	IN		HANDLE		ProcessHandle,
	IN OUT	PPVOID		BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		NewProtect,
	OUT		PULONG		OldProtect);

GENERATE_SYSCALL_WIN7(NtAllocateVirtualMemory_Win7,		0x0013, 0x0015, 0x00, 0x18,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN		ULONG_PTR	ZeroBits,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		AllocationType,
	IN		ULONG		Protect);

GENERATE_SYSCALL_WIN7(NtQueryVirtualMemory_Win7,			0x010B, 0x0020, 0x00, 0x18,
	IN		HANDLE			ProcessHandle,
	IN		PVOID			BaseAddress OPTIONAL,
	IN		MEMINFOCLASS	MemoryInformationClass,
	OUT		PVOID			MemoryInformation,
	IN		SIZE_T			MemoryInformationLength,
	OUT		PSIZE_T			ReturnLength OPTIONAL);

GENERATE_SYSCALL_WIN7(NtFreeVirtualMemory_Win7,			0x0083, 0x001B, 0x00, 0x10,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		FreeType);

GENERATE_SYSCALL_WIN7(NtOpenKeyEx_Win7,					0x00B7, 0x00F2, 0x00, 0x10,
	OUT		PHANDLE						KeyHandle,
	IN		ACCESS_MASK					DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes,
	IN		ULONG						OpenOptions);

GENERATE_SYSCALL_WIN7(NtQueryObject_Win7,				0x00F8, 0x000D, 0x00, 0x14,
	IN		HANDLE						ObjectHandle,
	IN		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
	OUT		PVOID						ObjectInformation,
	IN		ULONG						Length,
	OUT		PULONG						ReturnLength OPTIONAL);

GENERATE_SYSCALL_WIN7(NtOpenFile_Win7,					0x00B3, 0x0030, 0x00, 0x18,
	OUT		PHANDLE				FileHandle,
	IN		ACCESS_MASK			DesiredAccess,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		ULONG				ShareAccess,
	IN		ULONG				OpenOptions);

GENERATE_SYSCALL_WIN7(NtWriteFile_Win7,					0x018C, 0x0005, 0x1A, 0x24,
	IN		HANDLE				FileHandle,
	IN		HANDLE				Event OPTIONAL,
	IN		PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN		PVOID				ApcContext OPTIONAL,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		PVOID				Buffer,
	IN		ULONG				Length,
	IN		PLONGLONG			ByteOffset OPTIONAL,
	IN		PULONG				Key OPTIONAL);

GENERATE_SYSCALL_WIN7(NtRaiseHardError_Win7,				0x0110, 0x0130, 0x00, 0x18,
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response);

GENERATE_SYSCALL_WIN7(NtQueryInformationThread_Win7,		0x00EC, 0x0022, 0x00, 0x14,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

GENERATE_SYSCALL_WIN7(NtSetInformationThread_Win7,		0x014F, 0x000A, 0x00, 0x10,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength);

GENERATE_SYSCALL_WIN7(NtNotifyChangeKey_Win7,			0x00AC, 0x00EB, 0x00, 0x28,
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

GENERATE_SYSCALL_WIN7(NtNotifyChangeMultipleKeys_Win7,	0x00AD, 0x00EC, 0x00, 0x30,
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

GENERATE_SYSCALL_WIN7(NtCreateSection_Win7,					0x0054, 0x0047, 0x00, 0x1C,
	OUT	PHANDLE				SectionHandle,
	IN	ULONG				DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
	IN	PLONGLONG			MaximumSize OPTIONAL,
	IN	ULONG				PageAttributes,
	IN	ULONG				SectionAttributes,
	IN	HANDLE				FileHandle OPTIONAL);

GENERATE_SYSCALL_WIN7(NtQueryInformationProcess_Win7,		0x00EA, 0x0016, 0x00, 0x14,
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

GENERATE_SYSCALL_WIN7(NtAssignProcessToJobObject_Win7,		0x002B, 0x0085, 0x08, 0x08,
	IN	HANDLE				JobHandle,
	IN	HANDLE				ProcessHandle);

GENERATE_SYSCALL_WIN81(NtQuerySystemTime_Win81,			0x0096, 0x0059, 0x04,
	OUT		PLONGLONG	CurrentTime);

GENERATE_SYSCALL_WIN81(NtCreateUserProcess_Win81,			0x0149, 0x00B7, 0x2C,
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

GENERATE_SYSCALL_WIN81(NtProtectVirtualMemory_Win81,		0x00C6, 0x004F, 0x14,
	IN		HANDLE		ProcessHandle,
	IN OUT	PPVOID		BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		NewProtect,
	OUT		PULONG		OldProtect);

GENERATE_SYSCALL_WIN81(NtAllocateVirtualMemory_Win81,		0x019B, 0x0017, 0x18,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN		ULONG_PTR	ZeroBits,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		AllocationType,
	IN		ULONG		Protect);

GENERATE_SYSCALL_WIN81(NtQueryVirtualMemory_Win81,			0x0092, 0x0022, 0x18,
	IN		HANDLE			ProcessHandle,
	IN		PVOID			BaseAddress OPTIONAL,
	IN		MEMINFOCLASS	MemoryInformationClass,
	OUT		PVOID			MemoryInformation,
	IN		SIZE_T			MemoryInformationLength,
	OUT		PSIZE_T			ReturnLength OPTIONAL);

GENERATE_SYSCALL_WIN81(NtFreeVirtualMemory_Win81,			0x011C, 0x001D, 0x10,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		FreeType);

GENERATE_SYSCALL_WIN81(NtOpenKeyEx_Win81,					0x00E6, 0x0107, 0x10,
	OUT		PHANDLE						KeyHandle,
	IN		ACCESS_MASK					DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes,
	IN		ULONG						OpenOptions);

GENERATE_SYSCALL_WIN81(NtQueryObject_Win81,				0x00A5, 0x000F, 0x14,
	IN		HANDLE						ObjectHandle,
	IN		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
	OUT		PVOID						ObjectInformation,
	IN		ULONG						Length,
	OUT		PULONG						ReturnLength OPTIONAL);

GENERATE_SYSCALL_WIN81(NtOpenFile_Win81,					0x00EB, 0x0032, 0x18,
	OUT		PHANDLE				FileHandle,
	IN		ACCESS_MASK			DesiredAccess,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		ULONG				ShareAccess,
	IN		ULONG				OpenOptions);

GENERATE_SYSCALL_WIN81(NtWriteFile_Win81,					0x0006, 0x0007, 0x24,
	IN		HANDLE				FileHandle,
	IN		HANDLE				Event OPTIONAL,
	IN		PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN		PVOID				ApcContext OPTIONAL,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		PVOID				Buffer,
	IN		ULONG				Length,
	IN		PLONGLONG			ByteOffset OPTIONAL,
	IN		PULONG				Key OPTIONAL);

GENERATE_SYSCALL_WIN81(NtRaiseHardError_Win81,				0x008B, 0x0147, 0x18,
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response);

GENERATE_SYSCALL_WIN81(NtQueryInformationThread_Win81,		0x00B1, 0x0024, 0x14,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

GENERATE_SYSCALL_WIN81(NtSetInformationThread_Win81,		0x004B, 0x000C, 0x10,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength);

GENERATE_SYSCALL_WIN81(NtNotifyChangeKey_Win81,			0x00F2, 0x0100, 0x28,
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

GENERATE_SYSCALL_WIN81(NtNotifyChangeMultipleKeys_Win81,	0x00F1, 0x0101, 0x30,
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

GENERATE_SYSCALL_WIN81(NtCreateSection_Win81,					0x0154, 0x0049, 0x1C,
	OUT	PHANDLE				SectionHandle,
	IN	ULONG				DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
	IN	PLONGLONG			MaximumSize OPTIONAL,
	IN	ULONG				PageAttributes,
	IN	ULONG				SectionAttributes,
	IN	HANDLE				FileHandle OPTIONAL);

GENERATE_SYSCALL_WIN81(NtQueryInformationProcess_Win81,		0x00B3, 0x0018, 0x14,
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

GENERATE_SYSCALL_WIN81(NtAssignProcessToJobObject_Win81,		0x0182, 0x008A, 0x08,
	IN	HANDLE				JobHandle,
	IN	HANDLE				ProcessHandle);

#define CALL_SYSCALL(SyscallName, ...) \
do { \
	try { \
		if (KexRtlCurrentProcessBitness() != KexRtlOperatingSystemBitness()) { \
			if (OriginalMajorVersion == 6 && OriginalMinorVersion == 1) return Kex##SyscallName##_Win7_Wow64(__VA_ARGS__); \
			else if (OriginalMajorVersion == 6 && OriginalMinorVersion == 3) return Kex##SyscallName##_Win81_Wow64(__VA_ARGS__); \
		} else { \
			if (OriginalMajorVersion == 6 && OriginalMinorVersion == 1) return Kex##SyscallName##_Win7_Native32(__VA_ARGS__); \
			else if (OriginalMajorVersion == 6 && OriginalMinorVersion == 3) return Kex##SyscallName##_Win81_Native32(__VA_ARGS__); \
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