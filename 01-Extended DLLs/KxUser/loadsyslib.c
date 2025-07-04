#include "buildcfg.h"
#include "kxuserp.h"

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

HMODULE LoadSystemLibrary(
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