///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     gui.c
//
// Abstract:
//
//     Implements the user interface logic of the shell extension.
//
// Author:
//
//     vxiiduu (08-Feb-2024)
//
// Environment:
//
//     Inside explorer.exe
//
// Revision History:
//
//     vxiiduu              08-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "KexShlEx.h"
#include "resource.h"

INT_PTR CALLBACK DialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	PKEXSHLEX_PROPSHEET_DATA PropSheetData;

	PropSheetData = (PKEXSHLEX_PROPSHEET_DATA) GetWindowLongPtr(Window, GWLP_USERDATA);

	if (Message == WM_INITDIALOG) {
		BOOLEAN Success;
		IKexShlEx *CKexShlEx;
		LPPROPSHEETPAGE PropSheetPage;
		PCWSTR ExeFullPath;
		HWND WinVerComboBox;
		KXCFG_PROGRAM_CONFIGURATION ProgramConfiguration;
		ULONG Index;

		ASSERT (PropSheetData == NULL);
		PropSheetPage = (LPPROPSHEETPAGE) LParam;
		ASSERT (PropSheetPage != NULL);
		CKexShlEx = (IKexShlEx *) PropSheetPage->lParam;
		ASSERT (CKexShlEx != NULL);
		ExeFullPath = CKexShlEx->ExeFullPath;

		//
		// Allocate a structure for us to store the path to the executable
		// as well as record whether any of the settings have changed.
		//

		PropSheetData = SafeAlloc(KEXSHLEX_PROPSHEET_DATA, 1);
		if (!PropSheetData) {
			for (Index = 110; Index < 130; ++Index) {
				EnableWindow(GetDlgItem(Window, Index), FALSE);
			}
		}

		if (PropSheetData) {
			PropSheetData->ExeFullPath		= ExeFullPath;
			PropSheetData->SettingsChanged	= FALSE;
		}

		//
		// Associate the data structure with the property sheet.
		//

		SetWindowLongPtr(Window, GWLP_USERDATA, (LONG_PTR) PropSheetData);

		//
		// Populate the Windows version combo box.
		//

		WinVerComboBox = GetDlgItem(Window, IDWINVERCOMBOBOX);

		ComboBox_AddString(WinVerComboBox, L"Windows 7 (Service Pack 1)");
		ComboBox_AddString(WinVerComboBox, L"Windows 8");
		ComboBox_AddString(WinVerComboBox, L"Windows 8.1");
		ComboBox_AddString(WinVerComboBox, L"Windows 10");
		ComboBox_AddString(WinVerComboBox, L"Windows 11");

		// Set default selection to Windows 10.
		ComboBox_SetCurSel(WinVerComboBox, 3);

		//
		// Query the VxKex configuration for the current program.
		//

		Success = KxCfgGetConfiguration(
			ExeFullPath,
			&ProgramConfiguration);

		if (Success) {
			CheckDlgButton(Window, IDUSEVXKEX,				!!ProgramConfiguration.Enabled);

			if (ProgramConfiguration.WinVerSpoof != WinVerSpoofNone) {
				EnableWindow(WinVerComboBox, TRUE);
				EnableWindow(GetDlgItem(Window, IDSTRONGSPOOF), TRUE);
				CheckDlgButton(Window, IDSPOOFVERSIONCHECK, BST_CHECKED);
				ComboBox_SetCurSel(WinVerComboBox, ProgramConfiguration.WinVerSpoof - 1);
			}

			CheckDlgButton(Window, IDSTRONGSPOOF,			!!ProgramConfiguration.StrongSpoofOptions);
			CheckDlgButton(Window, IDDISABLEFORCHILD,		!!ProgramConfiguration.DisableForChild);
			CheckDlgButton(Window, IDDISABLEAPPSPECIFIC,	!!ProgramConfiguration.DisableAppSpecificHacks);
		}

		//
		// Add tooltips to all the settings.
		//

		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
			ToolTip(Window, IDUSEVXKEX,
				L"启用或禁用主 VxKex NEXT 兼容层。");
			ToolTip(Window, IDSPOOFVERSIONCHECK,
				L"一些应用程序会检查 Windows 版本，如果版本过低，则拒绝运行。"
				L"此选项可以帮助这些应用程序正常运行。\r\n\r\n"
				L"通常，不应使用高于运行应用程序所需的 Windows 版本，因为这会降低应用程序的兼容性。");
			ToolTip(Window, IDSTRONGSPOOF,
				L"一些应用程序使用不常用的方法检查 Windows 版本。"
				L"这个选项可以帮助诱使它们工作。"
				L"除非您遇到版本检测方面的问题，否则请勿启用此设置。");
			ToolTip(Window, IDDISABLEFORCHILD,
				L"默认情况下，由此程序启动的所有其他程序都会启用 VxKex NEXT 运行。此选项禁用这一行为。");
			ToolTip(Window, IDDISABLEAPPSPECIFIC,
				L"对于某些应用程序，VxKex NEXT 可能会使用特定于应用程序的变通方法或修补程序。"
				L"此选项禁用这一行为。使用此选项可能会降低应用程序的兼容性。");
		} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
			ToolTip(Window, IDUSEVXKEX,
				L"啟用或停用主 VxKex NEXT 相容層。");
			ToolTip(Window, IDSPOOFVERSIONCHECK,
				L"一些應用程式會檢查 Windows 版本，如果版本過低，則拒絕執行。"
				L"此選項可以幫助這些應用程式正常執行。\r\n\r\n"
				L"通常，不應使用高於執行應用程式所需的 Windows 版本，因為這會降低應用程式的相容性。");
			ToolTip(Window, IDSTRONGSPOOF,
				L"一些應用程式使用不常用的方法檢查 Windows 版本。"
				L"這個選項可以幫助誘使它們工作。"
				L"除非您遇到版本檢測方面的問題，否則請勿啟用此設定。");
			ToolTip(Window, IDDISABLEFORCHILD,
				L"預設情況下，由此程式啟動的所有其他程式都會啟用 VxKex NEXT 執行。此選項停用這一行為。");
			ToolTip(Window, IDDISABLEAPPSPECIFIC,
				L"對於某些應用程式，VxKex NEXT 可能會使用特定於應用程式的變通方法或修補程式。"
				L"此選項停用這一行為。使用此選項可能會降低應用程式的相容性。");
		} else {
			ToolTip(Window, IDUSEVXKEX,
				L"Enable or disable the main VxKex NEXT compatibility layer.");
			ToolTip(Window, IDSPOOFVERSIONCHECK,
				L"Some applications check the Windows version and refuse to run if it is too low. "
				L"This option can help these applications to run correctly.\r\n\r\n"
				L"Generally, you should not use a higher Windows version than required to run the "
				L"application, because this can degrade application compatibility.");
			ToolTip(Window, IDSTRONGSPOOF,
				L"Some applications check the Windows version using uncommon methods. This option "
				L"can help trick them into working. Do not enable this setting unless you are having "
				L"a problem with version detection.");
			ToolTip(Window, IDDISABLEFORCHILD,
				L"By default, all other programs that are started by this program run with VxKex NEXT "
				L"enabled. This option disables that behavior.");
			ToolTip(Window, IDDISABLEAPPSPECIFIC,
				L"For some applications, VxKex NEXT may use application-specific workarounds or patches. "
				L"This option disables that behavior. Using this option may degrade application "
				L"compatibility.");
			
		}
		ToolTip(Window, IDREPORTBUG, _L(KEX_BUGREPORT_STR));

		SetFocus(GetDlgItem(Window, IDUSEVXKEX));
		return FALSE;
	} else if (Message == WM_COMMAND) {
		if (LOWORD(WParam) == IDSPOOFVERSIONCHECK) {
			BOOLEAN VersionSpoofEnabled;

			VersionSpoofEnabled = !!IsDlgButtonChecked(Window, IDSPOOFVERSIONCHECK);

			// enable the win ver combo box when the user enables version spoof
			EnableWindow(GetDlgItem(Window, IDWINVERCOMBOBOX), VersionSpoofEnabled);

			// enable the strong spoof checkbox when user enables version spoof
			EnableWindow(GetDlgItem(Window, IDSTRONGSPOOF), VersionSpoofEnabled);
		}

		// this causes the "Apply" button to be enabled
		PropSheet_Changed(GetParent(Window), Window);

		// Record the change to the settings.
		if (PropSheetData) {
			PropSheetData->SettingsChanged = TRUE;
		}
	} else if (Message == WM_NOTIFY && ((LPNMHDR) LParam)->code == PSN_APPLY &&
			   PropSheetData && PropSheetData->SettingsChanged) {

		//
		// The OK or Apply button was clicked and we need to apply new settings.
		//

		KXCFG_PROGRAM_CONFIGURATION ProgramConfiguration;

		ZeroMemory(&ProgramConfiguration, sizeof(ProgramConfiguration));

		ProgramConfiguration.Enabled					= IsDlgButtonChecked(Window, IDUSEVXKEX);

		if (IsDlgButtonChecked(Window, IDSPOOFVERSIONCHECK)) {
			ProgramConfiguration.WinVerSpoof = (KEX_WIN_VER_SPOOF) (ComboBox_GetCurSel(GetDlgItem(Window, IDWINVERCOMBOBOX)) + 1);
		} else {
			ProgramConfiguration.WinVerSpoof			= WinVerSpoofNone;
		}

		ProgramConfiguration.StrongSpoofOptions			= IsDlgButtonChecked(Window, IDSTRONGSPOOF) ? KEX_STRONGSPOOF_VALID_MASK : 0;
		ProgramConfiguration.DisableForChild			= IsDlgButtonChecked(Window, IDDISABLEFORCHILD);
		ProgramConfiguration.DisableAppSpecificHacks	= IsDlgButtonChecked(Window, IDDISABLEAPPSPECIFIC);

		//
		// All the configuration is inside the ProgramConfiguration structure.
		// Call a helper function to carry out the steps necessary to write it
		// to the registry.
		//

		if (PropSheetData) {
			KxCfgSetConfiguration(PropSheetData->ExeFullPath, &ProgramConfiguration, NULL);
			PropSheetData->SettingsChanged = FALSE;
		}
	} else if (Message == WM_NOTIFY && WParam == IDREPORTBUG) {
		ULONG NotificationCode;

		NotificationCode = ((LPNMHDR) LParam)->code;

		if (NotificationCode == NM_CLICK || NotificationCode == NM_RETURN) {
			// user clicked the report bug button - open in his default browser
			ShellExecute(Window, L"open", _L(KEX_BUGREPORT_STR), NULL, NULL, SW_NORMAL);
		}
	} else {
		return FALSE;
	}

	return TRUE;
}

UINT WINAPI PropSheetCallbackProc(
	IN		HWND				Window,
	IN		UINT				Message,
	IN OUT	LPPROPSHEETPAGE		PropSheetPage)
{
	IKexShlEx *CKexShlEx;

	ASSERT (PropSheetPage != NULL);

	CKexShlEx = (IKexShlEx *) PropSheetPage->lParam;
	ASSERT (CKexShlEx != NULL);

	if (Message == PSPCB_ADDREF) {
		CKexShlEx_AddRef(CKexShlEx);
	} else if (Message == PSPCB_RELEASE) {
		CKexShlEx_Release(CKexShlEx);
	}

	return TRUE;
}