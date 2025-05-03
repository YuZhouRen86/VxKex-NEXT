///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     main.c
//
// Abstract:
//
//     KexSetup, the VxKex installer (and uninstaller) program.
//
// Author:
//
//     vxiiduu (31-Jan-2024)
//
// Environment:
//
//     Win32, without any vxkex support components
//
// Revision History:
//
//     vxiiduu               31-Jan-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#define NEED_VERSION_DEFS
#include "kexsetup.h"

LANGID CURRENTLANG = 0;
PWSTR FRIENDLYAPPNAME;

BOOLEAN Is64BitOS;
KEXSETUP_OPERATION_MODE OperationMode;
BOOLEAN SilentUnattended;
BOOLEAN PreserveConfig;
WCHAR KexDir[MAX_PATH];
ULONG ExistingVxKexVersion;
ULONG InstallerVxKexVersion;

VOID EntryPoint(
	VOID)
{
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

	KexgApplicationFriendlyName = FRIENDLYAPPNAME;
	ExistingVxKexVersion = KexSetupGetInstalledVersion();
	InstallerVxKexVersion = KEX_VERSION_DW;
	
	Is64BitOS = IsWow64();
	GetDefaultInstallationLocation(KexDir);
	ProcessCommandLineOptions();

	if (SilentUnattended) {
		KexSetupPerformActions();
	} else {
		DisplayInstallerGUI();
	}

	ExitProcess(STATUS_SUCCESS);
}