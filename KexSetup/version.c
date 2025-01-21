///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     version.c
//
// Abstract:
//
//     This file contains functions which query the currently installed version
//     of VxKex.
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

// Returns 0 for not installed.
ULONG KexSetupGetInstalledVersion(
	VOID)
{
	HKEY KeyHandle;
	HKEY UninstallKeyHandle;
	ULONG InstalledVersion;

	KeyHandle = KxCfgOpenVxKexRegistryKey(FALSE, KEY_READ, NULL);
	UninstallKeyHandle = KxCfgpOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VxKex", KEY_READ, NULL);
	if (!(KeyHandle && UninstallKeyHandle)) {
		KeyHandle = KxCfgOpenLegacyVxKexRegistryKey(FALSE, KEY_READ, NULL);
		if (!(KeyHandle && UninstallKeyHandle)) {
			return 0;
		}
	}

	RegReadI32(KeyHandle, NULL, L"InstalledVersion", &InstalledVersion);
	SafeClose(KeyHandle);

	return InstalledVersion;
}