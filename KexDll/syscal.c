#include "buildcfg.h"
#include "kexdllp.h"

#define CALL_SYSCALL(SyscallName, ...) \
do { \
	try { \
		if (OriginalMajorVersion == 6 && OriginalMinorVersion == 1) return Kex##SyscallName##_Win7(__VA_ARGS__); \
		else if (OriginalMajorVersion == 6 && OriginalMinorVersion == 3) return Kex##SyscallName##_Win81(__VA_ARGS__); \
		else return SyscallName(__VA_ARGS__); \
	} \
	except (GetExceptionCode() == STATUS_ACCESS_VIOLATION) { \
		return STATUS_ACCESS_VIOLATION; \
	} \
} while (0)

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