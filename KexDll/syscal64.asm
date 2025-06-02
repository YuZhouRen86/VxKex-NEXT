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

GENERATE_SYSCALL KexNtQuerySystemTime_Win7,						0057h
GENERATE_SYSCALL KexNtCreateUserProcess_Win7,					00AAh
GENERATE_SYSCALL KexNtProtectVirtualMemory_Win7,				004Dh
GENERATE_SYSCALL KexNtAllocateVirtualMemory_Win7,				0015h
GENERATE_SYSCALL KexNtQueryVirtualMemory_Win7,					0020h
GENERATE_SYSCALL KexNtFreeVirtualMemory_Win7,					001Bh
GENERATE_SYSCALL KexNtQueryObject_Win7,							000Dh
GENERATE_SYSCALL KexNtOpenFile_Win7,							0030h
GENERATE_SYSCALL KexNtWriteFile_Win7,							0005h
GENERATE_SYSCALL KexNtRaiseHardError_Win7,						0130h
GENERATE_SYSCALL KexNtQueryInformationThread_Win7,				0022h
GENERATE_SYSCALL KexNtSetInformationThread_Win7,				000Ah
GENERATE_SYSCALL KexNtNotifyChangeKey_Win7,						00EBh
GENERATE_SYSCALL KexNtNotifyChangeMultipleKeys_Win7,			00ECh
GENERATE_SYSCALL KexNtCreateSection_Win7,						0047h
GENERATE_SYSCALL KexNtQueryInformationProcess_Win7,				0016h
GENERATE_SYSCALL KexNtAssignProcessToJobObject_Win7,			0085h

GENERATE_SYSCALL KexNtQuerySystemTime_Win81,					0059h
GENERATE_SYSCALL KexNtCreateUserProcess_Win81,					00B7h
GENERATE_SYSCALL KexNtProtectVirtualMemory_Win81,				004Fh
GENERATE_SYSCALL KexNtAllocateVirtualMemory_Win81,				0017h
GENERATE_SYSCALL KexNtQueryVirtualMemory_Win81,					0022h
GENERATE_SYSCALL KexNtFreeVirtualMemory_Win81,					001Dh
GENERATE_SYSCALL KexNtQueryObject_Win81,						000Fh
GENERATE_SYSCALL KexNtOpenFile_Win81,							0032h
GENERATE_SYSCALL KexNtWriteFile_Win81,							0007h
GENERATE_SYSCALL KexNtRaiseHardError_Win81,						0147h
GENERATE_SYSCALL KexNtQueryInformationThread_Win81,				0024h
GENERATE_SYSCALL KexNtSetInformationThread_Win81,				000Ch
GENERATE_SYSCALL KexNtNotifyChangeKey_Win81,					0100h
GENERATE_SYSCALL KexNtNotifyChangeMultipleKeys_Win81,			0101h
GENERATE_SYSCALL KexNtCreateSection_Win81,						0049h
GENERATE_SYSCALL KexNtQueryInformationProcess_Win81,			0018h
GENERATE_SYSCALL KexNtAssignProcessToJobObject_Win81,			008Ah

_TEXT ENDS

ENDIF
END