#include "vxlview.h"
#include "resource.h"

LANGID CURRENTLANG = 0;
PWSTR FRIENDLYAPPNAME;

static PCWSTR SeverityLookupTable_ENG[][2] = {
	{L"Critical",		L"The application encountered a critical error and cannot continue"},
	{L"Error",			L"The application encountered a non-critical error which affected its function"},
	{L"Warning",		L"An event occurred which should be investigated but does not affect the immediate functioning of the application"},
	{L"Information",	L"An informational message which is not a cause for concern"},
	{L"Detail",			L"An informational message, generated in large quantities, which is not a cause for concern"},
	{L"Debug",			L"Information useful only to the developer of the application"},
	{L"Unknown",		L"An invalid or unknown severity value"}
};

static PCWSTR SeverityLookupTable_CHS[][2] = {
	{L"严重错误",		L"应用程序遇到严重错误，无法继续运行"},
	{L"错误",			L"应用程序遇到影响其功能的非严重错误"},
	{L"警告",			L"发生了一个应调查的事件，但不影响应用程序直接运行"},
	{L"信息",			L"信息性消息，无需担心"},
	{L"详细信息",		L"大量产生的信息性消息，无需担心"},
	{L"调试",			L"仅对应用程序开发人员有用的信息"},
	{L"未知",			L"无效或未知的严重程度值"}
};

static PCWSTR SeverityLookupTable_CHT[][2] = {
	{L"嚴重錯誤",		L"應用程式遇到嚴重錯誤，無法繼續執行"},
	{L"錯誤",			L"應用程式遇到影響其功能的非嚴重錯誤"},
	{L"警告",			L"發生了一個應調查的事件，但不影響應用程式直接執行"},
	{L"資訊",			L"資訊性消息，無需擔心"},
	{L"詳細資訊",		L"大量產生的資訊性消息，無需擔心"},
	{L"調試",			L"僅對應用程式開發人員有用的資訊"},
	{L"未知",			L"無效或未知的嚴重程度值"}
};

PCWSTR VxlSeverityToText(
	IN		VXLSEVERITY		Severity,
	IN		BOOLEAN			LongDescription)
{
    PCWSTR (*SeverityLookupTable)[2];
	if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) SeverityLookupTable = SeverityLookupTable_CHS;
	else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) SeverityLookupTable = SeverityLookupTable_CHT;
	else SeverityLookupTable = SeverityLookupTable_ENG;
	return SeverityLookupTable[min((ULONG) Severity, LogSeverityMaximumValue)][!!LongDescription];
}

NTSTATUS NTAPI EntryPoint(
	IN	PVOID	Parameter)
{
	HACCEL Accelerators;
	MSG Message;
	INITCOMMONCONTROLSEX InitComctl;

	switch (GetUserDefaultUILanguage()) {
		case MAKELANGID(LANG_CHINESE, SUBLANG_NEUTRAL):
		case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED):
		case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE):
			CURRENTLANG = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
			FRIENDLYAPPNAME = FRIENDLYAPPNAME_CHS;
			break;
		case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL):
		case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_HONGKONG):
		case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_MACAU):
			CURRENTLANG = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
			FRIENDLYAPPNAME = FRIENDLYAPPNAME_CHT;
			break;
		default:
			CURRENTLANG = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
			FRIENDLYAPPNAME = FRIENDLYAPPNAME_ENG;
			break;
	}
	SetThreadUILanguage(CURRENTLANG);

	InitComctl.dwSize = sizeof(InitComctl);
	InitComctl.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&InitComctl);

	KexgApplicationFriendlyName = FRIENDLYAPPNAME;

	Accelerators = LoadAccelerators(NULL, MAKEINTRESOURCE(IDA_ACCELERATORS));
	CreateDialog(NULL, MAKEINTRESOURCE(IDD_MAINWND), NULL, MainWndProc);

	while (GetMessage(&Message, NULL, 0, 0)) {
		if (TranslateAccelerator(MainWindow, Accelerators, &Message)) {
			continue;
		}

		if (!IsDialogMessage(MainWindow, &Message)) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}

	LdrShutdownProcess();
	return NtTerminateProcess(NtCurrentProcess(), STATUS_SUCCESS);
}

INT_PTR CALLBACK MainWndProc(
	IN	HWND	_MainWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		BOOLEAN Success;
		PWSTR CommandLine;

		UNCONST (HWND) MainWindow = _MainWindow;
		UNCONST (HWND) StatusBarWindow = GetDlgItem(MainWindow, IDC_STATUSBAR);

		KexgApplicationMainWindow = MainWindow;
		SetWindowIcon(MainWindow, IDI_APPICON);

		InitializeBackend();
		InitializeListView();
		InitializeFilterControls();
		InitializeDetailsWindow();
		
		UpdateMainMenu();
		RestoreWindowPlacement();
		RestoreListViewColumns();

		CommandLine = GetCommandLineWithoutImageName();
		if (CommandLine[0] == '\0') {
			if (PROMPT_FOR_FILE_ON_STARTUP) {
				Success = OpenLogFileWithPrompt();
			} else {
				Success = TRUE;
			}
		} else {
			if (CommandLine[0] == '"') {
				PathUnquoteSpaces(CommandLine);
			}

			Success = OpenLogFile(CommandLine);
		}

		if (!Success) {
			ExitProcess(0);
		}
	} else if (Message == WM_CLOSE) {
		SaveListViewColumns();
		SaveWindowPlacement();
		CleanupBackend();
		ExitProcess(0);
	} else if (Message == WM_SIZE && (WParam == SIZENORMAL || WParam == SIZEFULLSCREEN)) {
		USHORT MainWndNewWidth = LOWORD(LParam);

		// DON'T change the order of the resize commands. They depend on each other.
		ResizeStatusBar(MainWndNewWidth);
		ResizeFilterControls();
		ResizeDetailsWindow();
		ResizeListView(MainWndNewWidth);
	} else if (Message == WM_GETMINMAXINFO) {
		PMINMAXINFO MinMaxInfo = (PMINMAXINFO) LParam;

		// Don't allow user to shrink the window too much.
		MinMaxInfo->ptMinTrackSize.x = DpiScaleX(800);
		MinMaxInfo->ptMinTrackSize.y = DpiScaleY(600);
	} else if (Message == WM_SETCURSOR) {
		HCURSOR Cursor;

		Cursor = (HCURSOR) GetClassLongPtr(MainWindow, GCLP_HCURSOR);

		// if the cursor is default, keep the default behavior - otherwise,
		// override the cursor. (intended for busy cursor)
		if (Cursor != LoadCursor(NULL, IDC_ARROW)) {
			SetCursor((HCURSOR) GetClassLongPtr(MainWindow, GCLP_HCURSOR));
			SetWindowLongPtr(MainWindow, DWLP_MSGRESULT, TRUE);
		} else {
			return FALSE;
		}
	} else if (Message == WM_COMMAND) {
		USHORT MessageSource;
		HWND FocusedWindow;
		WCHAR ClassName[32];

		MessageSource = LOWORD(WParam);

		switch (MessageSource) {
		case M_EXIT:
			PostMessage(MainWindow, WM_CLOSE, 0, 0);
			break;
		case M_OPEN:
			OpenLogFileWithPrompt();
			break;
		case M_EXPORT:
			ExportLogWithPrompt();
			break;
		case M_FIND:
			FocusedWindow = GetDlgItem(FilterWindow, IDC_SEARCHBOX);
			Edit_SetSel(FocusedWindow, 0, INT_MAX);
			SetFocus(FocusedWindow);
			break;
		case M_GOTORAW:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_GOTORAW), MainWindow, GotoRawDlgProc);
			break;
		case M_SELECTALL:
			FocusedWindow = GetFocus();
			GetClassName(FocusedWindow, ClassName, ARRAYSIZE(ClassName));

			if (StringEqual(ClassName, L"Edit")) {
				Edit_SetSel(FocusedWindow, 0, INT_MAX);
			}
			
			break;
		case M_ABOUT:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_ABOUT), MainWindow, AboutWndProc);
			break;
		default:
			return FALSE;
		}
	} else if (Message == WM_NOTIFY) {
		LPNMHDR Notification;

		Notification = (LPNMHDR) LParam;

		if (Notification->idFrom == IDC_LISTVIEW) {
			if (Notification->code == LVN_GETDISPINFO) {
				LPLVITEM Item;

				Item = &((NMLVDISPINFO *) LParam)->item;
				PopulateListViewItem(Item);
			} else if (Notification->code == LVN_ITEMCHANGED) {
				LPNMLISTVIEW ChangedItemInfo;

				ChangedItemInfo = (LPNMLISTVIEW) LParam;
				if (ChangedItemInfo->uNewState & LVIS_FOCUSED) {
					if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
						SetWindowTextF(StatusBarWindow, L"已选择条目 #%d。", 
							GetLogEntryRawIndex(ChangedItemInfo->iItem) + 1);
					} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
						SetWindowTextF(StatusBarWindow, L"﻿已選擇條目 #%d。", 
							GetLogEntryRawIndex(ChangedItemInfo->iItem) + 1);
					} else {
						SetWindowTextF(StatusBarWindow, L"Entry #%d selected.", 
							GetLogEntryRawIndex(ChangedItemInfo->iItem) + 1);
					}
					PopulateDetailsWindow(ChangedItemInfo->iItem);
				}
			} else {
				return FALSE;
			}
		} else {
			return FALSE;
		}
	} else if (Message == WM_CONTEXTMENU) {
		HWND Window;
		POINT ClickPoint;

		Window = (HWND) WParam;
		ClickPoint.x = GET_X_LPARAM(LParam);
		ClickPoint.y = GET_Y_LPARAM(LParam);

		if (Window == ListViewWindow) {
			HandleListViewContextMenu(&ClickPoint);
		}
	} else {
		// message not handled
		return FALSE;
	}

	return TRUE;
}

//
// Enable or disable menu items as necessary.
//
VOID UpdateMainMenu(
	VOID)
{
	HMENU MainMenu;

	MainMenu = GetMenu(MainWindow);

	if (IsLogFileOpened()) {
		EnableMenuItem(MainMenu, M_EXPORT, MF_ENABLED);
	} else {
		EnableMenuItem(MainMenu, M_EXPORT, MF_GRAYED);
	}
}