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

GENERATE_SYSCALL KxUserNtUserGetTouchInputInfo_Win7,						12C3h

_TEXT ENDS

ENDIF
END