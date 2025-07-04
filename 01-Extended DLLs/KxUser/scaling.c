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
	HMODULE Shcore;
	HRESULT (WINAPI *pGetProcessDpiAwareness) (HANDLE, PROCESS_DPI_AWARENESS *);
	BOOLEAN Success;

	Shcore = LoadSystemLibrary(L"shcore.dll");
	pGetProcessDpiAwareness = (HRESULT (WINAPI *) (HANDLE, PROCESS_DPI_AWARENESS *)) GetProcAddress(Shcore, "GetProcessDpiAwareness");
	if (pGetProcessDpiAwareness) {
		HRESULT Result = pGetProcessDpiAwareness(ProcessHandle, DpiAwareness);
		FreeLibrary(Shcore);
		return Result;
	}
	FreeLibrary(Shcore);

	Success = GetProcessDpiAwarenessInternal(ProcessHandle, DpiAwareness);

	if (!Success) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetThreadDpiAwarenessContext(
	VOID)
{
	HMODULE User32;
	DPI_AWARENESS_CONTEXT (WINAPI *pGetThreadDpiAwarenessContext) (VOID);
	PROCESS_DPI_AWARENESS DpiAwareness = PROCESS_DPI_UNAWARE;

	User32 = LoadSystemLibrary(L"user32.dll");
	pGetThreadDpiAwarenessContext = (DPI_AWARENESS_CONTEXT (WINAPI *) (VOID)) GetProcAddress(User32, "GetThreadDpiAwarenessContext");
	if (pGetThreadDpiAwarenessContext) {
		DPI_AWARENESS_CONTEXT DpiContext = pGetThreadDpiAwarenessContext();
		FreeLibrary(User32);
		return DpiContext;
	}
	FreeLibrary(User32);

	GetProcessDpiAwareness(NULL, &DpiAwareness);
	switch (DpiAwareness) {
	case PROCESS_DPI_UNAWARE:
		return DPI_AWARENESS_CONTEXT_UNAWARE;
	case PROCESS_SYSTEM_DPI_AWARE:
		return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
	case PROCESS_PER_MONITOR_DPI_AWARE:
		return DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
	default:
		return 0;
	}
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
	HMODULE Shcore;
	HRESULT (WINAPI *pSetProcessDpiAwareness) (PROCESS_DPI_AWARENESS);
	BOOLEAN Success;

	Shcore = LoadSystemLibrary(L"shcore.dll");
	pSetProcessDpiAwareness = (HRESULT (WINAPI *) (PROCESS_DPI_AWARENESS)) GetProcAddress(Shcore, "SetProcessDpiAwareness");
	if (pSetProcessDpiAwareness) {
		HRESULT Result = pSetProcessDpiAwareness(Awareness);
		FreeLibrary(Shcore);
		return Result;
	}
	FreeLibrary(Shcore);
	
	Success = SetProcessDpiAwarenessInternal(Awareness);

	if (!Success) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	DpiContext)
{
	HMODULE User32;
	DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext) (DPI_AWARENESS_CONTEXT);
	PROCESS_DPI_AWARENESS OldDpiAwareness = PROCESS_DPI_UNAWARE;

	User32 = LoadSystemLibrary(L"user32.dll");
	pSetThreadDpiAwarenessContext = (DPI_AWARENESS_CONTEXT (WINAPI *) (DPI_AWARENESS_CONTEXT)) GetProcAddress(User32, "SetThreadDpiAwarenessContext");
	if (pSetThreadDpiAwarenessContext) {
		DPI_AWARENESS_CONTEXT OldDpiContext = pSetThreadDpiAwarenessContext(DpiContext);
		FreeLibrary(User32);
		return OldDpiContext;
	}
	FreeLibrary(User32);

	GetProcessDpiAwareness(NULL, &OldDpiAwareness);

	switch (DpiContext) {
	case DPI_AWARENESS_CONTEXT_UNAWARE:
	case DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED:
		NOTHING;
		break;
	case DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
		SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
		break;
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2:
		SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
		break;
	default:
		return 0;
	}

	switch (OldDpiAwareness) {
	case PROCESS_DPI_UNAWARE:
		return DPI_AWARENESS_CONTEXT_UNAWARE;
	case PROCESS_SYSTEM_DPI_AWARE:
		return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
	case PROCESS_PER_MONITOR_DPI_AWARE:
		return DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
	default:
		return 0;
	}
}

KXUSERAPI BOOL WINAPI SetProcessDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	DpiContext)
{
	HMODULE User32;
	BOOL (WINAPI *pSetProcessDpiAwarenessContext) (DPI_AWARENESS_CONTEXT);

	User32 = LoadSystemLibrary(L"user32.dll");
	pSetProcessDpiAwarenessContext = (BOOL (WINAPI *) (DPI_AWARENESS_CONTEXT)) GetProcAddress(User32, "SetProcessDpiAwarenessContext");
	if (pSetProcessDpiAwarenessContext) {
		BOOL Success = pSetProcessDpiAwarenessContext(DpiContext);
		FreeLibrary(User32);
		return Success;
	}
	FreeLibrary(User32);

	switch (DpiContext) {
	case DPI_AWARENESS_CONTEXT_UNAWARE:
	case DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED:
		NOTHING;
		break;
	case DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
		SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
		break;
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2:
		SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
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

KXUSERAPI HRESULT WINAPI GetDpiForMonitor(
	IN	HMONITOR			Monitor,
	IN	MONITOR_DPI_TYPE	DpiType,
	OUT	PUINT				DpiX,
	OUT	PUINT				DpiY)
{
	HMODULE Shcore;
	HRESULT (WINAPI *pGetDpiForMonitor) (HMONITOR, MONITOR_DPI_TYPE, PUINT, PUINT);
	HDC DeviceContext;
	BOOL ISPROCESSDPIAWARE;

	Shcore = LoadSystemLibrary(L"shcore.dll");
	pGetDpiForMonitor = (HRESULT (WINAPI *) (HMONITOR, MONITOR_DPI_TYPE, PUINT, PUINT)) GetProcAddress(Shcore, "GetDpiForMonitor");
	if (pGetDpiForMonitor) {
		HRESULT Result = pGetDpiForMonitor(Monitor, DpiType, DpiX, DpiY);
		FreeLibrary(Shcore);
		return Result;
	}
	FreeLibrary(Shcore);

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

	if ((DpiType == MDT_EFFECTIVE_DPI && !ISPROCESSDPIAWARE) || (DpiType == MDT_ANGULAR_DPI && ISPROCESSDPIAWARE)) {
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
		*DpiX = GetSystemMetrics(SM_CXSCREEN) * USER_DEFAULT_SCREEN_DPI / GetDeviceCaps(DeviceContext, DESKTOPHORZRES);
		*DpiY = GetSystemMetrics(SM_CYSCREEN) * USER_DEFAULT_SCREEN_DPI / GetDeviceCaps(DeviceContext, DESKTOPVERTRES);
	} else {
		*DpiX = GetDeviceCaps(DeviceContext, LOGPIXELSX);
		*DpiY = GetDeviceCaps(DeviceContext, LOGPIXELSY);
	}

	unless (KexData->IfeoParameters.DisableAppSpecific) {
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
		}
	}

	ReleaseDC(NULL, DeviceContext);
	return S_OK;
}

KXUSERAPI DEVICE_SCALE_FACTOR WINAPI GetScaleFactorForDevice(
	IN	DISPLAY_DEVICE_TYPE		deviceType)
{
	HMODULE Shcore;
	DEVICE_SCALE_FACTOR (WINAPI *pGetScaleFactorForDevice) (DISPLAY_DEVICE_TYPE);
	HDC DeviceContext;
	ULONG LogPixelsX;

	Shcore = LoadSystemLibrary(L"shcore.dll");
	pGetScaleFactorForDevice = (DEVICE_SCALE_FACTOR (WINAPI *) (DISPLAY_DEVICE_TYPE)) GetProcAddress(Shcore, "GetScaleFactorForDevice");
	if (pGetScaleFactorForDevice) {
		DEVICE_SCALE_FACTOR DeviceScaleFactor = pGetScaleFactorForDevice(deviceType);
		FreeLibrary(Shcore);
		return DeviceScaleFactor;
	}
	FreeLibrary(Shcore);

	DeviceContext = GetDC(NULL);
	if (!DeviceContext) {
		return SCALE_100_PERCENT;
	}

	LogPixelsX = IsProcessDPIAware() ? GetDeviceCaps(DeviceContext, LOGPIXELSX) : GetDeviceCaps(DeviceContext, DESKTOPHORZRES) * USER_DEFAULT_SCREEN_DPI / GetSystemMetrics(SM_CXSCREEN);

	ReleaseDC(NULL, DeviceContext);
	return (DEVICE_SCALE_FACTOR) (LogPixelsX * SCALE_100_PERCENT / USER_DEFAULT_SCREEN_DPI);
}

KXUSERAPI HRESULT WINAPI GetScaleFactorForMonitor(
	IN	HMONITOR				Monitor,
	OUT	PDEVICE_SCALE_FACTOR	ScaleFactor)
{
	HMODULE Shcore;
	HRESULT (WINAPI *pGetScaleFactorForMonitor) (HMONITOR, PDEVICE_SCALE_FACTOR);
	HDC DeviceContext;
	ULONG LogPixelsX;

	Shcore = LoadSystemLibrary(L"shcore.dll");
	pGetScaleFactorForMonitor = (HRESULT (WINAPI *) (HMONITOR, PDEVICE_SCALE_FACTOR)) GetProcAddress(Shcore, "GetScaleFactorForMonitor");
	if (pGetScaleFactorForMonitor) {
		HRESULT Result = pGetScaleFactorForMonitor(Monitor, ScaleFactor);
		FreeLibrary(Shcore);
		return Result;
	}
	FreeLibrary(Shcore);

	DeviceContext = GetDC(NULL);
	if (!DeviceContext) {
		*ScaleFactor = SCALE_100_PERCENT;
		return S_OK;
	}

	LogPixelsX = IsProcessDPIAware() ? GetDeviceCaps(DeviceContext, LOGPIXELSX) : GetDeviceCaps(DeviceContext, DESKTOPHORZRES) * USER_DEFAULT_SCREEN_DPI / GetSystemMetrics(SM_CXSCREEN);

	ReleaseDC(NULL, DeviceContext);
	*ScaleFactor = (DEVICE_SCALE_FACTOR) (LogPixelsX * SCALE_100_PERCENT / USER_DEFAULT_SCREEN_DPI);
	return S_OK;
}

KXUSERAPI UINT WINAPI GetDpiForSystem(
	VOID)
{
	HMODULE User32;
	UINT (WINAPI *pGetDpiForSystem) (VOID);
	PROCESS_DPI_AWARENESS DpiAwareness = PROCESS_DPI_UNAWARE;
	HDC DeviceContext;
	ULONG LogPixelsX;

	User32 = LoadSystemLibrary(L"user32.dll");
	pGetDpiForSystem = (UINT (WINAPI *) (VOID)) GetProcAddress(User32, "GetDpiForSystem");
	if (pGetDpiForSystem) {
		UINT Dpi = pGetDpiForSystem();
		FreeLibrary(User32);
		return Dpi;
	}
	FreeLibrary(User32);

	GetProcessDpiAwareness(NULL, &DpiAwareness);
	if (DpiAwareness == PROCESS_DPI_UNAWARE) {
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
	HMODULE User32;
	HMODULE Shcore;
	UINT (WINAPI *pGetDpiForWindow) (HWND);
	HRESULT (WINAPI *pGetDpiForMonitor) (HMONITOR, MONITOR_DPI_TYPE, PUINT, PUINT);

	User32 = LoadSystemLibrary(L"user32.dll");
	pGetDpiForWindow = (UINT (WINAPI *) (HWND)) GetProcAddress(User32, "GetDpiForWindow");
	if (pGetDpiForWindow) {
		UINT Dpi = pGetDpiForWindow(Window);
		FreeLibrary(User32);
		return Dpi;
	}
	FreeLibrary(User32);

	if (!IsWindow(Window)) return 0;

	Shcore = LoadSystemLibrary(L"shcore.dll");
	pGetDpiForMonitor = (HRESULT (WINAPI *) (HMONITOR, MONITOR_DPI_TYPE, PUINT, PUINT)) GetProcAddress(Shcore, "GetDpiForMonitor");
	if (pGetDpiForMonitor) {
		HMONITOR Monitor = MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY);
		HRESULT Result;
		UINT DpiX = USER_DEFAULT_SCREEN_DPI, DpiY = USER_DEFAULT_SCREEN_DPI;
		if (!Monitor) {
			FreeLibrary(Shcore);
			return GetDpiForSystem();
		}
		Result = pGetDpiForMonitor(Monitor, MDT_EFFECTIVE_DPI, &DpiX, &DpiY);
		if (FAILED(Result)) {
			FreeLibrary(Shcore);
			return GetDpiForSystem();
		}
		return DpiX;
	}
	FreeLibrary(Shcore);

	return GetDpiForSystem();
}

KXUSERAPI BOOL WINAPI AdjustWindowRectExForDpi(
	IN OUT	LPRECT	Rect,
	IN		ULONG	WindowStyle,
	IN		BOOL	HasMenu,
	IN		ULONG	WindowExStyle,
	IN		ULONG	Dpi)
{
	if (!Rect) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	Rect->left *= Dpi;
	Rect->left /= USER_DEFAULT_SCREEN_DPI;
	Rect->top *= Dpi;
	Rect->top /= USER_DEFAULT_SCREEN_DPI;
	Rect->right *= Dpi;
	Rect->right /= USER_DEFAULT_SCREEN_DPI;
	Rect->bottom *= Dpi;
	Rect->bottom /= USER_DEFAULT_SCREEN_DPI;
	return AdjustWindowRectEx(
		Rect,
		WindowStyle,
		HasMenu,
		WindowExStyle);
}

KXUSERAPI UINT WINAPI GetDpiForShellUIComponent(
	IN	SHELL_UI_COMPONENT	component)
{
	HMODULE Shcore;
	UINT (WINAPI *pGetDpiForShellUIComponent) (SHELL_UI_COMPONENT);

	Shcore = LoadSystemLibrary(L"shcore.dll");
	pGetDpiForShellUIComponent = (UINT (WINAPI *) (SHELL_UI_COMPONENT)) GetProcAddress(Shcore, "GetDpiForShellUIComponent");
	if (pGetDpiForShellUIComponent) {
		UINT Dpi = pGetDpiForShellUIComponent(component);
		FreeLibrary(Shcore);
		return Dpi;
	}
	FreeLibrary(Shcore);

	return GetDpiForSystem();
}

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
	HMODULE User32;
	BOOL (WINAPI *pEnableNonClientDpiScaling) (HWND);

	User32 = LoadSystemLibrary(L"user32.dll");
	pEnableNonClientDpiScaling = (BOOL (WINAPI *) (HWND)) GetProcAddress(User32, "EnableNonClientDpiScaling");
	if (pEnableNonClientDpiScaling) {
		BOOL Success = pEnableNonClientDpiScaling(Window);
		FreeLibrary(User32);
		return Success;
	}
	FreeLibrary(User32);

	return TRUE;
}