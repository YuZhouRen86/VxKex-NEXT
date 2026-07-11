///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ash.c
//
// Abstract:
//
//     This file contains helper routines for app-specific hacks.
//
// Author:
//
//     vxiiduu (16-Feb-2024)
//
// Environment:
//
//     Native mode
//
// Revision History:
//
//     vxiiduu              16-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// Statically linked Qt6 applications tend to have a .qtmimed PE section in their
// EXE file. We can check for this section to determine that an application has
// statically-linked Qt6.
//
BOOLEAN AshIsStaticallyLinkedQt6Image(
	IN	PVOID	ModuleBase)
{
	PIMAGE_NT_HEADERS NtHeaders;
	ANSI_STRING SectionName;
	PIMAGE_SECTION_HEADER SectionHeader;

	NtHeaders = RtlImageNtHeader(ModuleBase);
	ASSERT (NtHeaders != NULL);

	if (NtHeaders == NULL) {
		return FALSE;
	}

	RtlInitConstantAnsiString(&SectionName, ".qtmimed");

	SectionHeader = KexRtlSectionTableFromName(NtHeaders, &SectionName);

	return (SectionHeader != NULL) ? TRUE : FALSE;
}

//
// ExeName must include the .exe extension.
//
KEXAPI BOOLEAN NTAPI AshExeBaseNameIs(
	IN	PCWSTR	ExeName)
{
	NTSTATUS Status;
	UNICODE_STRING ExeNameUS;

	ASSERT (KexData != NULL);

	Status = RtlInitUnicodeStringEx(&ExeNameUS, ExeName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	return RtlEqualUnicodeString(&KexData->ImageBaseName, &ExeNameUS, TRUE);
}

STATIC NTSTATUS AshpGetFullAndBaseNameFromAddress(
	IN	PCVOID			AddressInsideModule,
	OUT	PUNICODE_STRING	FullDllName OPTIONAL,
	OUT	PUNICODE_STRING	BaseDllName OPTIONAL)
{
	NTSTATUS Status;
	UNICODE_STRING FullDllNameTemp;

	RtlInitEmptyUnicodeStringFromTeb(&FullDllNameTemp);

	if (!FullDllName && !BaseDllName) {
		// nothing to do
		return STATUS_SUCCESS;
	}

	//
	// Get full name.
	// This can fail if the address is outside any module, which is true for
	// JIT-generated code or obfuscated VM-protected code, for example.
	//

	Status = KexLdrGetDllFullNameFromAddress(
		AddressInsideModule,
		&FullDllNameTemp);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	if (FullDllName) {
		*FullDllName = FullDllNameTemp;
	}

	//
	// Get base name if necessary
	//

	if (BaseDllName) {
		Status = KexRtlPathFindFileName(&FullDllNameTemp, BaseDllName);
		ASSERT (NT_SUCCESS(Status));
	}

	return Status;
}

//
// This function is intended to be used like this:
//
//   if (AshModuleBaseNameIs(ReturnAddress(), L"kernel32.dll"))
//
// File extension (.dll, .exe etc.) is required.
//
KEXAPI BOOLEAN NTAPI AshModuleBaseNameIs(
	IN	PCVOID	AddressInsideModule,
	IN	PCWSTR	ModuleName)
{
	NTSTATUS Status;
	UNICODE_STRING BaseDllName;
	UNICODE_STRING ComparisonBaseName;

	Status = AshpGetFullAndBaseNameFromAddress(
		AddressInsideModule,
		NULL,
		&BaseDllName);

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	Status = RtlInitUnicodeStringEx(&ComparisonBaseName, ModuleName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	return RtlEqualUnicodeString(&BaseDllName, &ComparisonBaseName, TRUE);
}

//
// As with AshModuleBaseNameIs, this is designed to be used with the
// ReturnAddress() macro as the argument.
//
KEXAPI BOOLEAN NTAPI AshModuleIsWindowsModule(
	IN	PCVOID	AddressInsideModule)
{
	NTSTATUS Status;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;

	Status = AshpGetFullAndBaseNameFromAddress(
		AddressInsideModule,
		&FullDllName,
		&BaseDllName);

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	return KexIsWindowsDll(&FullDllName, &BaseDllName);
}

//
// Similar to AshModuleIsWindowsModule but also returns TRUE for any DLL
// which is banned from having its imports rewritten, which is related to but
// not exactly the same set of DLLs as "Windows modules".
//
KEXAPI BOOLEAN NTAPI AshModuleIsDynamicRewriteExemptedModule(
	IN	PCVOID	AddressInsideModule)
{
	NTSTATUS Status;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;

	Status = AshpGetFullAndBaseNameFromAddress(
		AddressInsideModule,
		&FullDllName,
		&BaseDllName);

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	return !KexShouldRewriteDynamicImportsOfDll(&FullDllName, &BaseDllName);
}

NTSTATUS AshSetIsQt6Process(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING Variable;
	UNICODE_STRING Value;
	SIZE_T VariableValueLength = 0;

	//
	// Users have reported that these variables can fix certain Qt6 programs
	// which make use of Qt Quick Scene Graph (QSG).
	// https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph-renderer.html
	// https://github.com/YuZhouRen86/VxKex-NEXT/issues/336
	// Affected apps: AmneziaVPN, GPT4All
	//

	RtlInitConstantUnicodeString(&Variable, L"QSG_RHI_BACKEND");
	RtlInitConstantUnicodeString(&Value, L"opengl");
		
	Status = RtlQueryEnvironmentVariable(NULL, Variable.Buffer, Variable.Length / sizeof(WCHAR), NULL, 0, &VariableValueLength);
	ASSERT (Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_VARIABLE_NOT_FOUND);

	if (VariableValueLength == 0) {
		Status = RtlSetEnvironmentVariable(NULL, &Variable, &Value);
		ASSERT (NT_SUCCESS(Status));
	}

	KexData->Flags |= KEXDATA_FLAG_QT6;
	return STATUS_SUCCESS;
}

VOID AshApplyQBittorrentEnvironmentVariableHacks(
	VOID)
{
	UNICODE_STRING VariableName;
	UNICODE_STRING VariableValue;

	ASSERT (AshExeBaseNameIs(L"qbittorrent.exe"));

	//
	// APPSPECIFICHACK: Applying the environment variable below will eliminate
	// the problem of bad kerning from qBittorrent. If more Qt apps are found
	// which have bad kerning, this may help fix those too.
	//

	KexLogInformationEvent(L"App-Specific Hack applied for qBittorrent");
	RtlInitConstantUnicodeString(&VariableName, L"QT_SCALE_FACTOR");
	RtlInitConstantUnicodeString(&VariableValue, L"1.0000001");
	RtlSetEnvironmentVariable(NULL, &VariableName, &VariableValue);
}

VOID AshApplyPythonEnvironmentVariableHacks(
	VOID)
{
	UNICODE_STRING VariableName;
	UNICODE_STRING VariableValue;

	ASSERT (StringBeginsWithI(KexData->ImageBaseName.Buffer, L"python"));

	if (LOWORD(OriginalBuildNumber) < 10586) {
		KexLogInformationEvent(L"App-Specific Hack applied for Python");
		RtlInitConstantUnicodeString(&VariableName, L"PYTHON_BASIC_REPL");
		RtlInitConstantUnicodeString(&VariableValue, L"1");
		RtlSetEnvironmentVariable(NULL, &VariableName, &VariableValue);
	}
}

VOID AshApplyNodeJSEnvironmentVariableHacks(
	VOID)
{
	UNICODE_STRING VariableName;
	UNICODE_STRING VariableValue;

	ASSERT (AshExeBaseNameIs(L"node.exe"));

	if (OriginalMajorVersion < 10) {
		KexLogInformationEvent(L"App-Specific Hack applied for Node.js");
		RtlInitConstantUnicodeString(&VariableName, L"NODE_SKIP_PLATFORM_CHECK");
		RtlInitConstantUnicodeString(&VariableValue, L"1");
		RtlSetEnvironmentVariable(NULL, &VariableName, &VariableValue);
	}
}