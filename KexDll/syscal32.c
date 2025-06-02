///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     syscal32.c
//
// Abstract:
//
//     32-bit Unhooked System Call Stubs (native & WOW64).
//     TLDR: Tons of macros
//
// Author:
//
//     vxiiduu (23-Oct-2022)
//
// Environment:
//
//   Early process creation ONLY. For reasons of simplicity these functions
//   are not thread safe at all.
//
// Revision History:
//
//     vxiiduu              23-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

#ifdef KEX_ARCH_X86

#define KEXNTSYSCALLAPI __declspec(naked)
#pragma warning(disable:4414) // shut the fuck up i dont care

#define GENERATE_SYSCALL(SyscallName, SyscallNumber32, SyscallNumber64, EcxValue, Retn, ...) \
KEXNTSYSCALLAPI NTSTATUS NTAPI Kex##SyscallName##(__VA_ARGS__) { asm { \
	asm mov eax, SyscallNumber32 \
	asm mov edx, 0x7FFE0300 \
	asm cmp dword ptr [edx], 0 /* If [0x7ffe0300] is zero, that means we are running as a Wow64 program on a 64 bit OS. */ \
	asm je Kex##SyscallName##_Wow64 \
	asm call [edx] /* Native 32 bit call */ \
	asm ret Retn \
	asm Kex##SyscallName##_Wow64: /* Wow64 call */ \
	asm mov eax, SyscallNumber64 \
	asm mov ecx, EcxValue \
	asm lea edx, [esp+4] \
	asm call fs:0xC0 \
	asm add esp, 4 \
	asm ret Retn \
}}

GENERATE_SYSCALL(NtQuerySystemTime_Win7,			0x0107, 0x0057, 0x18, 0x04,
	OUT		PLONGLONG	CurrentTime);

GENERATE_SYSCALL(NtCreateUserProcess_Win7,			0x005D, 0x00AA, 0x00, 0x2C,
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

GENERATE_SYSCALL(NtProtectVirtualMemory_Win7,		0x00D7, 0x004D, 0x00, 0x14,
	IN		HANDLE		ProcessHandle,
	IN OUT	PPVOID		BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		NewProtect,
	OUT		PULONG		OldProtect);

GENERATE_SYSCALL(NtAllocateVirtualMemory_Win7,		0x0013, 0x0015, 0x00, 0x18,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN		ULONG_PTR	ZeroBits,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		AllocationType,
	IN		ULONG		Protect);

GENERATE_SYSCALL(NtQueryVirtualMemory_Win7,			0x010B, 0x0020, 0x00, 0x18,
	IN		HANDLE			ProcessHandle,
	IN		PVOID			BaseAddress OPTIONAL,
	IN		MEMINFOCLASS	MemoryInformationClass,
	OUT		PVOID			MemoryInformation,
	IN		SIZE_T			MemoryInformationLength,
	OUT		PSIZE_T			ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtFreeVirtualMemory_Win7,			0x0083, 0x001B, 0x00, 0x10,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		FreeType);

GENERATE_SYSCALL(NtOpenKeyEx_Win7,					0x00B7, 0x00F2, 0x00, 0x10,
	OUT		PHANDLE						KeyHandle,
	IN		ACCESS_MASK					DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes,
	IN		ULONG						OpenOptions);

GENERATE_SYSCALL(NtQueryObject_Win7,				0x00F8, 0x000D, 0x00, 0x14,
	IN		HANDLE						ObjectHandle,
	IN		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
	OUT		PVOID						ObjectInformation,
	IN		ULONG						Length,
	OUT		PULONG						ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtOpenFile_Win7,					0x00B3, 0x0030, 0x00, 0x18,
	OUT		PHANDLE				FileHandle,
	IN		ACCESS_MASK			DesiredAccess,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		ULONG				ShareAccess,
	IN		ULONG				OpenOptions);

GENERATE_SYSCALL(NtWriteFile_Win7,					0x018C, 0x0005, 0x1A, 0x24,
	IN		HANDLE				FileHandle,
	IN		HANDLE				Event OPTIONAL,
	IN		PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN		PVOID				ApcContext OPTIONAL,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		PVOID				Buffer,
	IN		ULONG				Length,
	IN		PLONGLONG			ByteOffset OPTIONAL,
	IN		PULONG				Key OPTIONAL);

GENERATE_SYSCALL(NtRaiseHardError_Win7,				0x0110, 0x0130, 0x00, 0x18,
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response);

GENERATE_SYSCALL(NtQueryInformationThread_Win7,		0x00EC, 0x0022, 0x00, 0x14,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtSetInformationThread_Win7,		0x014F, 0x000A, 0x00, 0x10,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength);

GENERATE_SYSCALL(NtNotifyChangeKey_Win7,			0x00AC, 0x00EB, 0x00, 0x28,
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

GENERATE_SYSCALL(NtNotifyChangeMultipleKeys_Win7,	0x00AD, 0x00EC, 0x00, 0x30,
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

GENERATE_SYSCALL(NtCreateSection_Win7,					0x0054, 0x0047, 0x00, 0x1C,
	OUT	PHANDLE				SectionHandle,
	IN	ULONG				DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
	IN	PLONGLONG			MaximumSize OPTIONAL,
	IN	ULONG				PageAttributes,
	IN	ULONG				SectionAttributes,
	IN	HANDLE				FileHandle OPTIONAL);

GENERATE_SYSCALL(NtQueryInformationProcess_Win7,		0x00EA, 0x0016, 0x00, 0x14,
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtAssignProcessToJobObject_Win7,		0x002B, 0x0085, 0x08, 0x08,
	IN	HANDLE				JobHandle,
	IN	HANDLE				ProcessHandle);

GENERATE_SYSCALL(NtQuerySystemTime_Win81,			0x0096, 0x0059, 0x18, 0x04,
	OUT		PLONGLONG	CurrentTime);

GENERATE_SYSCALL(NtCreateUserProcess_Win81,			0x0149, 0x00B7, 0x00, 0x2C,
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

GENERATE_SYSCALL(NtProtectVirtualMemory_Win81,		0x00C6, 0x004F, 0x00, 0x14,
	IN		HANDLE		ProcessHandle,
	IN OUT	PPVOID		BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		NewProtect,
	OUT		PULONG		OldProtect);

GENERATE_SYSCALL(NtAllocateVirtualMemory_Win81,		0x019B, 0x0017, 0x00, 0x18,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN		ULONG_PTR	ZeroBits,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		AllocationType,
	IN		ULONG		Protect);

GENERATE_SYSCALL(NtQueryVirtualMemory_Win81,			0x0092, 0x0022, 0x00, 0x18,
	IN		HANDLE			ProcessHandle,
	IN		PVOID			BaseAddress OPTIONAL,
	IN		MEMINFOCLASS	MemoryInformationClass,
	OUT		PVOID			MemoryInformation,
	IN		SIZE_T			MemoryInformationLength,
	OUT		PSIZE_T			ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtFreeVirtualMemory_Win81,			0x011C, 0x001D, 0x00, 0x10,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		FreeType);

GENERATE_SYSCALL(NtOpenKeyEx_Win81,					0x00E6, 0x0107, 0x00, 0x10,
	OUT		PHANDLE						KeyHandle,
	IN		ACCESS_MASK					DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes,
	IN		ULONG						OpenOptions);

GENERATE_SYSCALL(NtQueryObject_Win81,				0x00A5, 0x000F, 0x00, 0x14,
	IN		HANDLE						ObjectHandle,
	IN		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
	OUT		PVOID						ObjectInformation,
	IN		ULONG						Length,
	OUT		PULONG						ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtOpenFile_Win81,					0x00EB, 0x0032, 0x00, 0x18,
	OUT		PHANDLE				FileHandle,
	IN		ACCESS_MASK			DesiredAccess,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		ULONG				ShareAccess,
	IN		ULONG				OpenOptions);

GENERATE_SYSCALL(NtWriteFile_Win81,					0x0006, 0x0007, 0x1A, 0x24,
	IN		HANDLE				FileHandle,
	IN		HANDLE				Event OPTIONAL,
	IN		PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN		PVOID				ApcContext OPTIONAL,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		PVOID				Buffer,
	IN		ULONG				Length,
	IN		PLONGLONG			ByteOffset OPTIONAL,
	IN		PULONG				Key OPTIONAL);

GENERATE_SYSCALL(NtRaiseHardError_Win81,				0x008B, 0x0147, 0x00, 0x18,
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response);

GENERATE_SYSCALL(NtQueryInformationThread_Win81,		0x00B1, 0x0024, 0x00, 0x14,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtSetInformationThread_Win81,		0x004B, 0x000C, 0x00, 0x10,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength);

GENERATE_SYSCALL(NtNotifyChangeKey_Win81,			0x00F2, 0x0100, 0x00, 0x28,
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

GENERATE_SYSCALL(NtNotifyChangeMultipleKeys_Win81,	0x00F1, 0x0101, 0x00, 0x30,
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

GENERATE_SYSCALL(NtCreateSection_Win81,					0x0154, 0x0049, 0x00, 0x1C,
	OUT	PHANDLE				SectionHandle,
	IN	ULONG				DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
	IN	PLONGLONG			MaximumSize OPTIONAL,
	IN	ULONG				PageAttributes,
	IN	ULONG				SectionAttributes,
	IN	HANDLE				FileHandle OPTIONAL);

GENERATE_SYSCALL(NtQueryInformationProcess_Win81,		0x00B3, 0x0018, 0x00, 0x14,
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

GENERATE_SYSCALL(NtAssignProcessToJobObject_Win81,		0x0182, 0x008A, 0x08, 0x08,
	IN	HANDLE				JobHandle,
	IN	HANDLE				ProcessHandle);

#endif