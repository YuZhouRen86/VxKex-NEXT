#include "buildcfg.h"
#include "kxuserp.h"
#include <SetupAPI.h>
#pragma comment(lib, "setupapi.lib")

#define HID_USAGE_PAGE_DIGITIZER			((USHORT) 0x0D)
#define HID_USAGE_DIGITIZER_PEN				((USHORT) 0x02)
#define HID_USAGE_DIGITIZER_TOUCH_SCREEN	((USHORT) 0x04)
#define HID_USAGE_DIGITIZER_TOUCH_PAD		((USHORT) 0x05)

#define MAX_TOUCH_POINTS 256
#define MAX_PEN_POINTS 256

STATIC BOOLEAN g_MouseInPointerEnabled = FALSE;

STATIC TOUCH_POINT_ENTRY g_TouchPoints[MAX_TOUCH_POINTS];
STATIC CRITICAL_SECTION g_TouchPointsLock;
STATIC PEN_POINT_ENTRY g_PenPoints[MAX_PEN_POINTS];
STATIC CRITICAL_SECTION g_PenPointsLock;
STATIC BOOL g_TouchAndPenPointsInitialized = FALSE;

PWSTR GetDeviceFriendlyName(
	PCWSTR	DevicePath)
{
#pragma warning(push)
#pragma warning(disable:4995)
	PWSTR Result;
	HDEVINFO DeviceInfoSet;
	SP_DEVINFO_DATA DeviceInfoData;
	WCHAR TargetInstanceId[256];
	PCWSTR StartPos;
	PCWSTR EndPos;
	SIZE_T Length;
	DWORD DataType;
	WCHAR RegBuffer[1024];
	DWORD BufferSize;
	BOOL Success;

	Result = NULL;
	DeviceInfoSet = INVALID_HANDLE_VALUE;
	DataType = 0;
	BufferSize = 0;
	ZeroMemory(TargetInstanceId, sizeof(TargetInstanceId));
	ZeroMemory(RegBuffer, sizeof(RegBuffer));

	if (DevicePath == NULL) goto Exit;

	if (wcsncmp(DevicePath, L"\\\\?\\", 4) != 0) goto Exit;

	StartPos = DevicePath + 4;
	EndPos = wcsrchr(StartPos, L'#');
	if (EndPos == NULL) return NULL;
	Length = (SIZE_T)(EndPos - StartPos);
	if (Length >= 256) return NULL;
	StringCchCopyN(TargetInstanceId, ARRAYSIZE(TargetInstanceId), StartPos, Length);
	TargetInstanceId[Length] = L'\0';

	for (Length = 0; TargetInstanceId[Length] != L'\0'; ++Length) {
		if (TargetInstanceId[Length] == L'#')
			TargetInstanceId[Length] = L'\\';
	}

	DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
	if (DeviceInfoSet == INVALID_HANDLE_VALUE) goto Exit;

	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	if (!SetupDiOpenDeviceInfoW(DeviceInfoSet, TargetInstanceId, NULL, 0, &DeviceInfoData)) return NULL;
	Success = SetupDiGetDeviceRegistryPropertyW(
		DeviceInfoSet,
		&DeviceInfoData,
		SPDRP_DEVICEDESC,
		&DataType,
		(PBYTE)RegBuffer,
		sizeof(RegBuffer),
		&BufferSize);
	if (Success) {
		SIZE_T ResultLength = wcslen(RegBuffer) + 1;
		Result = (PWSTR)LocalAlloc(LMEM_FIXED, ResultLength * sizeof(WCHAR));
		if (Result) StringCchCopy(Result, ResultLength, RegBuffer);
	}
Exit:
	if (DeviceInfoSet != INVALID_HANDLE_VALUE) SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	return Result;
#pragma warning(pop)
}


BOOL WINAPI GetPointerDevices(
	IN OUT	PUINT32					DeviceCount,
	OUT		PPOINTER_DEVICE_INFO	PointerDevices)
{
	UINT RawDeviceCount;
	RAWINPUTDEVICELIST *RawDevList;
	POINTER_DEVICE_INFO *TempList;
	UINT Index;
	UINT ValidCount;
	UINT DevInfoSize;
	RID_DEVICE_INFO DevInfo;
	HANDLE hDevice;
	USHORT Usage;
	UINT NameSize;
	WCHAR *DeviceName;
	WCHAR *FriendlyName;
	UINT CopyCount;

	// Validate parameter
	if (DeviceCount == NULL)
		return FALSE;

	// First call: get required buffer size for raw input device list
	RawDeviceCount = 0;
	if (GetRawInputDeviceList(NULL, &RawDeviceCount, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) {
		*DeviceCount = 0;
		return FALSE;
	}

	if (RawDeviceCount == 0) {
		*DeviceCount = 0;
		return TRUE;
	}

	// Allocate memory for raw device list
	RawDevList = (PRAWINPUTDEVICELIST)LocalAlloc(LMEM_FIXED, RawDeviceCount * sizeof(RAWINPUTDEVICELIST));
	if (RawDevList == NULL) {
		*DeviceCount = 0;
		return FALSE;
	}

	if (GetRawInputDeviceList(RawDevList, &RawDeviceCount, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) {
		LocalFree((HLOCAL)RawDevList);
		*DeviceCount = 0;
		return FALSE;
	}

	// Temporary storage for valid pointer devices (max RawDeviceCount)
	TempList = (PPOINTER_DEVICE_INFO)LocalAlloc(LMEM_FIXED, RawDeviceCount * sizeof(POINTER_DEVICE_INFO));
	if (TempList == NULL) {
		LocalFree((HLOCAL)RawDevList);
		*DeviceCount = 0;
		return FALSE;
	}

	ValidCount = 0;
	DevInfo.cbSize = sizeof(RID_DEVICE_INFO);

	// Iterate all raw input devices
	for (Index = 0; Index < RawDeviceCount; ++Index) {
		hDevice = RawDevList[Index].hDevice;
		// We only care about HID devices (mouse is RIM_TYPEMOUSE and is excluded)
		if (RawDevList[Index].dwType != RIM_TYPEHID)
			continue;

		// Retrieve HID device information
		DevInfoSize = sizeof(RID_DEVICE_INFO);
		if (GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &DevInfo, &DevInfoSize) == (UINT)-1)
			continue;

		// Filter for Digitizer usage page (pointer devices)
		if (DevInfo.hid.usUsagePage != HID_USAGE_PAGE_DIGITIZER)
			continue;

		Usage = DevInfo.hid.usUsage;
		if (Usage != HID_USAGE_DIGITIZER_PEN &&
			Usage != HID_USAGE_DIGITIZER_TOUCH_SCREEN &&
			Usage != HID_USAGE_DIGITIZER_TOUCH_PAD) {
				continue;
		}

		// Fill POINTER_DEVICE_INFO structure (field names exactly as SDK)
		ZeroMemory(&TempList[ValidCount], sizeof(POINTER_DEVICE_INFO));
		TempList[ValidCount].device = hDevice;
		TempList[ValidCount].displayOrientation = DISPLAYCONFIG_ROTATION_IDENTITY;
		TempList[ValidCount].monitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
		TempList[ValidCount].startingCursorId = 1;
		TempList[ValidCount].maxActiveContacts = 1;
		if (Usage == HID_USAGE_DIGITIZER_TOUCH_SCREEN) {
			INT SystemMaximumTouches = GetSystemMetrics(SM_MAXIMUMTOUCHES);
			if (SystemMaximumTouches > 0) TempList[ValidCount].maxActiveContacts = SystemMaximumTouches;
		}

		// Determine device type
		if (Usage == HID_USAGE_DIGITIZER_PEN) {
			TempList[ValidCount].pointerDeviceType = POINTER_DEVICE_TYPE_INTEGRATED_PEN;
		} else if (Usage == HID_USAGE_DIGITIZER_TOUCH_SCREEN) {
			TempList[ValidCount].pointerDeviceType = POINTER_DEVICE_TYPE_TOUCH;
		} else if (Usage == HID_USAGE_DIGITIZER_TOUCH_PAD) {
			TempList[ValidCount].pointerDeviceType = POINTER_DEVICE_TYPE_TOUCH_PAD;
		} else {
			TempList[ValidCount].pointerDeviceType = POINTER_DEVICE_TYPE_TOUCH; // fallback
		}

		// Retrieve device path to query friendly name via SetupAPI
		NameSize = 0;
		if (GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, NULL, &NameSize) != (UINT)-1 && NameSize > 0) {
			DeviceName = (PWSTR)LocalAlloc(LMEM_FIXED, (NameSize + 1) * sizeof(WCHAR));
			if (DeviceName) {
				if (GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, DeviceName, &NameSize) != (UINT)-1) {
					FriendlyName = GetDeviceFriendlyName(DeviceName);
					if (FriendlyName) {
						StringCchCopy(TempList[ValidCount].productString, POINTER_DEVICE_PRODUCT_STRING_MAX, FriendlyName);
						TempList[ValidCount].productString[POINTER_DEVICE_PRODUCT_STRING_MAX - 1] = L'\0';
						LocalFree((HLOCAL)FriendlyName);
					} else {
						StringCchCopy(TempList[ValidCount].productString, POINTER_DEVICE_PRODUCT_STRING_MAX, L"Unknown Pointer Device");
					}
				} else {
					StringCchCopy(TempList[ValidCount].productString, POINTER_DEVICE_PRODUCT_STRING_MAX, L"Unknown Pointer Device");
				}
				LocalFree((HLOCAL)DeviceName);
			} else {
				StringCchCopy(TempList[ValidCount].productString, POINTER_DEVICE_PRODUCT_STRING_MAX, L"Unknown Pointer Device");
			}
		} else {
			StringCchCopy(TempList[ValidCount].productString, POINTER_DEVICE_PRODUCT_STRING_MAX, L"Unknown Pointer Device");
		}

		ValidCount++;
	}

	LocalFree((HLOCAL)RawDevList);

	// Return results
	if (PointerDevices == NULL) {
		// Caller only requested count
		*DeviceCount = ValidCount;
		LocalFree((HLOCAL)TempList);
		return TRUE;
	}

	// Copy as many as caller's buffer can hold, update count with actual number
	CopyCount = *DeviceCount;
	if (CopyCount > ValidCount)
		CopyCount = ValidCount;

	if (CopyCount > 0) {
		CopyMemory(PointerDevices, TempList, CopyCount * sizeof(POINTER_DEVICE_INFO));
	}

	*DeviceCount = ValidCount;   // Report total number of devices found

	LocalFree((HLOCAL)TempList);
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetPointerType(
	IN	DWORD				PointerId,
	OUT	POINTER_INPUT_TYPE	*PointerType)
{
	// For some reason we have to set this, and all other pointer type requests,
	// to touchpad pointer type or otherwise Unity doesn't work.
	if (PointerId == 0)	*PointerType = PT_TOUCHPAD;
	else *PointerType = PT_TOUCH;
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetPointerInfo(
	IN	DWORD			PointerId,
	OUT	POINTER_INFO	*PointerInfo)
{
	if (PointerId == 0) {
		PointerInfo->pointerType = PT_TOUCHPAD;
		PointerInfo->pointerId = PointerId;
		PointerInfo->frameId = 0;
		PointerInfo->pointerFlags = POINTER_FLAG_NONE;
		PointerInfo->sourceDevice = NULL;
		PointerInfo->hwndTarget = NULL;
		GetCursorPos(&PointerInfo->ptPixelLocation);
		GetCursorPos(&PointerInfo->ptHimetricLocation);
		GetCursorPos(&PointerInfo->ptPixelLocationRaw);
		GetCursorPos(&PointerInfo->ptHimetricLocationRaw);
		PointerInfo->dwTime = 0;
		PointerInfo->historyCount = 1;
		PointerInfo->InputData = 0;
		PointerInfo->dwKeyStates = 0;
		PointerInfo->PerformanceCount = 0;
		PointerInfo->ButtonChangeType = POINTER_CHANGE_NONE;
	} else {
		INT Index;
		PVOID Entry = NULL;
		BOOL IsPenPoint = FALSE;
		EnterCriticalSection(&g_TouchPointsLock);
		EnterCriticalSection(&g_PenPointsLock);
		for (Index = 0; Index < MAX_TOUCH_POINTS; Index++) {
			if (g_TouchPoints[Index].Valid && g_TouchPoints[Index].PointerId == PointerId) {
				Entry = &g_TouchPoints[Index];
				break;
			}
		}
		for (Index = 0; Index < MAX_PEN_POINTS; Index++) {
			if (g_PenPoints[Index].Valid && g_PenPoints[Index].PointerId == PointerId) {
				Entry = &g_PenPoints[Index];
				IsPenPoint = TRUE;
				break;
			}
		}
		if (!Entry) {
			LeaveCriticalSection(&g_TouchPointsLock);
			LeaveCriticalSection(&g_PenPointsLock);
			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}
		if (IsPenPoint) {
			PPEN_POINT_ENTRY PenPointEntry = (PPEN_POINT_ENTRY)Entry;
			*PointerInfo = PenPointEntry->PenInfo.pointerInfo;
		} else {
			PTOUCH_POINT_ENTRY TouchPointEntry = (PTOUCH_POINT_ENTRY)Entry;
			*PointerInfo = TouchPointEntry->TouchInfo.pointerInfo;
		}
		LeaveCriticalSection(&g_TouchPointsLock);
		LeaveCriticalSection(&g_PenPointsLock);
		return TRUE;
	}
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetPointerTouchInfo(
	IN	DWORD				PointerId,
	OUT	PPOINTER_TOUCH_INFO	TouchInfo)
{
	INT Index;
	PTOUCH_POINT_ENTRY Entry;

	if (PointerId == 0) {
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	EnterCriticalSection(&g_TouchPointsLock);
	Entry = NULL;
	for (Index = 0; Index < MAX_TOUCH_POINTS; Index++) {
		if (g_TouchPoints[Index].Valid && g_TouchPoints[Index].PointerId == PointerId) {
			Entry = &g_TouchPoints[Index];
			break;
		}
	}
	if (!Entry) {
		LeaveCriticalSection(&g_TouchPointsLock);
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	*TouchInfo = Entry->TouchInfo;
	LeaveCriticalSection(&g_TouchPointsLock);
	return TRUE;
}

//
// Returns an array of touch information for all active touch points.
// The caller passes PointerId = 0 to indicate "all touches" (as
// documented for this API). We return the full frame of active touches.
//

KXUSERAPI BOOL WINAPI GetPointerFrameTouchInfo(
	IN		DWORD	PointerId,
	IN OUT	LPDWORD	PointerCount,
	OUT		LPVOID	TouchInfo)
{
	INT Index;
	DWORD Count = 0;
	DWORD Idx = 0;
	PPOINTER_TOUCH_INFO InfoArray = (PPOINTER_TOUCH_INFO)TouchInfo;

	// This implementation only supports retrieving the entire frame;
	// passing a non-zero PointerId is not supported by this simplified hook.
	if (PointerId != 0) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	EnterCriticalSection(&g_TouchPointsLock);

	// Count active touch points
	for (Index = 0; Index < MAX_TOUCH_POINTS; Index++) {
		if (g_TouchPoints[Index].Active) Count++;
	}

	// If the supplied buffer is too small, return the required count
	if (*PointerCount < Count) {
		*PointerCount = Count;
		LeaveCriticalSection(&g_TouchPointsLock);
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	*PointerCount = Count;
	for (Index = 0; Index < MAX_TOUCH_POINTS; Index++) {
		if (g_TouchPoints[Index].Active) {
			InfoArray[Idx++] = g_TouchPoints[Index].TouchInfo;
		}
	}

	LeaveCriticalSection(&g_TouchPointsLock);
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetPointerFrameTouchInfoHistory(
	IN		DWORD	PointerId,
	IN OUT	DWORD	EntriesCount,
	IN OUT	LPDWORD PointerCount,
	OUT		LPVOID	TouchInfo)
{
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetPointerPenInfo(
	IN	DWORD				PointerId,
	OUT	PPOINTER_PEN_INFO	PenInfo)
{
	INT Index;
	PPEN_POINT_ENTRY Entry;

	if (PointerId == 0) {
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	EnterCriticalSection(&g_PenPointsLock);
	Entry = NULL;
	for (Index = 0; Index < MAX_PEN_POINTS; Index++) {
		if (g_PenPoints[Index].Valid && g_PenPoints[Index].PointerId == PointerId) {
			Entry = &g_PenPoints[Index];
			break;
		}
	}
	if (!Entry) {
		LeaveCriticalSection(&g_PenPointsLock);
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	*PenInfo = Entry->PenInfo;
	LeaveCriticalSection(&g_PenPointsLock);
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetPointerPenInfoHistory(
	IN		DWORD	PointerId,
	IN OUT	LPDWORD	EntriesCount,
	OUT		LPVOID	PenInfo)
{
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI SkipPointerFrameMessages(
	IN	DWORD	PointerId)
{
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetPointerDeviceRects(
	IN	HANDLE	Device,
	OUT	LPRECT	PointerDeviceRect,
	OUT	LPRECT	DisplayRect)
{
	PointerDeviceRect->top = 0;
	PointerDeviceRect->left = 0;
	PointerDeviceRect->bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	PointerDeviceRect->right = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	DisplayRect->top = 0;
	DisplayRect->left = 0;
	DisplayRect->bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	DisplayRect->right = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	return TRUE;
}

KXUSERAPI BOOL WINAPI RegisterPointerDeviceNotifications(
	IN	HWND	Window,
	IN	BOOL	NotifyRange)
{
	if (!IsWindow(Window)) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetWindowFeedbackSetting(
	IN		HWND			Window,
	IN		FEEDBACK_TYPE	FeedbackType,
	IN		ULONG			Flags,
	IN OUT	PULONG			ConfigurationSize,
	IN		PCVOID			Configuration OPTIONAL)
{
	if (!IsWindow(Window)) {
		RtlSetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
		return FALSE;
	}

	if (FeedbackType == 0 ||
		FeedbackType > 12 ||
		ConfigurationSize == NULL ||
		(Flags & ~GWFS_INCLUDE_ANCESTORS) != 0) {

			RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
			return FALSE;
	}

	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI SetWindowFeedbackSetting(
	IN	HWND			Window,
	IN	FEEDBACK_TYPE	FeedbackType,
	IN	ULONG			Flags,
	IN	ULONG			ConfigurationSize,
	IN	PCVOID			Configuration OPTIONAL)
{
	if (!IsWindow(Window)) {
		RtlSetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
		return FALSE;
	}

	if (FeedbackType == 0 ||
		FeedbackType > 12 ||
		(ConfigurationSize != 0 && Configuration != NULL) ||
		Flags != 0 ||
		(ConfigurationSize != 0 && ConfigurationSize != 4)) {

			RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
			return FALSE;
	}

	// In Windows 8 the feedback settings are stored in a window property
	// (i.e. GetProp/SetProp). The property is an atom named "SysFeedbackSettings".
	// Of course, we won't bother actually doing that, since the window feedback
	// stuff is only relevant for touch screens and pens.

	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI IsMouseInPointerEnabled(
	VOID)
{
	return g_MouseInPointerEnabled;
}

KXUSERAPI BOOL WINAPI EnableMouseInPointer(
	IN	BOOL	Enabled)
{
	STATIC BOOLEAN AlreadyCalled = FALSE;

	// normalize boolean into the range (0,1)
	Enabled = !!Enabled;

	if (AlreadyCalled) {
		if (Enabled == g_MouseInPointerEnabled) {
			return TRUE;
		}

		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	AlreadyCalled = TRUE;
	g_MouseInPointerEnabled = Enabled;

	if (Enabled) {
		// Mouse-in-pointer mode requires window message interception to work.
		EnableWindowMessageInterception();
	}

	return TRUE;
}

VOID InitializeTouchAndPenPoints(
	VOID)
{
	if (!g_TouchAndPenPointsInitialized) {
		InitializeCriticalSection(&g_TouchPointsLock);
		InitializeCriticalSection(&g_PenPointsLock);
		g_TouchAndPenPointsInitialized = TRUE;
	}
}

VOID CleanupTouchAndPenPoints(
	VOID)
{
	if (g_TouchAndPenPointsInitialized) {
		DeleteCriticalSection(&g_TouchPointsLock);
		DeleteCriticalSection(&g_PenPointsLock);
		g_TouchAndPenPointsInitialized = FALSE;
	}
}

PTOUCH_POINT_ENTRY FindOrCreateTouchPoint(
	DWORD	PointerId)
{
	INT Index;
	EnterCriticalSection(&g_TouchPointsLock);
	for (Index = 0; Index < MAX_TOUCH_POINTS; Index++) {
		if (g_TouchPoints[Index].Active && g_TouchPoints[Index].PointerId == PointerId) {
			LeaveCriticalSection(&g_TouchPointsLock);
			return &g_TouchPoints[Index];
		}
	}
	for (Index = 0; Index < MAX_TOUCH_POINTS; Index++) {
		if (!g_TouchPoints[Index].Active) {
			RtlZeroMemory(&g_TouchPoints[Index], sizeof(TOUCH_POINT_ENTRY));
			g_TouchPoints[Index].PointerId = PointerId;
			g_TouchPoints[Index].Active = TRUE;
			g_TouchPoints[Index].Valid = TRUE;
			LeaveCriticalSection(&g_TouchPointsLock);
			return &g_TouchPoints[Index];
		}
	}
	LeaveCriticalSection(&g_TouchPointsLock);
	return NULL;
}

VOID DeactivateTouchPoint(
	DWORD	PointerId)
{
	INT Index;
	EnterCriticalSection(&g_TouchPointsLock);
	for (Index = 0; Index < MAX_TOUCH_POINTS; Index++) {
		if (g_TouchPoints[Index].Valid && g_TouchPoints[Index].Active && g_TouchPoints[Index].PointerId == PointerId) {
			g_TouchPoints[Index].TouchInfo.pointerInfo.pointerFlags |= POINTER_FLAG_UP;
			g_TouchPoints[Index].Active = FALSE;
			break;
		}
	}
	LeaveCriticalSection(&g_TouchPointsLock);
}

VOID RemoveTouchPoint(
	DWORD	PointerId)
{
	INT Index;
	EnterCriticalSection(&g_TouchPointsLock);
	for (Index = 0; Index < MAX_TOUCH_POINTS; Index++) {
		if (g_TouchPoints[Index].Valid && g_TouchPoints[Index].PointerId == PointerId) {
			g_TouchPoints[Index].Valid = FALSE;
			break;
		}
	}
	LeaveCriticalSection(&g_TouchPointsLock);
}

PPEN_POINT_ENTRY FindOrCreatePenPoint(
	DWORD	PointerId)
{
	INT Index;
	EnterCriticalSection(&g_PenPointsLock);
	for (Index = 0; Index < MAX_PEN_POINTS; Index++) {
		if (g_PenPoints[Index].Active && g_PenPoints[Index].PointerId == PointerId) {
			LeaveCriticalSection(&g_PenPointsLock);
			return &g_PenPoints[Index];
		}
	}
	for (Index = 0; Index < MAX_PEN_POINTS; Index++) {
		if (!g_PenPoints[Index].Active) {
			RtlZeroMemory(&g_PenPoints[Index], sizeof(PEN_POINT_ENTRY));
			g_PenPoints[Index].PointerId = PointerId;
			g_PenPoints[Index].Active = TRUE;
			g_PenPoints[Index].Valid = TRUE;
			LeaveCriticalSection(&g_PenPointsLock);
			return &g_PenPoints[Index];
		}
	}
	LeaveCriticalSection(&g_PenPointsLock);
	return NULL;
}

VOID DeactivatePenPoint(
	DWORD	PointerId)
{
	INT Index;
	EnterCriticalSection(&g_PenPointsLock);
	for (Index = 0; Index < MAX_PEN_POINTS; Index++) {
		if (g_PenPoints[Index].Valid && g_PenPoints[Index].Active && g_PenPoints[Index].PointerId == PointerId) {
			g_PenPoints[Index].PenInfo.pointerInfo.pointerFlags |= POINTER_FLAG_UP;
			g_PenPoints[Index].Active = FALSE;
			break;
		}
	}
	LeaveCriticalSection(&g_PenPointsLock);
}

VOID RemovePenPoint(
	DWORD	PointerId)
{
	INT Index;
	EnterCriticalSection(&g_PenPointsLock);
	for (Index = 0; Index < MAX_PEN_POINTS; Index++) {
		if (g_PenPoints[Index].Valid && g_PenPoints[Index].PointerId == PointerId) {
			g_PenPoints[Index].Valid = FALSE;
			break;
		}
	}
	LeaveCriticalSection(&g_PenPointsLock);
}

PVOID FindOrCreateTouchOrPenPoint(
	DWORD	PointerId,
	BOOL	IsPenMessage)
{
	if (IsPenMessage) return (PVOID)FindOrCreatePenPoint(PointerId);
	else return (PVOID)FindOrCreateTouchPoint(PointerId);
}

VOID DeactivateTouchOrPenPoint(
	DWORD	PointerId,
	BOOL	IsPenMessage)
{
	if (IsPenMessage) DeactivatePenPoint(PointerId);
	else DeactivateTouchPoint(PointerId);
}

VOID RemoveTouchOrPenPoint(
	DWORD	PointerId,
	BOOL	IsPenMessage)
{
	if (IsPenMessage) RemovePenPoint(PointerId);
	else RemoveTouchPoint(PointerId);
}