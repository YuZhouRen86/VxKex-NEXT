IFDEF RAX

_TEXT SEGMENT

GENERATE_SYSCALL MACRO SyscallName, SyscallNumber64
PUBLIC SyscallName
ALIGN 16
SyscallName PROC
	mov			r10, rcx
	mov			eax, SyscallNumber64
	syscall
	ret
SyscallName ENDP
ENDM

EXTERN SSN_NtQuerySystemTime: DWORD
EXTERN SSN_NtCreateUserProcess: DWORD
EXTERN SSN_NtProtectVirtualMemory: DWORD
EXTERN SSN_NtAllocateVirtualMemory: DWORD
EXTERN SSN_NtQueryVirtualMemory: DWORD
EXTERN SSN_NtFreeVirtualMemory: DWORD
EXTERN SSN_NtQueryObject: DWORD
EXTERN SSN_NtOpenFile: DWORD
EXTERN SSN_NtWriteFile: DWORD
EXTERN SSN_NtRaiseHardError: DWORD
EXTERN SSN_NtQueryInformationThread: DWORD
EXTERN SSN_NtSetInformationThread: DWORD
EXTERN SSN_NtNotifyChangeKey: DWORD
EXTERN SSN_NtNotifyChangeMultipleKeys: DWORD
EXTERN SSN_NtCreateSection: DWORD
EXTERN SSN_NtQueryInformationProcess: DWORD
EXTERN SSN_NtAssignProcessToJobObject: DWORD
EXTERN SSN_NtMapViewOfSection: DWORD

GENERATE_SYSCALL KexNtQuerySystemTime_ASM,							[SSN_NtQuerySystemTime]
GENERATE_SYSCALL KexNtCreateUserProcess_ASM,						[SSN_NtCreateUserProcess]
GENERATE_SYSCALL KexNtProtectVirtualMemory_ASM,						[SSN_NtProtectVirtualMemory]
GENERATE_SYSCALL KexNtAllocateVirtualMemory_ASM,					[SSN_NtAllocateVirtualMemory]
GENERATE_SYSCALL KexNtQueryVirtualMemory_ASM,						[SSN_NtQueryVirtualMemory]
GENERATE_SYSCALL KexNtFreeVirtualMemory_ASM,						[SSN_NtFreeVirtualMemory]
GENERATE_SYSCALL KexNtQueryObject_ASM,								[SSN_NtQueryObject]
GENERATE_SYSCALL KexNtOpenFile_ASM,									[SSN_NtOpenFile]
GENERATE_SYSCALL KexNtWriteFile_ASM,								[SSN_NtWriteFile]
GENERATE_SYSCALL KexNtRaiseHardError_ASM,							[SSN_NtRaiseHardError]
GENERATE_SYSCALL KexNtQueryInformationThread_ASM,					[SSN_NtQueryInformationThread]
GENERATE_SYSCALL KexNtSetInformationThread_ASM,						[SSN_NtSetInformationThread]
GENERATE_SYSCALL KexNtNotifyChangeKey_ASM,							[SSN_NtNotifyChangeKey]
GENERATE_SYSCALL KexNtNotifyChangeMultipleKeys_ASM,					[SSN_NtNotifyChangeMultipleKeys]
GENERATE_SYSCALL KexNtCreateSection_ASM,							[SSN_NtCreateSection]
GENERATE_SYSCALL KexNtQueryInformationProcess_ASM,					[SSN_NtQueryInformationProcess]
GENERATE_SYSCALL KexNtAssignProcessToJobObject_ASM,					[SSN_NtAssignProcessToJobObject]
GENERATE_SYSCALL KexNtMapViewOfSection_ASM,							[SSN_NtMapViewOfSection]

_TEXT ENDS

ENDIF
END