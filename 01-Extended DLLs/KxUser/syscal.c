#include "buildcfg.h"
#include "kxuserp.h"

#if defined(KEX_ARCH_X64)

#define CALL_SYSCALL(SyscallName, ...) \
do { \
	try { \
		return KxUser##SyscallName##_Win7(__VA_ARGS__); \
	} except (GetExceptionCode() == STATUS_ACCESS_VIOLATION) { \
		return STATUS_ACCESS_VIOLATION; \
	} \
} while (0)

#else

#define KEXNTSYSCALLAPI __declspec(naked)

#define GENERATE_SYSCALL_WIN7(SyscallName, SyscallNumber32, SyscallNumber64, EcxValue, Retn, ...) \
KEXNTSYSCALLAPI NTSTATUS NTAPI KxUser##SyscallName##_Native32(__VA_ARGS__) { asm { \
	asm mov eax, SyscallNumber32 \
	asm call [edx] /* Native 32 bit call */ \
	asm ret Retn \
}} \
KEXNTSYSCALLAPI NTSTATUS NTAPI KxUser##SyscallName##_Wow64(__VA_ARGS__) { asm { \
	asm mov eax, SyscallNumber64 \
	asm mov ecx, EcxValue \
	asm lea edx, [esp+4] \
	asm call fs:0xC0 \
	asm add esp, 4 \
	asm ret Retn \
}}

GENERATE_SYSCALL_WIN7(NtUserGetTouchInputInfo_Win7,			0x1290, 0x12C3, 0x00, 0x10,
	HTOUCHINPUT	TouchInput,
	UINT		Inputs,
	PTOUCHINPUT	InputsPtr,
	int			Size);

#define CALL_SYSCALL(SyscallName, ...) \
do { \
	try { \
		if (KexRtlCurrentProcessBitness() != KexRtlOperatingSystemBitness()) { \
			return KxUser##SyscallName##_Win7_Wow64(__VA_ARGS__); \
		} else { \
			return KxUser##SyscallName##_Win7_Native32(__VA_ARGS__); \
		} \
		return STATUS_NOT_SUPPORTED; \
	} except (GetExceptionCode() == STATUS_ACCESS_VIOLATION) { \
		return STATUS_ACCESS_VIOLATION; \
	} \
} while (0)

#endif

KXUSERAPI NTSTATUS NTAPI KxUserNtUserGetTouchInputInfo(
	HTOUCHINPUT	TouchInput,
	UINT		Inputs,
	PTOUCHINPUT	InputsPtr,
	int			Size)
{
	CALL_SYSCALL(NtUserGetTouchInputInfo, TouchInput, Inputs, InputsPtr, Size);
}