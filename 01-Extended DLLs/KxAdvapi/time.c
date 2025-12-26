#include "buildcfg.h"
#include <KexComm.h>

DWORD WINAPI EnumDynamicTimeZoneInformation(
	IN	const DWORD						Index,
	OUT	PDYNAMIC_TIME_ZONE_INFORMATION	TimeZoneInformation)
{
	LSTATUS LStatus;
	DYNAMIC_TIME_ZONE_INFORMATION DynamicTimeZoneInformation;
	HKEY TimeZoneRootKey = NULL;
	DWORD TZKeyNameLength = ARRAYSIZE(TimeZoneInformation->TimeZoneKeyName);

	if (!TimeZoneInformation) return ERROR_INVALID_PARAMETER;
	
	LStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones", 0, KEY_READ, &TimeZoneRootKey);
	if (LStatus != ERROR_SUCCESS) goto Exit;

	LStatus = RegEnumKeyEx(TimeZoneRootKey, Index, DynamicTimeZoneInformation.TimeZoneKeyName, &TZKeyNameLength, NULL, NULL, NULL, NULL);
	if (LStatus != ERROR_SUCCESS) goto Exit;

	DynamicTimeZoneInformation.DynamicDaylightTimeDisabled = TRUE;
	if (!GetTimeZoneInformationForYear(0, &DynamicTimeZoneInformation, (PTIME_ZONE_INFORMATION)TimeZoneInformation)) {
		LStatus = GetLastError();
		goto Exit;
	}

	StringCchCopy(TimeZoneInformation->TimeZoneKeyName, TZKeyNameLength, DynamicTimeZoneInformation.TimeZoneKeyName);
	TimeZoneInformation->DynamicDaylightTimeDisabled = FALSE;

Exit:
	if (TimeZoneRootKey) RegCloseKey(TimeZoneRootKey);
	return LStatus;
}

DWORD WINAPI GetDynamicTimeZoneInformationEffectiveYears(
	IN	const PDYNAMIC_TIME_ZONE_INFORMATION	TimeZoneInformation,
	OUT	LPDWORD									FirstYear,
	OUT	LPDWORD									LastYear)
{
	LSTATUS LStatus;
	HKEY TimeZoneRootKey = NULL;
	HKEY TimeZoneKey = NULL;
	HKEY DynamicDSTKey = NULL;
	DWORD FirstEntry = 0, LastEntry = 0;
	DWORD cbData;

	if (!TimeZoneInformation) return ERROR_INVALID_PARAMETER;

	LStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones", 0, KEY_READ, &TimeZoneRootKey);
	if (LStatus != ERROR_SUCCESS) goto Exit;

	LStatus = RegOpenKeyEx(TimeZoneRootKey, TimeZoneInformation->TimeZoneKeyName, 0, KEY_READ, &TimeZoneKey);
	if (LStatus != ERROR_SUCCESS) goto Exit;

	LStatus = RegOpenKeyEx(TimeZoneKey, L"Dynamic DST", 0, KEY_READ, &DynamicDSTKey);
	if (LStatus != ERROR_SUCCESS) goto Exit;

	cbData = sizeof(FirstEntry);
	LStatus = RegQueryValueEx(DynamicDSTKey, L"FirstEntry", NULL, NULL, (LPBYTE)&FirstEntry, &cbData);
	if (LStatus != ERROR_SUCCESS) goto Exit;

	cbData = sizeof(LastEntry);
	LStatus = RegQueryValueEx(DynamicDSTKey, L"LastEntry", NULL, NULL, (LPBYTE)&LastEntry, &cbData);
	if (LStatus != ERROR_SUCCESS) goto Exit;

	*FirstYear = FirstEntry;
	*LastYear = LastEntry;

Exit:
	if (DynamicDSTKey) RegCloseKey(DynamicDSTKey);
	if (TimeZoneKey) RegCloseKey(TimeZoneKey);
	if (TimeZoneRootKey) RegCloseKey(TimeZoneRootKey);

	return LStatus;
}