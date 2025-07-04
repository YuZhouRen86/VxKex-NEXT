#include "buildcfg.h"
#include "kxbasep.h"
#include <KexStrSafe.h>

//
// Iertutil.dll contains a version check which will end up causing errors (failure
// to open files with ShellExecute including with shell context menus) if the reported
// OS version is higher than 6.1.
//
// This function un-spoofs the OS version for Windows DLLs, and preserves normal behavior
// for all other situations.
//

KXBASEAPI BOOL WINAPI Ext_GetVersionExA(
	IN OUT	POSVERSIONINFOA	VersionInfo)
{
	BOOL Success;

	Success = GetVersionExA(VersionInfo);

	if (Success && AshModuleIsWindowsModule(ReturnAddress())) {
		VersionInfo->dwMajorVersion = OriginalMajorVersion ? OriginalMajorVersion : 6;
		VersionInfo->dwMinorVersion = OriginalMinorVersion ? OriginalMinorVersion : 1;
		VersionInfo->dwBuildNumber = OriginalBuildNumber ? LOWORD(OriginalBuildNumber) : 7601;
	} else if (KexData->IfeoParameters.WinVerSpoof != WinVerSpoofNone) {
		switch (KexData->IfeoParameters.WinVerSpoof) {
		case WinVerSpoofWin7:
			VersionInfo->dwMajorVersion		= 6;
			VersionInfo->dwMinorVersion		= 1;
			VersionInfo->dwBuildNumber		= 7601;
			
			StringCchCopyA(
				VersionInfo->szCSDVersion,
				ARRAYSIZE(VersionInfo->szCSDVersion),
				"Service Pack 1");

			break;
		case WinVerSpoofWin8:
			VersionInfo->dwMajorVersion		= 6;
			VersionInfo->dwMinorVersion		= 2;
			VersionInfo->dwBuildNumber		= 9200;
			break;
		case WinVerSpoofWin8Point1:
			VersionInfo->dwMajorVersion		= 6;
			VersionInfo->dwMinorVersion		= 3;
			VersionInfo->dwBuildNumber		= 9600;
			break;
		case WinVerSpoofWin10:
			VersionInfo->dwMajorVersion		= 10;
			VersionInfo->dwMinorVersion		= 0;
			VersionInfo->dwBuildNumber		= 19045;
			break;
		case WinVerSpoofWin11:
		default:
			VersionInfo->dwMajorVersion		= 10;
			VersionInfo->dwMinorVersion		= 0;
			VersionInfo->dwBuildNumber		= 26100;
			break;
		}
	}

	return Success;
}

KXBASEAPI BOOL WINAPI Ext_GetVersionExW(
	IN OUT	POSVERSIONINFOW	VersionInfo)
{
	BOOL Success;

	Success = GetVersionExW(VersionInfo);

	if (KexData->IfeoParameters.WinVerSpoof != WinVerSpoofNone) {
		switch (KexData->IfeoParameters.WinVerSpoof) {
		case WinVerSpoofWin7:
			VersionInfo->dwMajorVersion		= 6;
			VersionInfo->dwMinorVersion		= 1;
			VersionInfo->dwBuildNumber		= 7601;
			
			StringCchCopy(
				VersionInfo->szCSDVersion,
				ARRAYSIZE(VersionInfo->szCSDVersion),
				L"Service Pack 1");

			break;
		case WinVerSpoofWin8:
			VersionInfo->dwMajorVersion		= 6;
			VersionInfo->dwMinorVersion		= 2;
			VersionInfo->dwBuildNumber		= 9200;
			break;
		case WinVerSpoofWin8Point1:
			VersionInfo->dwMajorVersion		= 6;
			VersionInfo->dwMinorVersion		= 3;
			VersionInfo->dwBuildNumber		= 9600;
			break;
		case WinVerSpoofWin10:
			VersionInfo->dwMajorVersion		= 10;
			VersionInfo->dwMinorVersion		= 0;
			VersionInfo->dwBuildNumber		= 19045;
			break;
		case WinVerSpoofWin11:
		default:
			VersionInfo->dwMajorVersion		= 10;
			VersionInfo->dwMinorVersion		= 0;
			VersionInfo->dwBuildNumber		= 26100;
			break;
		}
	}

	return Success;
}

KXBASEAPI BOOL WINAPI Ext_VerifyVersionInfoW(
	IN	LPOSVERSIONINFOEXW	lpVersionInformation,
	IN	DWORD				dwTypeMask,
	IN	DWORDLONG			dwlConditionMask)
{
	if (AshExeBaseNameIs(L"RobloxStudioBeta.exe")) return TRUE;
	return VerifyVersionInfoW(lpVersionInformation, dwTypeMask, dwlConditionMask);
}