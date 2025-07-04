///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllnotif.c
//
// Abstract:
//
//     Contains a routine which is called after a DLL is mapped into the
//     process, but before its imports are resolved. This is the opportune
//     moment to rewrite DLL imports.
//
// Author:
//
//     vxiiduu (18-Oct-2022)
//
// Revision History:
//
//     vxiiduu              18-Oct-2022  Initial creation.
//     vxiiduu              23-Feb-2024  Change wording from "loaded" to "mapped"
//                                       in order to better reflect reality.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

BOOL GetDllVersion(
	IN	PVOID	DllBase,
	IN	LANGID	LanguageId,
	OUT	PULONG	Major,
	OUT	PULONG	Minor,
	OUT	PULONG	Build,
	OUT	PULONG	Revision)
{
	NTSTATUS Status;
	LDR_RESOURCE_INFO ResourceInfo = {16, 1, LanguageId};
	PIMAGE_RESOURCE_DATA_ENTRY ResourceEntryPointer = NULL;
	PVOID ResourceDataPointer = NULL;
	ULONG ResourceSize = 0;
	VS_FIXEDFILEINFO *FixedInfo = NULL;
	ULONG Index;

	if (!DllBase) return FALSE;

	Status = LdrFindResource_U(
		DllBase,
		&ResourceInfo,
		3,
		&ResourceEntryPointer);
	if (!NT_SUCCESS(Status) || !ResourceEntryPointer) return FALSE;

	Status = LdrAccessResource(
		DllBase,
		ResourceEntryPointer,
		&ResourceDataPointer,
		&ResourceSize);
	if (!NT_SUCCESS(Status) || !ResourceDataPointer || ResourceSize == 0) return FALSE;

	for (Index = 0; Index < ResourceSize - sizeof(VS_FIXEDFILEINFO); ++Index) {
		VS_FIXEDFILEINFO *TempFixedInfo = (VS_FIXEDFILEINFO*)((PBYTE)ResourceDataPointer + Index);
		if (TempFixedInfo->dwSignature == 0xFEEF04BD) {
			FixedInfo = TempFixedInfo;
			break;
		}
	}
    if (!FixedInfo) return FALSE;
	
	*Major = HIWORD(FixedInfo->dwFileVersionMS);
	*Minor = LOWORD(FixedInfo->dwFileVersionMS);
	*Build = HIWORD(FixedInfo->dwFileVersionLS);
	*Revision = LOWORD(FixedInfo->dwFileVersionLS);
	return TRUE;
}
//
// This function is called within a try/except block inside NTDLL. So if we
// fuck up here, nothing super bad is going to happen, although of course it
// should still be avoided.
//
// NotificationData->Flags contains values starting with LDR_DLL_LOADED_FLAG_
// and it can (in Windows XP) only be 0 or 1. A value of 1 means that the DLL
// has been relocated. In Windows 7, NotificationData->Flags is always zero 
// and there is no information that can be gathered through this flag.
//
// This function is not called to notify for DLL unloads if the process is
// exiting. This is because, as an optimization, the LdrUnloadDll (XP) or 
// LdrpUnloadDll (Win7) routine does not actually unmap any DLLs when the
// process is exiting.
//
VOID NTAPI KexDllNotificationCallback(
	IN	LDR_DLL_NOTIFICATION_REASON	Reason,
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData,
	IN	PVOID						Context OPTIONAL)
{
	NTSTATUS Status;
	STATIC CONST PCWSTR ReasonToStringLookup[] = {
		L"(unknown)",
		L"mapped",
		L"unmapped",
		L"(unknown)"
	};
	
	if ((!wcsncmp(NotificationData->BaseDllName->Buffer, L"MacType.dll", MAX_PATH) || !wcsncmp(NotificationData->BaseDllName->Buffer, L"MacType64.dll", MAX_PATH)) && Reason == LDR_DLL_NOTIFICATION_REASON_LOADED) {
		ULONG Major, Minor, Build, Revision;
		BOOL ShowWarningDialog = TRUE;
		if (GetDllVersion(NotificationData->DllBase, 0, &Major, &Minor, &Build, &Revision)) {
			ShowWarningDialog = (Major == 1 && Minor > 2018 && (Minor < 2025 || (Minor == 2025 && Build < 609)));
		}
		if (ShowWarningDialog) {
			ULONG Response;
			PWCHAR ImageBaseName;
			LCID DefaultUILanguageId;
			PCWSTR FormattingMessage_ENG = L"Detected an old version of MacType is running. VxKex NEXT compatibility layer is enabled for this program (%s), enabling VxKex NEXT and older versions of MacType (versions between 2019.1 and 2025.4.11) at the same time may cause the program to crash. Updating MacType to version 2025.6.9 or later will resolve the issue."
				L"\n\n"
				L"It is recommended to terminate this process to prevent further program errors. Do you want to terminate this process immediately?";
			PCWSTR FormattingMessage_CHS = L"检测到旧版 MacType 正在运行。此程序（%s）已启用 VxKex NEXT 兼容层，VxKex NEXT 与旧版 MacType（2019.1 和 2025.4.11 之间的版本）同时启用可能导致程序崩溃，更新 MacType 至 2025.6.9 及以上版本即可解决此问题。"
				L"\n\n"
				L"建议结束此进程，以防止程序进一步的错误。是否想要立即结束此进程？";
			PCWSTR FormattingMessage_CHT = L"檢測到舊版 MacType 正在執行。此程式（%s）已啟用 VxKex NEXT 相容層，VxKex NEXT 與舊版 MacType（2019.1 和 2025.4.11 之間的版本）同時啟用可能導致程式當機，更新 MacType 至 2025.6.9 及以上版本即可解決此問題。"
				L"\n\n"
				L"建議結束此處理程序，以防止程式進一步的錯誤。是否想要立即結束此處理程序？";
			PCWSTR WarningTitle_ENG = L"VxKex NEXT Application Warning";
			PCWSTR WarningTitle_CHS = L"VxKex NEXT 应用程序警告";
			PCWSTR WarningTitle_CHT = L"VxKex NEXT 應用程式警告";
			PCWSTR FormattingMessage;
			PCWSTR WarningTitle;
			STRSAFE_LPWSTR Message;
			SIZE_T MessageLength;

			ImageBaseName = KexData->ImageBaseName.Buffer;
			NtQueryDefaultLocale(TRUE, &DefaultUILanguageId);
			switch (DefaultUILanguageId) {
				case MAKELANGID(LANG_CHINESE, SUBLANG_NEUTRAL):
				case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED):
				case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE):
					DefaultUILanguageId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
					break;
				case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL):
				case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_HONGKONG):
				case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_MACAU):
					DefaultUILanguageId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
					break;
				default:
					DefaultUILanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
					break;
			}
				
			FormattingMessage = ((DefaultUILanguageId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? FormattingMessage_CHS :
				(DefaultUILanguageId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? FormattingMessage_CHT :
				FormattingMessage_ENG);
			WarningTitle = ((DefaultUILanguageId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ? WarningTitle_CHS :
				(DefaultUILanguageId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ? WarningTitle_CHT :
				WarningTitle_ENG);
				
			MessageLength = wcslen(FormattingMessage) + wcslen(ImageBaseName) + 1;
			Message = SafeAlloc(WCHAR, MessageLength);
			StringCchPrintf(Message, MessageLength, FormattingMessage, ImageBaseName);
			Response = KexMessageBox(MB_ICONEXCLAMATION | MB_YESNO, WarningTitle, Message);
			SafeFree(Message);
			if (Response == 8) NtTerminateProcess(NtCurrentProcess(), STATUS_KEXDLL_INITIALIZATION_FAILURE);
		}
	}

	KexLogDetailEvent(
		L"The DLL %wZ was %s\r\n\r\n"
		L"Full path to the DLL: \"%wZ\"\r\n"
		L"Loaded at 0x%p (size: %I32u bytes)",
		NotificationData->BaseDllName,
		ARRAY_LOOKUP_BOUNDS_CHECKED(ReasonToStringLookup, Reason),
		NotificationData->FullDllName,
		NotificationData->DllBase,
		NotificationData->SizeOfImage);

	if (Reason == LDR_DLL_NOTIFICATION_REASON_LOADED) {
		BOOLEAN ShouldRewriteImports;

		ShouldRewriteImports = KexShouldRewriteImportsOfDll(
			NotificationData->FullDllName);

		unless (KexData->IfeoParameters.DisableAppSpecific) {
			if (ShouldRewriteImports) {
				if (!(KexData->Flags & KEXDATA_FLAG_CHROMIUM)) {
					//
					// APPSPECIFICHACK: Perform heuristic-based Chromium detection if we don't
					// already know that this is a Chromium process.
					//
					AshPerformChromiumDetectionFromLoadedDll(NotificationData);
				}
			}
		}

		if (ShouldRewriteImports) {
			Status = KexRewriteImageImportDirectory(
				NotificationData->DllBase,
				NotificationData->BaseDllName,
				NotificationData->FullDllName);
		}
	}
}