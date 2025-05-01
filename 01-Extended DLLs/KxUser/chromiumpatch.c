#include "buildcfg.h"
#include "kxuserp.h"
#include <ShellAPI.h>

PPWSTR Ext_CommandLineToArgvW(
	IN	LPCWSTR	lpCmdLine,
	OUT	PINT	pNumArgs)
{
	PPWSTR Result = CommandLineToArgvW(lpCmdLine, pNumArgs);
	
	unless (KexData->IfeoParameters.DisableAppSpecific){
		if ((KexData->Flags & KEXDATA_FLAG_CHROMIUM) && KexRtlCurrentProcessBitness() == 32 && Result && *pNumArgs >= 1) {
			ULONG Index;
			PCWSTR NoSandbox = L"--no-sandbox";
			PWSTR NoSandboxOption = NULL;
			PPWSTR NewArgv = (PPWSTR) GlobalAlloc(LMEM_FIXED, (*pNumArgs + 1) * sizeof(LPWSTR));

			if (!NewArgv) return Result;

			for (Index = 0; Index < (ULONG)*pNumArgs; Index++) {
				PWSTR CopiedText = (PWSTR) GlobalAlloc(LMEM_FIXED, (wcslen(Result[Index]) + 1) * sizeof(WCHAR));
				ZeroMemory(CopiedText, wcslen(Result[Index]) + 1);
				if (!CopiedText) {
					GlobalFree(NewArgv);
					return Result;
				}
				StringCchCopyW(CopiedText, (wcslen(Result[Index]) + 1), Result[Index]);
				NewArgv[Index] = CopiedText;
			}

			NoSandboxOption = (PWSTR) GlobalAlloc(LMEM_FIXED, (wcslen(NoSandbox) + 1) * sizeof(WCHAR));
			ZeroMemory(NoSandboxOption, wcslen(NoSandbox) + 1);
			if (!NoSandboxOption) {
				for (Index = 0; Index < (ULONG)*pNumArgs; Index++) {
					GlobalFree(NewArgv[Index]);
				}
				GlobalFree(NewArgv);
				return Result;
			}

			StringCchCopyW(NoSandboxOption, (wcslen(NoSandbox) + 1), NoSandbox);
			NewArgv[*pNumArgs] = NoSandboxOption;

			*pNumArgs += 1;
			GlobalFree(Result);
			Result = NewArgv;
		}
	}
	return Result;
}