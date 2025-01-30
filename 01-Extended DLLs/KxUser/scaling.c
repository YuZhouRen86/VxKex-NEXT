#include "buildcfg.h"
#include "kxuserp.h"

KXUSERAPI BOOL WINAPI AreDpiAwarenessContextsEqual(
	IN	DPI_AWARENESS_CONTEXT	Value1,
	IN	DPI_AWARENESS_CONTEXT	Value2)
{
	return (Value1 == Value2);
}

KXUSERAPI BOOL WINAPI IsValidDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	Value)
{
	switch (Value) {
	case DPI_AWARENESS_CONTEXT_UNAWARE:
	case DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED:
	case DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2:
		return TRUE;
	default:
		return FALSE;
	}
}

KXUSERAPI DPI_AWARENESS WINAPI GetAwarenessFromDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	Value)
{
	switch (Value) {
	case DPI_AWARENESS_CONTEXT_UNAWARE:
	case DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED:
		return DPI_AWARENESS_UNAWARE;
	case DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
		return DPI_AWARENESS_SYSTEM_AWARE;
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2:
		return DPI_AWARENESS_PER_MONITOR_AWARE;
	default:
		return DPI_AWARENESS_INVALID;
	}
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetThreadDpiAwarenessContext(
	VOID)
{
	if (IsProcessDPIAware()) {
		return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
	} else {
		return DPI_AWARENESS_CONTEXT_UNAWARE;
	}
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	DpiContext)
{
	BOOLEAN OldDpiAwareness;

	OldDpiAwareness = IsProcessDPIAware();

	switch (DpiContext) {
	case DPI_AWARENESS_CONTEXT_UNAWARE:
		NOTHING;
		break;
	case DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
		SetProcessDPIAware();
		break;
	default:
		return 0;
	}

	if (OldDpiAwareness) {
		return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
	} else {
		return DPI_AWARENESS_CONTEXT_UNAWARE;
	}
}

KXUSERAPI BOOL WINAPI SetProcessDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	DpiContext)
{
	switch (DpiContext) {
	case DPI_AWARENESS_CONTEXT_UNAWARE:
		NOTHING;
		break;
	case DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2:
		SetProcessDPIAware();
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetWindowDpiAwarenessContext(
	IN	HWND	Window)
{
	ULONG WindowThreadId;
	ULONG WindowProcessId;

	WindowThreadId = GetWindowThreadProcessId(Window, &WindowProcessId);
	if (!WindowThreadId) {
		return 0;
	}

	if (WindowProcessId == (ULONG) NtCurrentTeb()->ClientId.UniqueProcess) {
		return GetThreadDpiAwarenessContext();
	}

	return DPI_AWARENESS_CONTEXT_UNAWARE;
}

KXUSERAPI BOOL WINAPI GetProcessDpiAwarenessInternal(
	IN	HANDLE					ProcessHandle,
	OUT	PROCESS_DPI_AWARENESS	*DpiAwareness)
{
	if (ProcessHandle == NULL ||
		ProcessHandle == NtCurrentProcess() ||
		GetProcessId(ProcessHandle) == (ULONG) NtCurrentTeb()->ClientId.UniqueProcess) {

		*DpiAwareness = IsProcessDPIAware() ? PROCESS_SYSTEM_DPI_AWARE : PROCESS_DPI_UNAWARE;
	} else {
		*DpiAwareness = PROCESS_DPI_UNAWARE;
	}

	return TRUE;
}

KXUSERAPI HRESULT WINAPI GetProcessDpiAwareness(
	IN	HANDLE					ProcessHandle,
	OUT	PROCESS_DPI_AWARENESS	*DpiAwareness)
{
	BOOLEAN Success;

	Success = GetProcessDpiAwarenessInternal(ProcessHandle, DpiAwareness);

	if (!Success) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

KXUSERAPI BOOL WINAPI SetProcessDpiAwarenessInternal(
	IN	PROCESS_DPI_AWARENESS	DpiAwareness)
{
	if (DpiAwareness >= PROCESS_MAX_DPI_AWARENESS) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (DpiAwareness != PROCESS_DPI_UNAWARE) {
		// On Windows 7, SetProcessDPIAware() always returns TRUE
		// no matter what, so there is no point in checking its
		// return value.
		SetProcessDPIAware();
	}

	return TRUE;
}

KXUSERAPI HRESULT WINAPI SetProcessDpiAwareness(
	IN	PROCESS_DPI_AWARENESS	Awareness)
{
	BOOLEAN Success;
	
	Success = SetProcessDpiAwarenessInternal(Awareness);

	if (!Success) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

KXUSERAPI HRESULT WINAPI GetDpiForMonitor(
	IN	HMONITOR			Monitor,
	IN	MONITOR_DPI_TYPE	DpiType,
	OUT	PULONG				DpiX,
	OUT	PULONG				DpiY)
{
	HDC DeviceContext;
	BOOL ISPROCESSDPIAWARE;

	if (DpiType >= MDT_MAXIMUM_DPI) {
		return E_INVALIDARG;
	}

	if (!DpiX || !DpiY) {
		return E_INVALIDARG;
	}

	if (!Monitor) {
		return E_INVALIDARG;
	}
	
	ISPROCESSDPIAWARE = IsProcessDPIAware();

	if ((MDT_EFFECTIVE_DPI && !ISPROCESSDPIAWARE) || (DpiType == MDT_ANGULAR_DPI && ISPROCESSDPIAWARE)) {
		*DpiX = USER_DEFAULT_SCREEN_DPI;
		*DpiY = USER_DEFAULT_SCREEN_DPI;
		return S_OK;
	}

	DeviceContext = GetDC(NULL);
	if (!DeviceContext) {
		*DpiX = USER_DEFAULT_SCREEN_DPI;
		*DpiY = USER_DEFAULT_SCREEN_DPI;
		return S_OK;
	}

	if (DpiType == MDT_ANGULAR_DPI && !ISPROCESSDPIAWARE) {
		*DpiX = GetSystemMetrics(SM_CXSCREEN) * 96 / GetDeviceCaps(DeviceContext, DESKTOPHORZRES);
		*DpiY = GetSystemMetrics(SM_CYSCREEN) * 96 / GetDeviceCaps(DeviceContext, DESKTOPVERTRES);
	} else {
		*DpiX = GetDeviceCaps(DeviceContext, LOGPIXELSX);
		*DpiY = GetDeviceCaps(DeviceContext, LOGPIXELSY);
	}

	unless (KexData->IfeoParameters.DisableAppSpecific){
		if (AshExeBaseNameIs(L"java.exe")
			|| AshExeBaseNameIs(L"ABDownloadManager.exe")
			|| AshExeBaseNameIs(L"jetbrains_client64.exe")
			|| AshExeBaseNameIs(L"jetbrains-toolbox.exe")
			|| AshExeBaseNameIs(L"Fleet.exe")
			|| AshExeBaseNameIs(L"aqua64.exe")
			|| AshExeBaseNameIs(L"clion64.exe")
			|| AshExeBaseNameIs(L"datagrip64.exe")
			|| AshExeBaseNameIs(L"dataspell64.exe")
			|| AshExeBaseNameIs(L"goland64.exe")
			|| AshExeBaseNameIs(L"idea64.exe")
			|| AshExeBaseNameIs(L"phpstorm64.exe")
			|| AshExeBaseNameIs(L"pycharm64.exe")
			|| AshExeBaseNameIs(L"rider64.exe")
			|| AshExeBaseNameIs(L"rubymine64.exe")
			|| AshExeBaseNameIs(L"rustrover64.exe")
			|| AshExeBaseNameIs(L"webstorm64.exe")
			|| AshExeBaseNameIs(L"writerside64.exe")) {
			*DpiX = USER_DEFAULT_SCREEN_DPI;
			*DpiY = USER_DEFAULT_SCREEN_DPI;
		};
	};

	ReleaseDC(NULL, DeviceContext);
	return S_OK;
}

KXUSERAPI DEVICE_SCALE_FACTOR WINAPI GetScaleFactorForDevice(
	IN	DISPLAY_DEVICE_TYPE		deviceType)
{
	HDC DeviceContext;
	ULONG LogPixelsX;

	DeviceContext = GetDC(NULL);
	if (!DeviceContext) {
		return SCALE_100_PERCENT;
	}

	LogPixelsX = IsProcessDPIAware() ? GetDeviceCaps(DeviceContext, LOGPIXELSX) : GetDeviceCaps(DeviceContext, DESKTOPHORZRES) * 96 / GetSystemMetrics(SM_CXSCREEN);

	ReleaseDC(NULL, DeviceContext);
	return (DEVICE_SCALE_FACTOR) (LogPixelsX * 100 / 96);
}

KXUSERAPI HRESULT WINAPI GetScaleFactorForMonitor(
	IN	HMONITOR				Monitor,
	OUT	PDEVICE_SCALE_FACTOR	ScaleFactor)
{
	HDC DeviceContext;
	ULONG LogPixelsX;

	DeviceContext = GetDC(NULL);
	if (!DeviceContext) {
		*ScaleFactor = SCALE_100_PERCENT;
		return S_OK;
	}

	LogPixelsX = IsProcessDPIAware() ? GetDeviceCaps(DeviceContext, LOGPIXELSX) : GetDeviceCaps(DeviceContext, DESKTOPHORZRES) * 96 / GetSystemMetrics(SM_CXSCREEN);

	ReleaseDC(NULL, DeviceContext);
	*ScaleFactor = (DEVICE_SCALE_FACTOR) (LogPixelsX * 100 / 96);
	return S_OK;
}

KXUSERAPI UINT WINAPI GetDpiForSystem(
	VOID)
{
	HDC DeviceContext;
	ULONG LogPixelsX;

	if (!IsProcessDPIAware()) {
		return USER_DEFAULT_SCREEN_DPI;
	}

	DeviceContext = GetDC(NULL);
	if (!DeviceContext) {
		return USER_DEFAULT_SCREEN_DPI;
	}

	LogPixelsX = GetDeviceCaps(DeviceContext, LOGPIXELSX);
	ReleaseDC(NULL, DeviceContext);

	return LogPixelsX;
}

KXUSERAPI UINT WINAPI GetDpiForWindow(
	IN	HWND	Window)
{
	return IsWindow(Window) ? GetDpiForSystem() : 0;
}

KXUSERAPI BOOL WINAPI AdjustWindowRectExForDpi(
	IN OUT	LPRECT	Rect,
	IN		ULONG	WindowStyle,
	IN		BOOL	HasMenu,
	IN		ULONG	WindowExStyle,
	IN		ULONG	Dpi)
{
	Rect->left *= Dpi;
	Rect->left /= 96;
	Rect->top *= Dpi;
	Rect->top /= 96;
	Rect->right *= Dpi;
	Rect->right /= 96;
	Rect->bottom *= Dpi;
	Rect->bottom /= 96;
	return AdjustWindowRectEx(
		Rect,
		WindowStyle,
		HasMenu,
		WindowExStyle);
}

KXUSERAPI UINT WINAPI GetDpiForShellUIComponent(
	IN	SHELL_UI_COMPONENT	component)
{
	return GetDpiForSystem();
};

KXUSERAPI BOOL WINAPI LogicalToPhysicalPointForPerMonitorDPI(
	IN		HWND	Window,
	IN OUT	PPOINT	Point)
{
	return LogicalToPhysicalPoint(Window, Point);
}

KXUSERAPI BOOL WINAPI PhysicalToLogicalPointForPerMonitorDPI(
	IN		HWND	Window,
	IN OUT	PPOINT	Point)
{
	return PhysicalToLogicalPoint(Window, Point);
}

KXUSERAPI BOOL WINAPI EnableNonClientDpiScaling(
	IN	HWND	Window)
{
	return TRUE;
}