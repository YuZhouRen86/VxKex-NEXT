///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kxuserp.h
//
// Abstract:
//
//     Private header file for KxUser.
//
// Author:
//
//     vxiiduu (10-Feb-2022)
//
// Revision History:
//
//     vxiiduu              10-Feb-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>
#include <KexDll.h>
#include <KxUser.h>

EXTERN PKEX_PROCESS_DATA KexData;

typedef struct _FNDWORDMSG {
	PVOID		pwnd;
	UINT		msg;
	WPARAM		wParam;
	LPARAM		lParam;
	ULONG_PTR	xParam;
	PVOID		xpfnProc;
} TYPEDEF_TYPE_NAME(FNDWORDMSG);

typedef struct _TOUCH_POINT_ENTRY {
	DWORD				PointerId;
	BOOL				Valid;
	BOOL				Active;
	POINTER_TOUCH_INFO	TouchInfo;
} TYPEDEF_TYPE_NAME(TOUCH_POINT_ENTRY);

typedef struct _PEN_POINT_ENTRY {
	DWORD				PointerId;
	BOOL				Valid;
	BOOL				Active;
	POINTER_PEN_INFO	PenInfo;
} TYPEDEF_TYPE_NAME(PEN_POINT_ENTRY);

//
// Tri-state registration status of a window for touch input. This is stored in
// the "VxKex_NEXT_Touch_Window_State" window property.
//
// The application-registered state must be kept separate from the VxKex-
// registered state: when the application registers the window itself, its own
// window procedure is expected to handle WM_TOUCH and our translation layer must
// stay out of the way. When VxKex registered it on the application's behalf,
// there is nobody else to consume WM_TOUCH, so the translation layer must run.
//
// Mixing the two up is catastrophic in one direction only: if VxKex's own
// registration were recorded as "application registered", ProcessTouchMessage
// would skip every WM_TOUCH it was supposed to translate and the entire touch
// pipeline would silently do nothing.
//
typedef enum _KXUSER_TOUCH_REGISTRATION_STATE {
	TouchRegistrationNone = 0,
	TouchRegistrationByApplication = 1,
	TouchRegistrationByVxKex = 2
} KXUSER_TOUCH_REGISTRATION_STATE;

//
// loadsyslib.c
//

HMODULE LoadSystemLibrary(
	LPCWSTR	FileName);

//
// pointer.c
//

KXUSERAPI BOOL WINAPI IsMouseInPointerEnabled(
	VOID);

//
// winmsg.c
//

VOID InitializeTouchAndPenPoints(
	VOID);

VOID CleanupTouchAndPenPoints(
	VOID);

PTOUCH_POINT_ENTRY FindOrCreateTouchPoint(
	DWORD	PointerId);

VOID DeactivateTouchPoint(
	DWORD	PointerId);

//
// Releases a cached touch point, writing the supplied (already rebuilt) score
// back first. DeactivateTouchPoint on its own only ORs POINTER_FLAG_UP into the
// cached flags, which leaves the POINTER_FLAG_DOWN bit set from the way down and
// makes the release indistinguishable from a fresh press.
//
VOID ReleaseTouchPoint(
	DWORD					PointerId,
	PCPOINTER_TOUCH_INFO	TouchInfo);

VOID RemoveTouchPoint(
	DWORD	PointerId);

PPEN_POINT_ENTRY FindOrCreatePenPoint(
	DWORD	PointerId);

VOID DeactivatePenPoint(
	DWORD	PointerId);

VOID ReleasePenPoint(
	DWORD				PointerId,
	PCPOINTER_PEN_INFO	PenInfo);

VOID RemovePenPoint(
	DWORD	PointerId);

PVOID FindOrCreateTouchOrPenPoint(
	DWORD	PointerId,
	BOOL	IsPenMessage);

VOID DeactivateTouchOrPenPoint(
	DWORD	PointerId,
	BOOL	IsPenMessage);

VOID ReleaseTouchOrPenPoint(
	DWORD	PointerId,
	BOOL	IsPenMessage,
	PVOID	PointInfo);

VOID RemoveTouchOrPenPoint(
	DWORD	PointerId,
	BOOL	IsPenMessage);

NTSTATUS EnableWindowMessageInterception(
	VOID);

//
// syscal.c
//

#if defined(KEX_ARCH_X64)

#define KXUSER_DECLARE_SYSCALL(SyscallName, ...) \
KXUSERAPI NTSTATUS NTAPI KxUser##SyscallName##_Win7(__VA_ARGS__); \
KXUSERAPI NTSTATUS NTAPI KxUser##SyscallName(__VA_ARGS__);

#else

#define KXUSER_DECLARE_SYSCALL(SyscallName, ...) \
KXUSERAPI NTSTATUS NTAPI KxUser##SyscallName##_Win7_Native32(__VA_ARGS__); \
KXUSERAPI NTSTATUS NTAPI KxUser##SyscallName##_Win7_Wow64(__VA_ARGS__); \
KXUSERAPI NTSTATUS NTAPI KxUser##SyscallName(__VA_ARGS__);

#endif

KXUSER_DECLARE_SYSCALL(NtUserGetTouchInputInfo,
	HTOUCHINPUT	TouchInput,
	UINT		Inputs,
	PTOUCHINPUT	InputsPtr,
	int			Size);