///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     gui.c
//
// Abstract:
//
//     This file contains functions which deal with the user interface (i.e.
//     gathering information interactively from the user).
//
// Author:
//
//     vxiiduu (02-Feb-2024)
//
// Environment:
//
//     Win32, without any vxkex support components
//
// Revision History:
//
//     vxiiduu               02-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "kexsetup.h"
#include <ShlObj.h>

HWND MainWindow = NULL;
ULONG CurrentScene;
HANDLE ElevatedProcess = NULL;

#define SCENE_SELECT_INSTALLATION_DIR	1
#define SCENE_INSTALLING				2
#define SCENE_INSTALLATION_COMPLETE		3

#define SCENE_UNINSTALL_CONFIRM			4
#define SCENE_UNINSTALLING				5
#define SCENE_UNINSTALLATION_COMPLETE	6

#define SCENE_UPDATE_CHANGELOG			7
#define SCENE_UPDATING					8
#define SCENE_UPDATE_COMPLETE			9

typedef HRESULT (WINAPI *PFNENABLETHEMEDIALOGTEXTURE)(HWND, DWORD);

BOOLEAN HideAndDisableControl(
	IN	USHORT	ControlId)
{
	HWND Window;

	ASSERT (IsWindow(MainWindow));

	Window = GetDlgItem(MainWindow, ControlId);

	if (!Window) {
		return FALSE;
	}

	ShowWindow(Window, SW_HIDE);
	EnableWindow(Window, FALSE);
	return TRUE;
}

BOOLEAN ShowAndEnableControl(
	IN	USHORT	ControlId)
{
	HWND Window;

	ASSERT (IsWindow(MainWindow));

	Window = GetDlgItem(MainWindow, ControlId);

	if (!Window) {
		return FALSE;
	}

	ShowWindow(Window, SW_NORMAL);
	EnableWindow(Window, TRUE);
	return TRUE;
}

VOID UpdateDiskFreeSpace(
	VOID)
{
	HWND StaticControlWindow;
	WCHAR InstallationDir[MAX_PATH];
	WCHAR InstallationVolume[MAX_PATH];
	WCHAR FormattedSize[16];
	WCHAR LabelText[22 + ARRAYSIZE(FormattedSize)];
	ULARGE_INTEGER uliFreeSpace;
	StringCchCopy(
		LabelText,
		ARRAYSIZE(LabelText),
		(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"可用磁盘空间：" :
		(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"可用磁碟空間：" :
		L"Disk space available: ");
	StaticControlWindow = GetDlgItem(MainWindow, IDS1SPACEAVAIL);
	GetDlgItemText(MainWindow, IDS1DIRPATH, InstallationDir, ARRAYSIZE(InstallationDir));
	GetVolumePathName(InstallationDir, InstallationVolume, ARRAYSIZE(InstallationVolume));

	if (GetDiskFreeSpaceEx(InstallationVolume, &uliFreeSpace, NULL, NULL)) {
		if (StrFormatByteSize(uliFreeSpace.QuadPart, FormattedSize, ARRAYSIZE(FormattedSize))) {
			StringCchCat(LabelText, ARRAYSIZE(LabelText), FormattedSize);
			SetDlgItemText(MainWindow, IDS1SPACEAVAIL, LabelText);
			return;
		}
	}

	// dont display anything if the directory is invalid
	SetDlgItemText(MainWindow, IDS1SPACEAVAIL, L"");
}

//
// This function is pretty craptacular and only works well for small stuff
// because it doesn't handle errors or large files effectively.
// Hence why it's only used here and not put into KexW32ML.
// Also probably chews a shitload of stack - not so great.
//
ULONG GetDirectorySize(
	IN	PCWSTR	DirectoryPath)
{
	ULONG DirectorySize;
	PWSTR FindSpec;
	ULONG FindSpecCch;
	HANDLE FindHandle;
	WIN32_FIND_DATA FindData;

	ASSERT (DirectoryPath != NULL);
	ASSERT (DirectoryPath[0] != '\0');

	DirectorySize = 0;
	FindSpec = StackAlloc(WCHAR, wcslen(DirectoryPath) + 2 + 1);
	FindSpecCch = (ULONG) wcslen(DirectoryPath) + 2 + 1;
	StringCchPrintf(FindSpec, FindSpecCch, L"%s\\*", DirectoryPath);

	FindHandle = FindFirstFileEx(
		FindSpec,
		FindExInfoBasic,
		&FindData,
		FindExSearchNameMatch,
		NULL,
		FIND_FIRST_EX_LARGE_FETCH);

	if (FindHandle == INVALID_HANDLE_VALUE) {
		FindHandle = FindFirstFileEx(
			FindSpec,
			FindExInfoStandard,
			&FindData,
			FindExSearchNameMatch,
			NULL,
			0);
		if (FindHandle == INVALID_HANDLE_VALUE) {
			return -1;
		}
	}

	do {
		// skip . and ..
		if (FindData.cFileName[0] == '.') {
			if (FindData.cFileName[1] == '.' && FindData.cFileName[2] == '\0') {
				continue;
			}

			if (FindData.cFileName[1] == '\0') {
				continue;
			}
		}

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			WCHAR SubDirPath[MAX_PATH];

			// recurse
			StringCchCopy(SubDirPath, ARRAYSIZE(SubDirPath), DirectoryPath);
			PathCchAppend(SubDirPath, ARRAYSIZE(SubDirPath), FindData.cFileName);
			DirectorySize += GetDirectorySize(SubDirPath);

			continue;
		}

		DirectorySize += FindData.nFileSizeLow;
	} until (!FindNextFile(FindHandle, &FindData) && GetLastError() == ERROR_NO_MORE_FILES);

	return DirectorySize;
}

ULONG GetRequiredSpaceInBytes(
	VOID)
{
	WCHAR ExeDir[MAX_PATH];
	
	GetModuleFileName(NULL, ExeDir, ARRAYSIZE(ExeDir));
	PathCchRemoveFileSpec(ExeDir, ARRAYSIZE(ExeDir));
	return GetDirectorySize(ExeDir);
}

//
// Look in resource.h and you will notice that the control IDs are numerically
// grouped by the scene they belong in.
//
VOID SetScene(
	IN	ULONG	SceneNumber)
{
	BOOLEAN Success;
	USHORT ControlId;
	USHORT MaxControlId;
	PCWSTR HeaderTexts[9][2];
	PCWSTR HeaderTextsENG[9][2] = {
		{L"Choose Install Location",	L"Choose where you want VxKex NEXT to install files."},
		{L"Installing...",				L"Installation is now in progress."},
		{L"Installation Complete",		L"VxKex NEXT is now ready for use."},
		{L"Uninstall VxKex NEXT",		L"Review the information below, and then click \"Uninstall\"."},
		{L"Uninstalling...",			L"Uninstallation is now in progress."},
		{L"Uninstallation Complete",	L"VxKex NEXT has been removed from your computer."},
		{L"Update VxKex",				L"Review the information below, and then click \"Update\"."},
		{L"Updating...",				L"Update is now in progress."},
		{L"Update Complete",			L"VxKex NEXT is now ready for use."}
	};
	PCWSTR HeaderTextsCHS[9][2] = {
		{L"选择安装位置",				L"选择您希望 VxKex NEXT 安装文件的位置。"},
		{L"正在安装...",				L"安装正在进行中。"},
		{L"安装完成",					L"VxKex NEXT 现已准备就绪，可供使用。"},
		{L"卸载 VxKex NEXT",			L"查看以下信息，然后单击“卸载”。"},
		{L"正在卸载...",				L"卸载正在进行中。"},
		{L"卸载完成",					L"已从您的计算机中删除 VxKex NEXT。"},
		{L"更新 VxKex",					L"查看以下信息，然后单击“更新”。"},
		{L"正在更新...",				L"更新正在进行中。"},
		{L"更新完成",					L"VxKex NEXT 现已准备就绪，可供使用。"}
	};
	PCWSTR HeaderTextsCHT[9][2] = {
		{L"選擇安裝位置",				L"選擇您希望 VxKex NEXT 安裝檔案的位置。"},
		{L"正在安裝...",				L"安裝正在進行中。"},
		{L"安裝完成",					L"VxKex NEXT 現已準備就緒，可供使用。"},
		{L"解除安裝 VxKex NEXT",		L"查看以下資訊，然後單擊「解除安裝」。"},
		{L"正在解除安裝...",			L"解除安裝正在進行中。"},
		{L"解除安裝完成",				L"已從您的電腦中刪除 VxKex NEXT。"},
		{L"更新 VxKex",					L"查看以下資訊，然後單擊「更新」。"},
		{L"正在更新...",				L"更新正在進行中。"},
		{L"更新完成",					L"VxKex NEXT 現已準備就緒，可供使用。"}
	};
	int i, j;
	if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
		for (i=0; i<9; ++i) {
			for (j=0; j<2; ++j) {
				HeaderTexts[i][j] = HeaderTextsCHS[i][j];
			}
		};
	} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
		for (i=0; i<9; ++i) {
			for (j=0; j<2; ++j) {
				HeaderTexts[i][j] = HeaderTextsCHT[i][j];
			}
		};
	} else {
		for (i=0; i<9; ++i) {
			for (j=0; j<2; ++j) {
				HeaderTexts[i][j] = HeaderTextsENG[i][j];
			}
		};
	}
	ASSERT (SceneNumber >= 1);
	ASSERT (SceneNumber <= 9);

	if (CurrentScene == 0) {
		// when CurrentScene is zero, that means everything is shown.
		// we have to hide everything first.
		ControlId = 110;
		MaxControlId = 199;
	} else {
		if (SceneNumber == CurrentScene) {
			return;
		}

		ControlId = 100 + ((USHORT) CurrentScene * 10);
		MaxControlId = ControlId + 9;
	}

	while (ControlId <= MaxControlId) {
		HideAndDisableControl(ControlId++);
	}

	ControlId = 100 + ((USHORT) SceneNumber * 10);
	MaxControlId = ControlId + 9;

	while (ControlId <= MaxControlId) {
		Success = ShowAndEnableControl(ControlId++);

		if (!Success) {
			break;
		}
	}

	// Set the header text that appears in the white banner at the top of the window.
	SetDlgItemText(MainWindow, IDHDRTEXT, HeaderTexts[SceneNumber - 1][0]);
	SetDlgItemText(MainWindow, IDHDRSUBTEXT, HeaderTexts[SceneNumber - 1][1]);

	if (SceneNumber == SCENE_SELECT_INSTALLATION_DIR) {
		WCHAR FormattedSpaceString[16];
		WCHAR RequiredSpaceString[64];

		ASSERT (OperationMode == OperationModeInstall);
		StringCchCopy(
			RequiredSpaceString,
			ARRAYSIZE(RequiredSpaceString),
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"所需磁盘空间：最高 " :
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"所需磁碟空間：最高 " :
			L"Disk space required: up to ");
		SetDlgItemText(
			MainWindow,
			IDNEXT,
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"安装(&I)" :
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"安裝(&I)" :
			L"&Install");
		Button_SetShield(GetDlgItem(MainWindow, IDNEXT), TRUE);

		// populate the installation location edit control with KexDir
		SetDlgItemText(MainWindow, IDS1DIRPATH, KexDir);
		
		StrFormatByteSize(
			GetRequiredSpaceInBytes(),
			FormattedSpaceString,
			ARRAYSIZE(FormattedSpaceString));

		StringCchCat(
			RequiredSpaceString,
			ARRAYSIZE(RequiredSpaceString),
			FormattedSpaceString);
		
		SetDlgItemText(MainWindow, IDS1SPACEREQ, RequiredSpaceString);
	} else if (SceneNumber == SCENE_UNINSTALL_CONFIRM) {
		ASSERT (OperationMode == OperationModeUninstall);
		SetDlgItemText(
			MainWindow,
			IDNEXT,
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"卸载(&U)" :
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"解除安裝(&U)" :
			L"&Uninstall");
		Button_SetShield(GetDlgItem(MainWindow, IDNEXT), TRUE);
	} else if (SceneNumber == SCENE_UPDATE_CHANGELOG) {
		HWND EditWindow;
		HANDLE ChangelogFileHandle;
		WCHAR ChangelogPath[MAX_PATH];
		PWSTR Changelog;
		ULONG ChangelogCch;
		ULONG Discard;
		BOOLEAN CannotDisplay;

		CannotDisplay = FALSE;

		ASSERT (OperationMode == OperationModeUpgrade);
		SetDlgItemText(
			MainWindow,
			IDNEXT,
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"更新(&U)" :
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"更新(&U)" :
			L"&Update");
		Button_SetShield(GetDlgItem(MainWindow, IDNEXT), TRUE);

		GetModuleFileName(NULL, ChangelogPath, ARRAYSIZE(ChangelogPath));
		PathCchRemoveFileSpec(ChangelogPath, ARRAYSIZE(ChangelogPath));
		PathCchAppend(ChangelogPath, ARRAYSIZE(ChangelogPath), L"Core\\Changelog.txt");

		ChangelogFileHandle = CreateFile(
			ChangelogPath,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (ChangelogFileHandle == INVALID_HANDLE_VALUE) {
			CannotDisplay = TRUE;
			goto CannotDisplayChangelog;
		}

		//
		// The Changelog must be encoded in Unicode.
		//

		ChangelogCch = (GetFileSize(ChangelogFileHandle, NULL) / sizeof(WCHAR)) + 1;
		Changelog = StackAlloc(WCHAR, ChangelogCch);

		CannotDisplay = !ReadFile(
			ChangelogFileHandle,
			Changelog,
			ChangelogCch * sizeof(WCHAR),
			&Discard,
			NULL);

		CloseHandle(ChangelogFileHandle);

		if (CannotDisplay) {
			goto CannotDisplayChangelog;
		}

		// check for Unicode BOM
		if (Changelog[0] != 0xFEFF) {
			CannotDisplay = TRUE;
			goto CannotDisplayChangelog;
		}
		
		// null terminate
		Changelog[ChangelogCch - 1] = '\0';
		++Changelog;

CannotDisplayChangelog:
		if (CannotDisplay) {
			ASSERT (FALSE);

			Changelog = L"The changelog cannot be displayed.\r\n"
						L"Visit the website to read it:\r\n"
						_L(KEX_WEB_STR);
		}

		EditWindow = GetDlgItem(MainWindow, IDS7CHANGELOG);
		Edit_ReplaceSel(EditWindow, Changelog);
		PostMessage(EditWindow, EM_SETSEL, 0, 0);
	} else {
		HWND NextButton;

		NextButton = GetDlgItem(MainWindow, IDNEXT);
		SetWindowText(
			NextButton,
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"完成(&F)" :
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"完成(&F)" :
			L"&Finish");
		Button_SetShield(NextButton, FALSE);

		switch (SceneNumber) {
		case SCENE_INSTALLING:
		case SCENE_UNINSTALLING:
		case SCENE_UPDATING:
			ShowAndEnableControl(IDPROGRESS);
			SendDlgItemMessage(MainWindow, IDPROGRESS, PBM_SETMARQUEE, TRUE, 0);
			EnableWindow(NextButton, FALSE);
			break;
		default:
			SendDlgItemMessage(MainWindow, IDPROGRESS, PBM_SETMARQUEE, FALSE, 0);
			HideAndDisableControl(IDPROGRESS);
			HideAndDisableControl(IDCANCEL2);
			EnableWindow(NextButton, TRUE);
			break;
		}
	}

	CurrentScene = SceneNumber;
}

HGDIOBJ SetStaticControlBackground(
	IN	HWND	DialogWindow,
	IN	HWND	ControlWindow,
	IN	HDC		DeviceContext)
{
	static HFONT BoldFont = NULL;
	PCWSTR DEFAULTFONTFAMILY;
	DEFAULTFONTFAMILY = (
		CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) ? L"Microsoft Yahei" :
		CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL) ? L"Microsoft Jhenghei" :
		L"Segoe UI");

	if (!BoldFont) {
		BoldFont = CreateFont(
			-MulDiv(8, GetDeviceCaps(DeviceContext, LOGPIXELSY), 72),
			0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			DEFAULTFONTFAMILY);
	}

	switch (GetWindowLongPtr(ControlWindow, GWLP_ID)) {
	case IDHDRTEXT:
		SelectObject(DeviceContext, BoldFont);
		// fall through
	case IDSTATIC2:
	case IDHDRSUBTEXT:
		// Note: don't use hollow brush, that causes glitches when you change the text
		// in a text control.
		// We want this color to match the SS_WHITERECT background. That color would
		// be COLOR_3DHILIGHT. The documentation for SS_WHITERECT says that it uses
		// the "color used to fill the window background", which sounds like it should
		// be COLOR_WINDOW, but the documentation is a total fucking piece of shit
		// which was probably written by some monkey fucking retard who never looked
		// at the very simple source code for the control he was supposed to be
		// "documenting".
		SetBkColor(DeviceContext, GetSysColor(COLOR_3DHILIGHT));
		return GetSysColorBrush(COLOR_3DHILIGHT);
	case IDS7CHANGELOG:
		SetTextColor(DeviceContext, GetSysColor(COLOR_WINDOWTEXT));
		SetBkColor(DeviceContext, GetSysColor(COLOR_WINDOW));
		return GetSysColorBrush(COLOR_WINDOW);
	}

	return FALSE;
}

DWORD WINAPI WaitForElevatedProcessEnd(
	IN	PVOID	Parameter)
{
	NTSTATUS ExitCode;
	ULONG WaitResult;

	ASSERT (ElevatedProcess != NULL);

	if (KexIsDebugBuild) {
		// No timeout for debug build because the elevated process might need to
		// be debugged. Annoying to have the GUI close for no reason during
		// debugging.
		WaitResult = WaitForSingleObject(ElevatedProcess, INFINITE);
	} else {
		WaitResult = WaitForSingleObject(ElevatedProcess, 60000); // 1 minute max
	}

	GetExitCodeProcess(ElevatedProcess, (PULONG) &ExitCode);

	if (WaitResult == WAIT_TIMEOUT) {
		ErrorBoxF(
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"以高权限运行的安装程序进程似乎已停止响应。" :
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"﻿以高權限執行的安裝程式處理程序似乎已停止響應。" :
			L"The elevated setup process appears to have stopped responding.", ExitCode);

		SendMessage(MainWindow, WM_USER + 3, 0, 0);
	} else if (!NT_SUCCESS(ExitCode)) {
		ErrorBoxF(
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"以高权限运行的安装程序进程退出，错误代码：0x%08lx" :
			(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"以高權限執行的安裝程式處理程序退出，錯誤碼：0x%08lx" :
			L"The elevated setup process exited with an error code: 0x%08lx", ExitCode);
		
		SendMessage(MainWindow, WM_USER + 3, 0, 0);
	} else {
		// Instead of calling SetScene directly, we need to ask the main thread
		// to do it. Calling SetScene directly works but subtly breaks the
		// Finish button.
		SendMessage(MainWindow, WM_USER + 2, 0, 0);
	}

	return 0;
}

INT_PTR CALLBACK DialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		HMODULE hUxTheme;
		PFNENABLETHEMEDIALOGTEXTURE pfnEnableThemeDialogTexture;
		BOOL pbEnabled = FALSE;

		MainWindow = Window;
		KexgApplicationMainWindow = MainWindow;

		hUxTheme = LoadLibrary(L"uxtheme.dll");
		pfnEnableThemeDialogTexture = (PFNENABLETHEMEDIALOGTEXTURE) GetProcAddress(hUxTheme, "EnableThemeDialogTexture");
		if (pfnEnableThemeDialogTexture) pfnEnableThemeDialogTexture(Window, ETDT_ENABLE | ETDT_USEAEROWIZARDTABTEXTURE);
		FreeLibrary(hUxTheme);

		CurrentScene = 0;

		SetScene((OperationMode * 3) + 1);

		if (OperationMode == OperationModeInstall) {
			HRESULT Result;
			Result = SHAutoComplete(GetDlgItem(Window, IDS1DIRPATH), SHACF_FILESYS_DIRS | SHACF_USETAB);
			ASSERT (SUCCEEDED(Result));
		} else if (OperationMode == OperationModeUninstall) {
			CheckDlgButton(Window, IDS4PRESERVECONFIG, PreserveConfig ? BST_CHECKED : BST_UNCHECKED);
			
			ToolTip(
				Window,
				IDS4PRESERVECONFIG,
				(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"保留应用程序的兼容性设置（如 Windows 版本欺骗）。这些应用程序仍将禁用 VxKex NEXT，如果您决定重新安装，则需要重新启用 VxKex NEXT。" :
				(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"保留應用程式的相容性設定（如 Windows 版本欺騙）。這些應用程式仍將停用 VxKex NEXT，如果您決定重新安裝，則需要重新啟用 VxKex NEXT。" :
				(L"Preserve the application compatibility settings (such as Windows version spoofing) "
				L"for applications. VxKex NEXT will still be disabled for these applications and you will "
				L"need to re-enable VxKex NEXT if you decide to reinstall."));
		}
	} else if (Message == WM_DRAWITEM) {
		LPDRAWITEMSTRUCT lpDrawItemStruct = (LPDRAWITEMSTRUCT) LParam;
		if (lpDrawItemStruct->CtlID == IDSTATIC4) {
			RECT Rect;
			HDC hdc = lpDrawItemStruct->hDC;
			HBRUSH hBrush = CreateSolidBrush(RGB(225, 235, 249));
			GetClientRect(lpDrawItemStruct->hwndItem, &Rect);
			FillRect(hdc, &Rect, hBrush);
			DeleteObject(hBrush);
		}
	} else if (Message == WM_CLOSE) {
		DialogProc(Window, WM_COMMAND, IDCANCEL2, 0);
	} else if (Message == WM_COMMAND) {
		USHORT ControlId;

		ControlId = LOWORD(WParam);

		if (ControlId == IDCANCEL2) {
			if (CurrentScene == SCENE_INSTALLING ||
				CurrentScene == SCENE_UPDATING ||
				CurrentScene == SCENE_UNINSTALLING) {

				INT UserResponse;

				UserResponse = MessageBox(
					Window,
					(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"您要取消设置吗？" :
					(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"您要取消設定嗎？" :
					L"Do you want to cancel Setup?",
					FRIENDLYAPPNAME,
					MB_ICONQUESTION | MB_YESNO);

				if (UserResponse == IDYES) {
					EndDialog(Window, 0);
				}
			} else {
				EndDialog(Window, 0);
			}
		} else if (ControlId == IDS1DIRPATH) {
			UpdateDiskFreeSpace();
		} else if (ControlId == IDS1BROWSE) {
			BOOLEAN UserPickedAFolder;
			WCHAR DirectoryPath[MAX_PATH];

			GetDlgItemText(Window, IDS1DIRPATH, DirectoryPath, ARRAYSIZE(DirectoryPath));

			UserPickedAFolder = PickFolder(
				Window,
				DirectoryPath,
				FOS_NOREADONLYRETURN,
				DirectoryPath,
				ARRAYSIZE(DirectoryPath));

			if (UserPickedAFolder) {
				PathCchAppend(DirectoryPath, ARRAYSIZE(DirectoryPath), L"VxKex");
			}

			SetDlgItemText(Window, IDS1DIRPATH, DirectoryPath);
			StringCchCopy(KexDir, ARRAYSIZE(KexDir), DirectoryPath);
		} else if (ControlId == IDNEXT) {
			if (CurrentScene == SCENE_INSTALLATION_COMPLETE ||
				CurrentScene == SCENE_UNINSTALLATION_COMPLETE ||
				CurrentScene == SCENE_UPDATE_COMPLETE) {

				if (IsDlgButtonChecked(Window, IDS3KEXCFG)) {
					WCHAR KexCfgFullPath[MAX_PATH];

					// "Open global configuration" checkbox is checked.
					StringCchPrintf(
						KexCfgFullPath,
						ARRAYSIZE(KexCfgFullPath),
						L"%s\\KexCfg.exe",
						KexDir);

					ShellExecute(Window, L"open", KexCfgFullPath, NULL, NULL, SW_SHOW);
				}

				// under these conditions the Next button turns into a Finish button
				EndDialog(Window, 0);
			} else {
				WCHAR ExeFullPath[MAX_PATH];
				PWSTR CommandLineArgs;
				ULONG ShellExecuteResult;
				
				GetModuleFileName(NULL, ExeFullPath, ARRAYSIZE(ExeFullPath));

				StringAllocPrintf(
					&CommandLineArgs,
					NULL,
					L"/SILENTUNATTEND /HWND:%u %s %s %s%s%s",
					Window,
					PreserveConfig ? L"/PRESERVECONFIG" : L"",
					OperationMode == OperationModeUninstall ? L"/UNINSTALL" : L"",
					OperationMode == OperationModeInstall ? L"/KEXDIR:\"" : L"",
					OperationMode == OperationModeInstall ? KexDir : L"",
					OperationMode == OperationModeInstall ? L"\"" : L"");

				ShellExecuteResult = (ULONG) ShellExecute(
					MainWindow,
					L"runas",
					ExeFullPath,
					CommandLineArgs,
					NULL,
					SW_SHOWNORMAL);

				SafeFree(CommandLineArgs);

				// SE_ERR_ACCESSDENIED happens when user clicks No on the
				// elevation prompt, so it's not an error for us.
				if (ShellExecuteResult <= 32 && ShellExecuteResult != SE_ERR_ACCESSDENIED) {
					ErrorBoxF(
						(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? L"ShellExecute 失败，错误代码为 %d。设置无法继续。" :
						(CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? L"ShellExecute 失敗，錯誤碼為 %d。設定無法繼續。" :
						L"ShellExecute failed with error code %d. Setup cannot continue.", ShellExecuteResult);
					ExitProcess(STATUS_UNSUCCESSFUL);
				}
			}
		} else if (ControlId == IDS4PRESERVECONFIG) {
			PreserveConfig = IsDlgButtonChecked(Window, ControlId);
		} else {
			return FALSE;
		}
	} else if (Message == WM_CTLCOLORSTATIC) {
		return (INT_PTR) SetStaticControlBackground(Window, (HWND) LParam, (HDC) WParam);
	} else if (Message == WM_USER + 1) {
		// LPARAM indicates a process handle sent by the elevated process.
		ASSERT (LParam != 0);
		ASSERT (ElevatedProcess == NULL);

		ElevatedProcess = (HANDLE) LParam;
		SetScene(CurrentScene + 1);

		CreateThread(
			NULL,
			0,
			WaitForElevatedProcessEnd,
			NULL,
			0,
			NULL);
	} else if (Message == WM_USER + 2) {
		SetScene(CurrentScene + 1);
	} else if (Message == WM_USER + 3) {
		// sent after displaying a fatal error dialog to the user
		EndDialog(Window, 0);
	} else {
		return FALSE;
	}

	return TRUE;
}

VOID DisplayInstallerGUI(
	VOID)
{
	unless (OperationMode == OperationModeUninstall) {
		KexSetupCheckForPrerequisites();
	}

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MAINWINDOW), NULL, DialogProc);
}