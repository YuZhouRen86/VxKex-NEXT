#include "buildcfg.h"
#include "kxmip.h"

typedef UINT32 TBS_RESULT;

STATIC HMODULE OriginalLoadLibrary(
	LPCWSTR	FileName)
{
	PTEB Teb = NtCurrentTeb();
	BOOLEAN ReEntrant = Teb->KexLdrShouldRewriteDll;
	HMODULE Module;
	Teb->KexLdrShouldRewriteDll = FALSE;
	Module = LoadLibrary(FileName);
	Teb->KexLdrShouldRewriteDll = ReEntrant;
	return Module;
}

STATIC HMODULE LoadSystemLibrary(
	LPCWSTR	FileName)
{
	PWSTR SafePath;
	UINT SysDirLength = GetSystemDirectory(NULL, 0);
	SIZE_T SafePathLength;
	UINT GSDResult;
	HMODULE Module;
	if (SysDirLength == 0) return OriginalLoadLibrary(FileName);
	SafePathLength = SysDirLength + wcslen(FileName) + 1;
	SafePath = SafeAlloc(WCHAR, SafePathLength);
	if (!SafePath) return OriginalLoadLibrary(FileName);
	GSDResult = GetSystemDirectory(SafePath, SysDirLength);
	if (GSDResult == 0 || GSDResult >= SysDirLength) {
		SafeFree(SafePath);
		return OriginalLoadLibrary(FileName);
	}
	StringCchCat(SafePath, SafePathLength, L"\\");
	StringCchCat(SafePath, SafePathLength, FileName);
	Module = OriginalLoadLibrary(SafePath);
	SafeFree(SafePath);
	return Module;
}

KXMIAPI TBS_RESULT WINAPI Tbsi_GetDeviceInfo(
	IN	UINT32	Size,
	OUT	PVOID	Info)
{
	TBS_RESULT (WINAPI *pTbsi_GetDeviceInfo) (UINT32, PVOID);
	TBS_RESULT Result;
	HMODULE Tbs = LoadSystemLibrary(L"tbs.dll");
	pTbsi_GetDeviceInfo = (TBS_RESULT (WINAPI *) (UINT32, PVOID)) GetProcAddress(Tbs, "Tbsi_GetDeviceInfo");
	if (pTbsi_GetDeviceInfo) {
		Result = pTbsi_GetDeviceInfo(Size, Info);
	} else {
		if (Size == 0 || !Info) Result = TBS_E_BAD_PARAMETER;
		else Result = TBS_E_TPM_NOT_FOUND;
	}
	FreeLibrary(Tbs);
	return Result;
}