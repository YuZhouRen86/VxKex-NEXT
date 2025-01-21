///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     vxlsever.c
//
// Abstract:
//
//     Contains a routine to convert a VXLSEVERITY enumeration value into a
//     human-readable string.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu	            30-Sep-2022  Initial creation.
//     vxiiduu              12-Nov-2022  Move into KexDll
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

static PCWSTR SeverityLookupTable_ENG[][2] = {
	{L"Critical",		L"The application encountered a critical error and cannot continue"},
	{L"Error",			L"The application encountered a non-critical error which affected its function"},
	{L"Warning",		L"An event occurred which should be investigated but does not affect the immediate functioning of the application"},
	{L"Information",	L"An informational message which is not a cause for concern"},
	{L"Detail",			L"An informational message, generated in large quantities, which is not a cause for concern"},
	{L"Debug",			L"Information useful only to the developer of the application"},
	{L"Unknown",		L"An invalid or unknown severity value"}
};

static PCWSTR SeverityLookupTable_CHS[][2] = {
	{L"严重错误",		L"应用程序遇到严重错误，无法继续运行"},
	{L"错误",			L"应用程序遇到影响其功能的非严重错误"},
	{L"警告",			L"发生了一个应调查的事件，但不影响应用程序直接运行"},
	{L"信息",			L"信息性消息，无需担心"},
	{L"详细信息",		L"大量产生的信息性消息，无需担心"},
	{L"调试",			L"仅对应用程序开发人员有用的信息"},
	{L"未知",			L"无效或未知的严重程度值"}
};

//
// Convert a VXLSEVERITY enumeration value into a human-readable string.
// You can obtain a long description by passing TRUE for the LongDescription
// parameter.
//
KEXAPI PCWSTR NTAPI VxlSeverityToText(
	IN		VXLSEVERITY		Severity,
	IN		BOOLEAN			LongDescription)
{
	LCID DefaultUILanguageId;
    PCWSTR (*SeverityLookupTable)[2];
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
	if (DefaultUILanguageId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) SeverityLookupTable = SeverityLookupTable_CHS;
	else SeverityLookupTable = SeverityLookupTable_ENG;
	return SeverityLookupTable[min((ULONG) Severity, LogSeverityMaximumValue)][!!LongDescription];
}