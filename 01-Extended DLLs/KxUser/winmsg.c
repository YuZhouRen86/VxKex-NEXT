///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     winmsg.c
//
// Abstract:
//
//     Functions which deal with the extension or modification of window
//     messages, window procedures, etc. This code was added to support some
//     Unity games which call EnableMouseInPointer and require WM_POINTERxxx
//     messages in order to register moues clicks.
//
//     Important note: GetMessage returns -1 on error, 0 if it receives WM_QUIT,
//     and any other value if it receives a valid message other than WM_QUIT.
//
//     PeekMessage returns 0 if there are no messages, and any other value if
//     it receives a message.
//
// Author:
//
//     vxiiduu (28-Apr-2026)
//
// Revision History:
//
//     vxiiduu              28-Apr-2026  Initial creation.
//     vxiiduu              29-Apr-2026  Enhance KernelUserDispatchWindowMessageCallback
//                                       so that it does not intercept messages
//                                       destined for system-provided controls.
//     vxiiduu              22-May-2026  Hook DefWindowProc to turn unprocessed
//                                       pointer messages back into mouse messages.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxuserp.h"
#include <WindowsX.h>

#define GET_POINTERID_WPARAM(wParam) ((UINT)(wParam) & 0xFFFF)

STATIC BOOLEAN g_WindowMessageInterceptionEnabled = FALSE;
STATIC PFN_DISPATCH g_pfnDWORD = NULL;

#define VXKEX_DEFAULT_POINTER_FLAGS ( \
	POINTER_FLAG_PRIMARY | \
	POINTER_FLAG_CONFIDENCE)

STATIC WORD PointerFlagsFromKeyStateAndWindowMessage(
	IN	WPARAM	WParam,
	IN	UINT	Message)
{
	USHORT PointerFlags;

	PointerFlags = 0;

	switch (Message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		PointerFlags |= POINTER_FLAG_FIRSTBUTTON;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		PointerFlags |= POINTER_FLAG_SECONDBUTTON;
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		PointerFlags |= POINTER_FLAG_THIRDBUTTON;
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		// This is kind of a hack because we can't tell whether the XBUTTON1
		// or XBUTTON2 was pressed...
		PointerFlags |= POINTER_FLAG_FOURTHBUTTON;
		break;
	default:
		NOTHING;
		break;
	}

	if (WParam & MK_LBUTTON) {
		PointerFlags |= POINTER_FLAG_FIRSTBUTTON;
	}

	if (WParam & MK_RBUTTON) {
		PointerFlags |= POINTER_FLAG_SECONDBUTTON;
	}

	if (WParam & MK_MBUTTON) {
		PointerFlags |= POINTER_FLAG_THIRDBUTTON;
	}

	if (WParam & MK_XBUTTON1) {
		PointerFlags |= POINTER_FLAG_FOURTHBUTTON;
	}

	if (WParam & MK_XBUTTON2) {
		PointerFlags |= POINTER_FLAG_FIFTHBUTTON;
	}

	return PointerFlags;
}

STATIC WPARAM KeyStateFromPointerFlags(
	IN	USHORT	PointerFlags)
{
	WORD KeyState;

	KeyState = 0;

	if (PointerFlags & POINTER_FLAG_FIRSTBUTTON) {
		KeyState |= MK_LBUTTON;
	}

	if (PointerFlags & POINTER_FLAG_SECONDBUTTON) {
		KeyState |= MK_RBUTTON;
	}

	if (PointerFlags & POINTER_FLAG_THIRDBUTTON) {
		KeyState |= MK_MBUTTON;
	}

	if (PointerFlags & POINTER_FLAG_FOURTHBUTTON) {
		KeyState |= MK_XBUTTON1;
	}

	if (PointerFlags & POINTER_FLAG_FIFTHBUTTON) {
		KeyState |= MK_XBUTTON2;
	}

	return KeyState;
}

//
// Certain mouse-based window messages give us client coordinates.
// Pointer-based window messages are supposed to use screen coordinates.
// Helper function to convert client to screen coordinates.
//
STATIC LPARAM MouseToPointerCoordinates(
	IN	HWND	Window,
	IN	LPARAM	Coordinates)
{
	POINT Point;
	BOOL Success;

	ASSERT (IsWindow(Window));

	Point.x = GET_X_LPARAM(Coordinates);
	Point.y = GET_Y_LPARAM(Coordinates);
	Success = ClientToScreen(Window, &Point);
	ASSERT (Success);

	return MAKELPARAM(Point.x, Point.y);
}

//
// Do the inverse of MouseToPointerCoordinates.
//
STATIC LPARAM PointerToMouseCoordinates(
	IN	HWND	Window,
	IN	LPARAM	Coordinates)
{
	POINT Point;
	BOOL Success;

	ASSERT (IsWindow(Window));

	Point.x = GET_X_LPARAM(Coordinates);
	Point.y = GET_Y_LPARAM(Coordinates);
	Success = ScreenToClient(Window, &Point);
	ASSERT (Success);

	return MAKELPARAM(Point.x, Point.y);
}

//
// Converts a WM_TOUCH message into one or more WM_POINTER* messages
// (DOWN, UP, UPDATE) and updates the internal touch point cache.
// Returns TRUE if the original WM_TOUCH message was consumed (set to
// WM_NULL), FALSE on failure.
//
STATIC BOOLEAN ProcessTouchMessage(
	IN OUT	PMSG	Message)
{
	NTSTATUS Status;
	BOOL ShouldCloseTouchInputHandle = FALSE;
	UINT Index;
	HWND Hwnd = Message->hwnd;
	HTOUCHINPUT HTouchInput = (HTOUCHINPUT)Message->lParam;
	UINT CInputs = LOWORD(Message->wParam);
	TOUCHINPUT Inputs[256];   // Maximum 256 simultaneous touch points

	if (CInputs > 256) CInputs = 256;

	// Retrieve touch input data
	Status = KxUserNtUserGetTouchInputInfo(HTouchInput, CInputs, Inputs, sizeof(TOUCHINPUT));
	if (!NT_SUCCESS(Status)) {
		if (Status == STATUS_INVALID_HANDLE) {
			BOOL Result = GetTouchInputInfo(HTouchInput, CInputs, Inputs, sizeof(TOUCHINPUT));
			if (!Result) {
				CloseTouchInputHandle(HTouchInput);
				return FALSE;
			}
			ShouldCloseTouchInputHandle = TRUE;
		} else return FALSE;
	}

	for (Index = 0; Index < CInputs; Index++) {
		TOUCHINPUT *Ti = &Inputs[Index];
		DWORD PointerId = Ti->dwID;   // Touch identifier, usually > 0

		// Convert himetric coordinates to screen pixels.
		// This is a simplified conversion; real applications may need
		// to use PhysicalToLogicalPoint or GetSystemMetrics for proper DPI scaling.
		POINT Pt = {Ti->x / 100, Ti->y / 100};

		UINT MessageType = 0;
		DWORD PointerFlags = VXKEX_DEFAULT_POINTER_FLAGS;

		// Build common POINTER_INFO structure
		POINTER_INFO Info;
		POINTER_TOUCH_INFO TouchInfo;
		POINTER_PEN_INFO PenInfo;
		BOOL IsPenMessage = Ti->dwFlags & TOUCHEVENTF_PEN;

		RtlZeroMemory(&Info, sizeof(Info));
		RtlZeroMemory(&TouchInfo, sizeof(TouchInfo));
		RtlZeroMemory(&PenInfo, sizeof(PenInfo));

		Info.pointerType = (IsPenMessage) ? PT_PEN : PT_TOUCH;
		Info.pointerId = PointerId;
		Info.ptPixelLocation = Pt;
		Info.ptHimetricLocation = Pt;
		Info.ptPixelLocationRaw = Pt;
		Info.ptHimetricLocationRaw = Pt;
		Info.dwTime = Ti->dwTime;
		Info.historyCount = 1;
		Info.InputData = 0;
		Info.dwKeyStates = 0;
		Info.PerformanceCount = 0;
		Info.ButtonChangeType = POINTER_CHANGE_NONE;

		if (IsPenMessage) {
			// Build pen-specific data
			PenInfo.pointerInfo = Info;
			PenInfo.penFlags = 0;
			PenInfo.penMask = 0;
			PenInfo.pressure = 10;   // Arbitrary pressure value
			PenInfo.rotation = 0;
			PenInfo.tiltX = 0;
			PenInfo.tiltY = 0;
		} else {
			// Build touch-specific data
			TouchInfo.pointerInfo = Info;
			TouchInfo.touchFlags = 0;
			TouchInfo.touchMask = 0;
			// Rough contact rectangle (5 pixels around the point)
			TouchInfo.rcContact.left = Pt.x - 5;
			TouchInfo.rcContact.right = Pt.x + 5;
			TouchInfo.rcContact.top = Pt.y - 5;
			TouchInfo.rcContact.bottom = Pt.y + 5;
			TouchInfo.orientation = 0;
			TouchInfo.pressure = 10;   // Arbitrary pressure value
		}

		if (Ti->dwFlags & TOUCHEVENTF_DOWN) {
			PVOID Entry;
			PointerFlags |= POINTER_FLAG_DOWN | POINTER_FLAG_INCONTACT;
			MessageType = WM_POINTERDOWN;

			// Store the new touch point
			Entry = FindOrCreateTouchOrPenPoint(PointerId, IsPenMessage);
			if (Entry) {
				if (IsPenMessage) {
					PPEN_POINT_ENTRY PenPointEntry = (PPEN_POINT_ENTRY)Entry;
					PenInfo.pointerInfo.pointerFlags = PointerFlags;
					PenPointEntry->PenInfo = PenInfo;
				} else {
					PTOUCH_POINT_ENTRY TouchPointEntry = (PTOUCH_POINT_ENTRY)Entry;
					TouchInfo.pointerInfo.pointerFlags = PointerFlags;
					TouchPointEntry->TouchInfo = TouchInfo;
				}
			}
		} else if (Ti->dwFlags & TOUCHEVENTF_UP) {
			MessageType = WM_POINTERUP;

			// Deactivate the point from cache
			DeactivateTouchOrPenPoint(PointerId, IsPenMessage);
		} else if (Ti->dwFlags & TOUCHEVENTF_MOVE) {
			PVOID Entry;
			PointerFlags |= POINTER_FLAG_UPDATE | POINTER_FLAG_INCONTACT;
			MessageType = WM_POINTERUPDATE;

			// Update the cached data
			Entry = FindOrCreateTouchOrPenPoint(PointerId, IsPenMessage);
			if (Entry) {
				if (IsPenMessage) {
					PPEN_POINT_ENTRY PenPointEntry = (PPEN_POINT_ENTRY)Entry;
					PenInfo.pointerInfo.pointerFlags = PointerFlags;
					PenPointEntry->PenInfo = PenInfo;
				} else {
					PTOUCH_POINT_ENTRY TouchPointEntry = (PTOUCH_POINT_ENTRY)Entry;
					TouchInfo.pointerInfo.pointerFlags = PointerFlags;
					TouchPointEntry->TouchInfo = TouchInfo;
				}
			}
		} else {
			// Other flags (like TOUCHEVENTF_NOCOALESCE) are ignored
			continue;
		}

		// Send the synthesized pointer message to the target window
		// We use SendMessage to preserve order; it will not re-enter our hook
		// because the hook is called during message retrieval, not during dispatch.
		SendMessage(Hwnd, MessageType,
			MAKEWPARAM(PointerId, PointerFlags),
			MAKELPARAM(Pt.x, Pt.y));
	}

	// Consume the original WM_TOUCH
	Message->message = WM_NULL;
	Message->wParam = 0;
	Message->lParam = 0;
	if (ShouldCloseTouchInputHandle) CloseTouchInputHandle(HTouchInput);
	return TRUE;
}

//
// This function is responsible for modifying window messages in place, if it
// is necessary, before they are passed to the application. All window messages
// received by VxKex applications should pass through here.
//
// This function is forbidden from reading any fields of the MSG structure
// other than hwnd, wParam, and lParam. This is because any messages sent from
// the kernel (such as scroll messages from touchpad drivers, for example)
// go through KernelUserDispatchWindowMessageCallback, and that function does
// not populate any other members of the MSG structure.
//
// If this function modifies a window message, it returns TRUE.
// If not, it returns FALSE.
//
STATIC BOOLEAN ProcessWindowMessageInPlace(
	IN OUT	PMSG	Message,
	IN		BOOLEAN	Unicode)
{
	ASSERT (Message != NULL);

	if (!(KexData->Flags & KEXDATA_FLAG_CHROMIUM) &&
		Message->message == WM_TOUCH &&
		!GetProp(Message->hwnd, L"VxKex_NEXT_Touch_Window_Registered")) {
		return ProcessTouchMessage(Message);
	}

	if (g_WindowMessageInterceptionEnabled == FALSE) {
		// Window message interception is not enabled. Do nothing.
		return FALSE;
	}

	//
	// If the application has called EnableMouseInPointer(TRUE), then we need to
	// convert mouse-related window messages into the new WM_POINTERxxx messages
	// in order for them to work. New versions of Unity require this for mouse
	// clicks to be registered.
	//
	if (IsMouseInPointerEnabled()) {
		WORD NewPointerFlags;

		NewPointerFlags = VXKEX_DEFAULT_POINTER_FLAGS;

		// Note: All VxKex-generated pointer messages will use a pointer ID of 0
		// to represent the system's mouse.

		switch (Message->message) {
		case WM_MOUSEACTIVATE: {
			UINT_PTR TopLevelParentWindow;
			WORD HitTest;

			// gather information from original message
			TopLevelParentWindow = Message->wParam;
			HitTest = LOWORD(Message->lParam);

			// modify the message
			Message->message = WM_POINTERACTIVATE;
			Message->lParam = TopLevelParentWindow;
			Message->wParam = MAKEWPARAM(0, HitTest);
			break;
							   }
		case WM_CAPTURECHANGED: {
			NewPointerFlags |= PointerFlagsFromKeyStateAndWindowMessage(
				Message->wParam,
				Message->message);

			Message->message = WM_POINTERCAPTURECHANGED;
			Message->wParam = MAKEWPARAM(0, NewPointerFlags);
			// lParam does not need to be changed - it's a HWND
			break;
								}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN: {
			NewPointerFlags |= PointerFlagsFromKeyStateAndWindowMessage(
				Message->wParam,
				Message->message);

			NewPointerFlags |= POINTER_FLAG_DOWN | POINTER_FLAG_INCONTACT;
			Message->message = WM_POINTERDOWN;
			Message->wParam = MAKEWPARAM(0, NewPointerFlags);
			Message->lParam = MouseToPointerCoordinates(Message->hwnd, Message->lParam);
			break;
							 }
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP: {
			NewPointerFlags |= PointerFlagsFromKeyStateAndWindowMessage(
				Message->wParam,
				Message->message);

			NewPointerFlags |= POINTER_FLAG_UP;
			Message->message = WM_POINTERUP;
			Message->wParam = MAKEWPARAM(0, NewPointerFlags);
			Message->lParam = MouseToPointerCoordinates(Message->hwnd, Message->lParam);
			break;
						   }
		case WM_MOUSEMOVE: {
			NewPointerFlags |= PointerFlagsFromKeyStateAndWindowMessage(
				Message->wParam,
				Message->message);

			Message->message = WM_POINTERUPDATE;
			Message->wParam = MAKEWPARAM(0, NewPointerFlags);
			Message->lParam = MouseToPointerCoordinates(Message->hwnd, Message->lParam);
			break;
						   }
		case WM_MOUSEWHEEL: {
			Message->message = WM_POINTERWHEEL;
			Message->wParam = MAKEWPARAM(0, HIWORD(Message->wParam));
			// Note that lParam does not need to be changed because WM_MOUSEWHEEL and
			// WM_POINTERWHEEL both use screen coordinates.
			break;
							}
		case WM_MOUSEHWHEEL: {
			Message->message = WM_POINTERHWHEEL;
			Message->wParam = MAKEWPARAM(0, HIWORD(Message->wParam));
			break;
							 }
		default:
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

// See __fnDWORD (sometimes called __fnEMPTY or something like that) in user32
// See comment above EnableWindowMessageInterception
STATIC NTSTATUS NTAPI KernelUserDispatchWindowMessageCallback(
	IN	PVOID	Context OPTIONAL)
{
	PFNDWORDMSG DwordMsg;
	HWND WindowHandle;
	WNDPROC WindowProc;
	MSG Msg;
	BOOLEAN MessageWasModified;

	ASSERT (g_pfnDWORD != NULL);

	DwordMsg = (PFNDWORDMSG) Context;

	if (DwordMsg == NULL || DwordMsg->pwnd == NULL) {
		//
		// Some messages, such as WM_TIMER, have pwnd set to NULL.
		// We don't care about these messages, and pwnd being NULL is annoying to
		// deal with in our later code so we will just bail out.
		//

		goto CallOriginalProc;
	}

	//
	// We are only provided a PWND. We need to convert this to a HWND.
	//

	WindowHandle = PWND_TO_HWND(DwordMsg->pwnd);
	ASSERT (WindowHandle != NULL);

	if (WindowHandle == NULL) {
		goto CallOriginalProc;
	}

	//
	// Place all the necessary information into a temporary MSG structure
	// so that we can call ProcessWindowMessageInPlace on it.
	//

	RtlZeroMemory(&Msg, sizeof(Msg));

	Msg.hwnd			= WindowHandle;
	Msg.message			= DwordMsg->msg;
	Msg.wParam			= DwordMsg->wParam;
	Msg.lParam			= DwordMsg->lParam;

	//
	// Process the window message.
	//

	MessageWasModified = ProcessWindowMessageInPlace(&Msg, TRUE);

	if (MessageWasModified) {
		//
		// Get the address of the window's window procedure.
		// We want to check which module this window procedure is in.
		// If it's in a Windows DLL (e.g. user32.dll or comctl32.dll), then we
		// do not want to mess with this window message.
		//
		// This check is done after ProcessWindowMessageInPlace because the call
		// to AshModuleIsWindowsModule is fairly expensive, since it involves
		// a loader call.
		//

		WindowProc = (WNDPROC) GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
		ASSERT (WindowProc != NULL);

		if (WindowProc == NULL) {
			goto CallOriginalProc;
		}

		if (AshModuleIsWindowsModule(WindowProc)) {
			//
			// The window procedure for this window is inside a Windows DLL, which
			// probably means that this window is a common control or common dialog.
			// We don't want to interfere with messages for such windows, so we will
			// bail out.
			//

			goto CallOriginalProc;
		}

		//
		// The window message was destined for an application-created window procedure.
		// We will proceed to copy all the necessary data from the MSG structure back
		// into the FNDWORDMSG structure.
		//

		DwordMsg->msg		= Msg.message;
		DwordMsg->wParam	= Msg.wParam;
		DwordMsg->lParam	= Msg.lParam;
	}

CallOriginalProc:
	return g_pfnDWORD(Context);
}

//
// Hooks on GetMessage and PeekMessage work for most messages queued in a
// message queue, but some, such as scroll messages from certain touchpad drivers,
// are sent directly from the kernel and therefore GetMessage/PeekMessage hooks
// do not work for intercepting those.
//
// Such messages pass through a function called fnDWORD in user32, the address of
// which is stored in the Peb->KernelCallbackTable. Overwriting this function
// pointer will allow us to hook the function and run our window message handling
// code.
//
// Window message interception is disabled by default until this function is called.
// This allows us to avoid performance and compatibility issues with hooking all
// window messages. Bugs in KernelUserDispatchWindowMessageCallback can cause
// confusing error messages, and it's best not to risk it for the 99% of
// applications that do not need this compatibility fix.
//
NTSTATUS EnableWindowMessageInterception(
	VOID)
{
	STATIC RTL_SRWLOCK Lock = RTL_SRWLOCK_INIT;
	NTSTATUS Status;
	PFN_DISPATCH *KernelCallbackTable;
	PVOID BaseAddress;
	SIZE_T RegionSize;
	ULONG OldProtect;

	if (g_WindowMessageInterceptionEnabled) {
		return STATUS_ALREADY_INITIALIZED;
	}

	KernelCallbackTable = NtCurrentPeb()->KernelCallbackTable;
	ASSERT (KernelCallbackTable != NULL);

	if (KernelCallbackTable == NULL) {
		KexLogErrorEvent(L"Failed to initialize window message interception because KernelCallbackTable is NULL");
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Get the original fnDWORD pointer
	//

	ASSERT (g_pfnDWORD == NULL);
	g_pfnDWORD = KernelCallbackTable[2];
	ASSERT (g_pfnDWORD != NULL);

	if (g_pfnDWORD == NULL) {
		KexLogErrorEvent(L"Failed to initialize window message interception because the pointer to fnDWORD is NULL");
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Replace it with our hook. The callback table is usually read-only so we
	// need to set the memory protection.
	//

	BaseAddress = &KernelCallbackTable[2];
	RegionSize = sizeof(KernelCallbackTable[2]);

	RtlAcquireSRWLockExclusive(&Lock);

	try {
		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			PAGE_READWRITE,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogErrorEvent(
				L"Failed to initialize window message interception.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);

			return Status;
		}

		KernelCallbackTable[2] = KernelUserDispatchWindowMessageCallback;

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			OldProtect,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));
	} finally {
		RtlReleaseSRWLockExclusive(&Lock);
	}

	KexLogInformationEvent(L"Window message interception enabled");

	g_WindowMessageInterceptionEnabled = TRUE;
	return STATUS_SUCCESS;
}

KXUSERAPI BOOL WINAPI Ext_GetMessageA(
	OUT	PMSG	Message,
	IN	HWND	Window OPTIONAL,
	IN	UINT	MessageFilterMin,
	IN	UINT	MessageFilterMax)
{
	INT ReturnValue;

	ReturnValue = GetMessageA(
		Message,
		Window,
		MessageFilterMin,
		MessageFilterMax);

	if (ReturnValue != -1) {
		ProcessWindowMessageInPlace(Message, FALSE);
	}

	return ReturnValue;
}

KXUSERAPI BOOL WINAPI Ext_GetMessageW(
	OUT	PMSG	Message,
	IN	HWND	Window OPTIONAL,
	IN	UINT	MessageFilterMin,
	IN	UINT	MessageFilterMax)
{
	INT ReturnValue;

	ReturnValue = GetMessageW(
		Message,
		Window,
		MessageFilterMin,
		MessageFilterMax);

	if (ReturnValue != -1) {
		ProcessWindowMessageInPlace(Message, TRUE);
	}

	return ReturnValue;
}

KXUSERAPI BOOL WINAPI Ext_PeekMessageA(
	OUT	PMSG	Message,
	IN	HWND	Window OPTIONAL,
	IN	UINT	MessageFilterMin,
	IN	UINT	MessageFilterMax,
	IN	UINT	MessageRemoveFlags)
{
	INT ReturnValue;

	ReturnValue = PeekMessageA(
		Message,
		Window,
		MessageFilterMin,
		MessageFilterMax,
		MessageRemoveFlags);

	if (ReturnValue != 0) {
		ProcessWindowMessageInPlace(Message, FALSE);
	}

	return ReturnValue;
}

KXUSERAPI BOOL WINAPI Ext_PeekMessageW(
	OUT	PMSG	Message,
	IN	HWND	Window OPTIONAL,
	IN	UINT	MessageFilterMin,
	IN	UINT	MessageFilterMax,
	IN	UINT	MessageRemoveFlags)
{
	INT ReturnValue;

	ReturnValue = PeekMessageW(
		Message,
		Window,
		MessageFilterMin,
		MessageFilterMax,
		MessageRemoveFlags);

	if (ReturnValue != 0) {
		ProcessWindowMessageInPlace(Message, TRUE);
	}

	return ReturnValue;
}

//
// DefWindowProc on Windows 8 or above changes WM_POINTERxxx messages
// back into their equivalent mouse messages.
//
// Applications which call EnableMouseInPointer but then don't handle
// one or more WM_POINTERxxx messages depend on this behavior.
//
// The game "Haste" (Unity) requires this.
//
// https://devblogs.microsoft.com/oldnewthing/20210728-00/?p=105487
//
STATIC LRESULT KxUserDefWindowProcAorW(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam,
	IN	BOOLEAN	Unicode)
{
	MSG Msg;

	if (g_WindowMessageInterceptionEnabled == FALSE) {
		// Window message interception is not enabled. Do nothing.
		goto CallOriginalDefWindowProc;
	}

	KexRtlZeroMemory(&Msg, sizeof(Msg));
	Msg.hwnd = Window;
	Msg.message = Message;
	Msg.wParam = WParam;
	Msg.lParam = LParam;

	// For pointer messages, check if the pointer ID is zero (mouse simulation).
	// If not zero, do not convert; pass to original DefWindowProc.
	switch (Message) {
	case WM_POINTERACTIVATE:
	case WM_POINTERCAPTURECHANGED:
	case WM_POINTERDOWN:
	case WM_POINTERUP:
	case WM_POINTERUPDATE:
	case WM_POINTERWHEEL:
	case WM_POINTERHWHEEL: {
		UINT PointerId = GET_POINTERID_WPARAM(WParam);
		if (PointerId != 0) {
			// This is a touch or pen pointer; do not convert.
			goto CallOriginalDefWindowProc;
		}
		// Otherwise, it's our simulated mouse pointer (ID=0); proceed with conversion.
		break;
						   }
	default:
		// Not a pointer message; no conversion needed.
		goto CallOriginalDefWindowProc;
	}

	switch (Message) {
	case WM_POINTERACTIVATE: {
		Msg.message = WM_MOUSEACTIVATE;
		Msg.wParam = (WPARAM) LParam;
		Msg.lParam = MAKELPARAM(HIWORD(WParam), 0);
		break;
							 }
	case WM_POINTERCAPTURECHANGED: {
		Msg.message = WM_CAPTURECHANGED;
		Msg.wParam = 0;
		// leave lParam unchanged - same in both messages
		break;
								   }
	case WM_POINTERDOWN:
	case WM_POINTERUP: {
		USHORT PointerFlags;

		PointerFlags = HIWORD(WParam);

		Msg.wParam = KeyStateFromPointerFlags(PointerFlags);
		Msg.lParam = PointerToMouseCoordinates(Window, LParam);

		if (PointerFlags & POINTER_FLAG_FIRSTBUTTON) {
			Msg.message = WM_LBUTTONDOWN;
		} else if (PointerFlags & POINTER_FLAG_SECONDBUTTON) {
			Msg.message = WM_RBUTTONDOWN;
		} else if (PointerFlags & POINTER_FLAG_THIRDBUTTON) {
			Msg.message = WM_MBUTTONDOWN;
		} else if ((PointerFlags & (POINTER_FLAG_FOURTHBUTTON | POINTER_FLAG_FIFTHBUTTON))) {
			Msg.message = WM_XBUTTONDOWN;
		} else {
			ASSERT (FALSE);
			Msg.message = WM_LBUTTONDOWN;
		}

		if (Message == WM_POINTERUP) {
			// adding 1 turns a WM_xxBUTTONDOWN into a WM_xxBUTTONUP message
			++Msg.message;
		}

		break;
					   }
	case WM_POINTERUPDATE: {
		USHORT PointerFlags;

		PointerFlags = HIWORD(WParam);

		Msg.message = WM_MOUSEMOVE;
		Msg.wParam = KeyStateFromPointerFlags(PointerFlags);
		Msg.lParam = PointerToMouseCoordinates(Window, LParam);
		break;
						   }
	case WM_POINTERWHEEL: {
		// No ned to change wParam or lParam.
		Msg.message = WM_MOUSEWHEEL;
		break;
						  }
	case WM_POINTERHWHEEL: {
		Msg.message = WM_MOUSEHWHEEL;
		break;
						   }
	default:
		// Should not happen (already filtered above)
		goto CallOriginalDefWindowProc;
	}

	//
	// TODO: Technically, we should be using PostMessage here. However, that
	// causes an infinite loop because we're posting a mouse message, then
	// convert it to a pointer message, it gets passed to DefWindowProc (us)
	// and then convert it to a mouse message over and over again.
	//

	if (Unicode) {
		SendMessageW(Msg.hwnd, Msg.message, Msg.wParam, Msg.lParam);
	} else {
		SendMessageA(Msg.hwnd, Msg.message, Msg.wParam, Msg.lParam);
	}

	return 0;

CallOriginalDefWindowProc:
	if (Unicode) {
		return DefWindowProcW(Window, Message, WParam, LParam);
	} else {
		return DefWindowProcA(Window, Message, WParam, LParam);
	}
}

KXUSERAPI LRESULT WINAPI Ext_DefWindowProcA(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	return KxUserDefWindowProcAorW(Window, Message, WParam, LParam, FALSE);
}

KXUSERAPI LRESULT WINAPI Ext_DefWindowProcW(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	return KxUserDefWindowProcAorW(Window, Message, WParam, LParam, TRUE);
}

KXUSERAPI BOOL WINAPI Ext_RegisterTouchWindow(
	IN	HWND	Window,
	IN	ULONG	Flags)
{
	BOOL Result = RegisterTouchWindow(Window, Flags);
	if (Result) SetProp(Window, L"VxKex_NEXT_Touch_Window_Registered", (HANDLE)TRUE);
	return Result;
}

KXUSERAPI BOOL WINAPI Ext_UnregisterTouchWindow(
	IN	HWND	Window)
{
	BOOL Result = RegisterTouchWindow(Window, 0);
	if (Result) SetProp(Window, L"VxKex_NEXT_Touch_Window_Registered", (HANDLE)FALSE);
	return Result;
}

KXUSERAPI HWND WINAPI Ext_CreateWindowExA(
	IN	DWORD		ExStyle,
	IN	LPCSTR		ClassName OPTIONAL,
	IN	LPCSTR		WindowName OPTIONAL,
	IN	DWORD		Style,
	IN	INT			X,
	IN	INT			Y,
	IN	INT			Width,
	IN	INT			Height,
	IN	HWND		WndParent OPTIONAL,
	IN	HMENU		Menu OPTIONAL,
	IN	HINSTANCE	Instance,
	IN	LPVOID		Param)
{
	HWND Window = CreateWindowExA(ExStyle, ClassName, WindowName, Style, X, Y, Width, Height, WndParent, Menu, Instance, Param);
	if (Window) Ext_UnregisterTouchWindow(Window);
	return Window;
}

KXUSERAPI HWND WINAPI Ext_CreateWindowExW(
	IN	DWORD		ExStyle,
	IN	LPCWSTR		ClassName OPTIONAL,
	IN	LPCWSTR		WindowName OPTIONAL,
	IN	DWORD		Style,
	IN	INT			X,
	IN	INT			Y,
	IN	INT			Width,
	IN	INT			Height,
	IN	HWND		WndParent OPTIONAL,
	IN	HMENU		Menu OPTIONAL,
	IN	HINSTANCE	Instance,
	IN	LPVOID		Param)
{
	HWND Window = CreateWindowExW(ExStyle, ClassName, WindowName, Style, X, Y, Width, Height, WndParent, Menu, Instance, Param);
	if (Window) Ext_UnregisterTouchWindow(Window);
	return Window;
}