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
		ULONG Response;
		PWCHAR ImageBaseName;
		LCID DefaultUILanguageId;
		ImageBaseName = KexData->ImageBaseName.Buffer;
		NtQueryDefaultLocale(TRUE, &DefaultUILanguageId);
		switch (DefaultUILanguageId) {
			case MAKELANGID(LANG_CHINESE, SUBLANG_NEUTRAL):
			case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED):
			case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE):
				DefaultUILanguageId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
				break;
			default:
				DefaultUILanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
				break;
		}
		if (DefaultUILanguageId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
			WCHAR Message[3000];
			StringCchPrintf(Message, 3000, L"��⵽ MacType �������С��˳��������� VxKex ���ݲ㣬VxKex �� MacType ͬʱ���ÿ��ܵ��³��������"
				L"\n\n"
				L"ҪΪ�˽��̽��� MacType ����� MacType Wizard �����������̹�����ȡ����ѡ�����ظ�Ȩ�޽��̡����ڡ���������һ�����ҵ���%s����һ��λ���б�ײ������һ�����ѡ���ų��˽��̡������ý����������� MacType ����Ч��"
				L"\n\n"
				L"��������˽��̣��Է�ֹ�����һ���Ĵ����Ƿ���Ҫ���������˽��̣�", ImageBaseName);
			Response = KexMessageBox(MB_ICONEXCLAMATION | MB_YESNO, L"VxKex Ӧ�ó��򾯸�", Message);
		} else {
			Response = KexMessageBox(MB_ICONEXCLAMATION | MB_YESNO, L"VxKex Application Warning", L"VxKex could not start because MacType is running. Please quit MacType before running this program."
				L"\n"
				L"It is recommanded to terminate this process now.");
		}
		if (Response == 8) {
			NtTerminateProcess(NtCurrentProcess(), STATUS_KEXDLL_INITIALIZATION_FAILURE);
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