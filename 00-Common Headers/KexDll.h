///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexDll.h
//
// Abstract:
//
//     VxKex base API
//
// Author:
//
//     vxiiduu (11-Oct-2022)
//
// Revision History:
//
//     vxiiduu               11-Oct-2022  Initial creation.
//     vxiiduu               06-Nov-2022  Refactor and create KexLdr* section
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <KexComm.h>
#include <KexSmp.h>

#ifndef KEXAPI
#  ifndef NO_KEXDLL_LIB
#    pragma comment(lib, "KexDll.lib")
#  endif
#  define KEXAPI DECLSPEC_IMPORT
#endif

#include <KexMls.h>

#ifndef KEX_COMPONENT
#  error You must define a Unicode component name as the KEX_COMPONENT macro.
#endif

#pragma region Macros and Structure Definitions

//
// Uncomment the macro definition to keep debug logging enabled in release
// builds.
//
// Note that this only affects debug logs made through the KexLogDebugEvent
// macro.
//
//#define RELEASE_DEBUGLOGS_ENABLED

#define REG_RESTRICT_NONE						(1 << REG_NONE)
#define REG_RESTRICT_SZ							(1 << REG_SZ)
#define REG_RESTRICT_EXPAND_SZ					(1 << REG_EXPAND_SZ)
#define REG_RESTRICT_BINARY						(1 << REG_BINARY)
#define REG_RESTRICT_DWORD						(1 << REG_DWORD)
#define REG_RESTRICT_DWORD_BE					(1 << REG_DWORD_BIG_ENDIAN)
#define REG_RESTRICT_LINK						(1 << REG_LINK)
#define REG_RESTRICT_MULTI_SZ					(1 << REG_MULTI_SZ)
#define REG_RESTRICT_RESOURCE_LIST				(1 << REG_RESOURCE_LIST)
#define REG_RESTRICT_FULL_RESOURCE_DESCRIPTOR	(1 << REG_FULL_RESOURCE_DESCRIPTOR)
#define REG_RESTRICT_RESOURCE_REQUIREMENTS_LIST	(1 << REG_RESOURCE_REQUIREMENTS_LIST)
#define REG_RESTRICT_QWORD						(1 << REG_QWORD)

#define LEGAL_REG_RESTRICT_MASK \
	(REG_RESTRICT_NONE | \
	 REG_RESTRICT_SZ | \
	 REG_RESTRICT_EXPAND_SZ | \
	 REG_RESTRICT_BINARY | \
	 REG_RESTRICT_DWORD | \
	 REG_RESTRICT_DWORD_BE | \
	 REG_RESTRICT_LINK | \
	 REG_RESTRICT_MULTI_SZ | \
	 REG_RESTRICT_RESOURCE_LIST | \
	 REG_RESTRICT_FULL_RESOURCE_DESCRIPTOR | \
	 REG_RESTRICT_RESOURCE_REQUIREMENTS_LIST | \
	 REG_RESTRICT_QWORD)

#define REG_RESTRICT_ANY LEGAL_REG_RESTRICT_MASK

#define QUERY_KEY_MULTIPLE_VALUE_FAIL_FAST 1

#define QUERY_KEY_MULTIPLE_VALUE_VALID_MASK \
	(QUERY_KEY_MULTIPLE_VALUE_FAIL_FAST)

#define KEXDATA_FLAG_PROPAGATED				1	// Indicates that this process was spawned from a VxKex-enabled parent
#define KEXDATA_FLAG_IFEO_OPTIONS_PRESENT	2	// Indicates that this process has VxKex options set in IFEO
#define KEXDATA_FLAG_MSIEXEC				4	// Indicates that this process is %SystemRoot%\system32\msiexec.exe
#define KEXDATA_FLAG_ENABLED_FOR_MSI		8	// Indicates that this MSI has VxKex enabled.
#define KEXDATA_FLAG_DISABLE_LOGGING		16	// Log files are not to be created.
#define KEXDATA_FLAG_CHROMIUM				32	// This is a Chromium-based application (Chrome, Edge, Electron, QtWebEngine, etc.)
#define KEXDATA_FLAG_KB2533623_PRESENT		64	// Indicates the DllDirectory APIs are available
#define KEXDATA_FLAG_FIREFOX				128	// This is a Firefox-based application (Firefox, Thunderbird, etc.)
#define KEXDATA_FLAG_DOTNET					256	// Indicates a .NET application.
#define KEXDATA_FLAG_QT6					512 // Indicates a Qt6 application.
#define KEXDATA_FLAG_MSI_SERVICE			1024	// Indicates MSIEXEC /V (Windows Installer service)
#define KEXDATA_FLAG_CONDRV_EMULATION		2048	// ConDrv emulation is enabled (see ntcondrv.c)

#define KEX_STRONGSPOOF_SHAREDUSERDATA	1
#define KEX_STRONGSPOOF_REGISTRY		2
#define KEX_STRONGSPOOF_VALID_MASK		(KEX_STRONGSPOOF_SHAREDUSERDATA | KEX_STRONGSPOOF_REGISTRY)

#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR		0x00000100
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR		0x00000200
#define LOAD_LIBRARY_SEARCH_USER_DIRS			0x00000400
#define LOAD_LIBRARY_SEARCH_SYSTEM32			0x00000800
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS		0x00001000
#define LOAD_LIBRARY_SAFE_CURRENT_DIRS			0x00002000

#define LOAD_LIBRARY_ALL_DLLDIR_FLAGS			(LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | \
												 LOAD_LIBRARY_SEARCH_APPLICATION_DIR | \
												 LOAD_LIBRARY_SEARCH_USER_DIRS | \
												 LOAD_LIBRARY_SEARCH_SYSTEM32 | \
												 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)

typedef struct _KEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY {
	IN		CONST UNICODE_STRING			ValueName;
	OUT		NTSTATUS						Status;
	IN OUT	ULONG							ValueDataCb;

	//
	// If you don't want to store data outside the structure, and if
	// you are querying value data that you know should be 8 bytes or
	// less, then you can set ValueData to point to ValueDataRawBytes.
	// Make sure you set ValueDataCb and ValueDataTypeRestrict as
	// appropriate.
	//
	union {
		OUT		PVOID							ValueData;
		OUT		ULONG							ValueDataAsDword;
		OUT		ULONGLONG						ValueDataAsQword;
		OUT		BYTE							ValueDataRawBytes[8];
	};

	IN		ULONG							ValueDataTypeRestrict;
	OUT		ULONG							ValueDataType;
} TYPEDEF_TYPE_NAME(KEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY);

#define VXLL_VERSION 1

typedef enum _VXLLOGINFOCLASS {
	LogLibraryVersion,
	LogNumberOfCriticalEvents,
	LogNumberOfErrorEvents,
	LogNumberOfWarningEvents,
	LogNumberOfInformationEvents,
	LogNumberOfDetailEvents,
	LogNumberOfDebugEvents,
	LogTotalNumberOfEvents,
	LogSourceApplication,
	MaxLogInfoClass
} VXLLOGINFOCLASS;

typedef enum _VXLSEVERITY {
	LogSeverityInvalidValue = -1,
	LogSeverityCritical,
	LogSeverityError,
	LogSeverityWarning,
	LogSeverityInformation,
	LogSeverityDetail,
	LogSeverityDebug,
	LogSeverityMaximumValue
} VXLSEVERITY;

// All UNICODE_STRINGs in VXLLOGENTRY are guaranteed to be null terminated.
// So you can pass the buffers directly to Win32 functions.
typedef struct _VXLLOGENTRY {
	UNICODE_STRING			TextHeader;
	UNICODE_STRING			Text;
	UCHAR					SourceComponentIndex;
	UCHAR					SourceFileIndex;
	UCHAR					SourceFunctionIndex;
	ULONG					SourceLine;
	CLIENT_ID				ClientId;
	VXLSEVERITY				Severity;
	SYSTEMTIME				Time;
} TYPEDEF_TYPE_NAME(VXLLOGENTRY);

typedef struct _VXLLOGFILEHEADER {
	CHAR		Magic[4];
	ULONG		Version;
	ULONG		EventSeverityTypeCount[LogSeverityMaximumValue];
	WCHAR		SourceApplication[32];
	WCHAR		SourceComponents[64][16];
	WCHAR		SourceFiles[128][16];
	WCHAR		SourceFunctions[256][64];
	BOOLEAN		Dirty;
} TYPEDEF_TYPE_NAME(VXLLOGFILEHEADER);

typedef struct _VXLLOGFILEENTRY {
	union {
		FILETIME	Time;
		LONGLONG	Time64;
	};

	// Do not directly use CLIENT_ID here since its size varies with
	// bitness. (contains HANDLE members)
	ULONG		ProcessId;
	ULONG		ThreadId;

	VXLSEVERITY	Severity;
	ULONG		SourceLine;
	UCHAR		SourceComponentIndex;
	UCHAR		SourceFileIndex;
	UCHAR		SourceFunctionIndex;

	USHORT		TextHeaderCch;
	USHORT		TextCch;

	WCHAR		Text[];
} TYPEDEF_TYPE_NAME(VXLLOGFILEENTRY);

// index cache (EntryIndexToFileOffset) makes reading and sorting the
// log file faster. Without it, writing the log file is very fast but
// read and export performance is unacceptably bad.
typedef struct _VXLCONTEXT {
	RTL_SRWLOCK				Lock;
	HANDLE					FileHandle;
	PULONG					EntryIndexToFileOffset;	// only populated when file is opened for READ ONLY, otherwise NULL

	union {
		PVXLLOGFILEHEADER		Header;
		PBYTE					MappedFile;			// only populated in READ ONLY mode, otherwise NULL
		PVOID					MappedSection;		// ^
	};

	ULONG					OpenMode;				// GENERIC_READ or GENERIC_WRITE
} TYPEDEF_TYPE_NAME(VXLCONTEXT);

typedef PVXLCONTEXT TYPEDEF_TYPE_NAME(VXLHANDLE);

typedef enum _KEX_WIN_VER_SPOOF {
	WinVerSpoofNone,	// do not spoof
	WinVerSpoofWin7,	// Win7 SP1
	WinVerSpoofWin8,
	WinVerSpoofWin8Point1,
	WinVerSpoofWin10,
	WinVerSpoofWin11,
	WinVerSpoofMax		// should always be the last value
} TYPEDEF_TYPE_NAME(KEX_WIN_VER_SPOOF);

//
// These variable names are present under the IFEO key for each executable
// with a KEX_ prefix. For example:
//
//   HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\
//   Image File Execution Options\application.exe\{id}\KEX_WinVerSpoof
//
// The KEX_IFEO_PARAMETERS structure may never contain members whose size
// varies by processor architecture, because it may be copied verbatim between
// processes during propagation. (See propagte.c.)
//

typedef struct _KEX_IFEO_PARAMETERS {
	ULONG						DisableForChild;
	ULONG						DisableAppSpecific;
	KEX_WIN_VER_SPOOF			WinVerSpoof;
	ULONG						StrongVersionSpoof;				// KEX_STRONGSPOOF_*
	ULONG						DisableConsoleEnhancements;		// boolean
	ULONG						TlsForceEnabledProtocols;		// SP_PROT_*
	ULONG						TlsForceDisabledProtocols;		// SP_PROT_*
} TYPEDEF_TYPE_NAME(KEX_IFEO_PARAMETERS);

//
// A KEX_PROCESS_DATA structure for the current process can be obtained
// outside of KexDll by calling the exported function KexDataInitialize.
//

typedef struct _KEX_PROCESS_DATA {
	ULONG					Flags;						// KEXDATA_FLAG_*
	KEX_IFEO_PARAMETERS		IfeoParameters;
	UNICODE_STRING			WinDir;						// e.g. "C:\Windows"
	UNICODE_STRING			KexDir;						// e.g. "C:\Program Files\VxKex"
	UNICODE_STRING			LogDir;
	UNICODE_STRING			Kex3264DirPath;				// e.g. "C:\Program Files\VxKex\Kex64"
	UNICODE_STRING			ImageBaseName;				// e.g. program.exe
	VXLHANDLE				LogHandle;
	PVOID					KexDllBase;
	PVOID					SystemDllBase;				// NTDLL base address
	PVOID					NativeSystemDllBase;
	PVOID					BaseDllBase;				// Kernel32 base address
	HANDLE					BaseNamedObjects;			// object directory handle
	HANDLE					UntrustedNamedObjects;
	HANDLE					KsecDD;						// handle to \Device\KsecDD
	HANDLE					GlobalKeyedEvent;
} TYPEDEF_TYPE_NAME(KEX_PROCESS_DATA);

typedef PVOID TYPEDEF_TYPE_NAME(DLL_DIRECTORY_COOKIE);

//
// A KEX_TEB_EXTENSION structure is located directly after the TEB, in free memory.
// The structure size is quite limited. On 32-bit, the structure is limited to a
// size of only 28 bytes. On 64-bit we have a lot more space (2024 bytes).
// The structure is always zero-initialized by the kernel upon thread start.
//
// Use the inline function KexCurrentTebExtension() to obtain a pointer to the
// KEX_TEB_EXTENSION of the current thread.
//

typedef struct _KEX_TEB_EXTENSION *PKEX_TEB_EXTENSION;

typedef struct _KEX_TEB_EXTENSION {
	// This member is used for supporting NtWaitForAlertByThreadId and
	// NtAlertThreadByThreadId (ntalrtid.c).
	VOLATILE LONG				AlertByThreadIdState;

	union {
		struct {
			BOOLEAN				KexLdrShouldRewriteDll:1;
		};

		UCHAR					BitField;
	};
} TYPEDEF_TYPE_NAME(KEX_TEB_EXTENSION);

// Make sure no overflow of the TEB pages occurs.
C_ASSERT(sizeof(TEB) + sizeof(KEX_TEB_EXTENSION) <= 
		 ((sizeof(TEB) + (PAGE_SIZE - 1)) & ~((SIZE_T) (PAGE_SIZE - 1))));

#pragma endregion

#pragma region Nt* functions

KEXAPI NTSTATUS NTAPI NtCompareObjects(
	IN	HANDLE	FirstObjectHandle,
	IN	HANDLE	SecondObjectHandle);

#pragma endregion

#pragma region Kex* functions

KEXAPI NTSTATUS NTAPI KexInitializePropagation(
	VOID);

KEXAPI NTSTATUS NTAPI KexDataInitialize(
	OUT	PPKEX_PROCESS_DATA	KexDataOut OPTIONAL);

#pragma endregion

#pragma region KexRtl* functions

KEXAPI INT NTAPI KexRtlOperatingSystemBitness(
	VOID);

KEXAPI VOID NTAPI KexRtlGetNtVersionNumbers(
	OUT	PULONG	MajorVersion OPTIONAL,
	OUT	PULONG	MinorVersion OPTIONAL,
	OUT	PULONG	BuildNumber OPTIONAL);

KEXAPI NTSTATUS NTAPI KexRtlPathFindFileName(
	IN	PCUNICODE_STRING Path,
	OUT	PUNICODE_STRING FileName);

KEXAPI NTSTATUS NTAPI KexRtlPathRemoveExtension(
	IN	PCUNICODE_STRING	Path,
	OUT	PUNICODE_STRING		PathWithoutExtension);

KEXAPI BOOLEAN NTAPI KexRtlPathReplaceIllegalCharacters(
	IN OUT	PUNICODE_STRING		Path,
	IN		WCHAR				ReplacementCharacter OPTIONAL,
	IN		BOOLEAN				AllowPathSeparators);

KEXAPI NTSTATUS NTAPI KexRtlGetProcessImageBaseName(
	OUT	PUNICODE_STRING	FileName);

KEXAPI NTSTATUS NTAPI KexRtlQueryKeyValueData(
	IN		HANDLE				KeyHandle,
	IN		PCUNICODE_STRING	ValueName,
	IN OUT	PULONG				ValueDataCb,
	OUT		PVOID				ValueData OPTIONAL,
	IN		ULONG				ValueDataTypeRestrict,
	OUT		PULONG				ValueDataType OPTIONAL);

KEXAPI NTSTATUS NTAPI KexRtlQueryKeyMultipleValueData(
	IN		HANDLE												KeyHandle,
	IN		PKEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY	QueryTable,
	IN OUT	PULONG												NumberOfQueryTableElements,
	IN		ULONG												Flags);

KEXAPI BOOLEAN NTAPI KexRtlUnicodeStringEndsWith(
	IN	PCUNICODE_STRING	String,
	IN	PCUNICODE_STRING	EndsWith,
	IN	BOOLEAN				CaseInsensitive);

KEXAPI PWCHAR NTAPI KexRtlFindUnicodeSubstring(
	PCUNICODE_STRING	Haystack,
	PCUNICODE_STRING	Needle,
	BOOLEAN				CaseInsensitive);

KEXAPI VOID NTAPI KexRtlAdvanceUnicodeString(
	OUT	PUNICODE_STRING	String,
	IN	USHORT			AdvanceCb);

KEXAPI VOID NTAPI KexRtlRetreatUnicodeString(
	OUT	PUNICODE_STRING	String,
	IN	USHORT			RetreatCb);

KEXAPI NTSTATUS NTAPI KexRtlSetUnicodeStringBufferEnd(
	OUT	PUNICODE_STRING	String,
	IN	PCWCHAR			NewEnd);

KEXAPI NTSTATUS NTAPI KexRtlShiftUnicodeString(
	IN OUT	PUNICODE_STRING	String,
	IN		USHORT			ShiftCch,
	IN		WCHAR			LeftFillCharacter OPTIONAL);

KEXAPI ULONG NTAPI KexRtlRemoteProcessBitness(
	IN	HANDLE	ProcessHandle);

KEXAPI NTSTATUS NTAPI KexRtlWriteProcessMemory(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	Destination,
	IN	PVOID		Source,
	IN	SIZE_T		Cb);

KEXAPI NTSTATUS NTAPI KexRtlCreateDirectoryRecursive(
	OUT	PHANDLE				DirectoryHandle,
	IN	ACCESS_MASK			DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	ULONG				ShareAccess);

KEXAPI PCWSTR NTAPI KexRtlNtStatusToString(
	IN	NTSTATUS	Status);

KEXAPI PIMAGE_SECTION_HEADER NTAPI KexRtlSectionTableFromRva(
	IN	PIMAGE_NT_HEADERS	NtHeaders,
	IN	ULONG				ImageRva);

KEXAPI PIMAGE_SECTION_HEADER NTAPI KexRtlSectionTableFromName(
	IN	PIMAGE_NT_HEADERS	NtHeaders,
	IN	PCANSI_STRING		SectionName);

KEXAPI NTSTATUS NTAPI KexRtlNullTerminateUnicodeString(
	IN	PUNICODE_STRING	String);

KEXAPI BOOLEAN NTAPI KexRtlUnicodeStringContainsEmbeddedNull(
	IN	PUNICODE_STRING	String);

KEXAPI VOID NTAPI KexRtlWakeAddressSingle(
	IN	PVOID			Address);

KEXAPI NTSTATUS NTAPI KexRtlWaitOnAddress(
	IN	volatile VOID	*Address,
	IN	PVOID			CompareAddress,
	IN	SIZE_T			AddressSize,
	IN	PLARGE_INTEGER	Timeout OPTIONAL);

KEXAPI VOID NTAPI KexRtlWakeByAddressSingle(
	IN	PVOID			Address);

KEXAPI VOID NTAPI KexRtlWakeByAddressAll(
	IN	PVOID			Address);

KEXAPI NTSTATUS NTAPI KexRtlWow64GetProcessMachines(
	IN	HANDLE	ProcessHandle,
	OUT	PUSHORT	ProcessMachine,
	OUT	PUSHORT	NativeMachine OPTIONAL);

KEXAPI NTSTATUS NTAPI KexRtlWow64IsWowGuestMachineSupported(
	IN	USHORT		WowGuestMachine,
	OUT	PBOOLEAN	MachineIsSupported);

KEXAPI VOID NTAPI KexRtlSetBit(
	IN	PRTL_BITMAP	BitmapHeader,
	IN	ULONG		BitNumber);

KEXAPI VOID NTAPI KexRtlClearBit(
	IN	PRTL_BITMAP	BitmapHeader,
	IN	ULONG		BitNumber);

KEXAPI NTSTATUS NTAPI KexRtlCreateUntrustedDirectoryObject(
	OUT	PHANDLE				DirectoryHandle,
	IN	ACCESS_MASK			DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes);

KEXAPI NTSTATUS NTAPI KexRtlGenerateRandomData(
	OUT	PVOID	RandomBuffer,
	IN	ULONG	NumberOfBytesToGenerate);

KEXAPI NTSTATUS NTAPI KexRtlDecryptMemory(
	IN OUT	PVOID	Memory,
	IN		ULONG	MemoryCb,
	IN		ULONG	Flags);

KEXAPI NTSTATUS NTAPI KexRtlEncryptMemory(
	IN OUT	PVOID	Memory,
	IN		ULONG	MemoryCb,
	IN		ULONG	Flags);

KEXAPI LONGLONG NTAPI KexRtlGetSystemTimePrecise(
	VOID);

#ifdef KEX_ARCH_X64
#  define KexRtlCurrentProcessBitness() (64)
#else
#  define KexRtlCurrentProcessBitness() (32)
#endif

#define KexRtlUpdateNullTerminatedUnicodeStringLength(UnicodeString) ((UnicodeString)->Length = (USHORT) (wcslen((UnicodeString)->Buffer) << 1))
#define KexRtlUpdateNullTerminatedAnsiStringLength(AnsiString) ((AnsiString)->Length = (USHORT) wcslen((AnsiString)->Buffer))
#define KexRtlUnicodeStringCch(UnicodeString) ((UnicodeString)->Length / sizeof(WCHAR))
#define KexRtlUnicodeStringBufferCch(UnicodeString) ((UnicodeString)->MaximumLength / sizeof(WCHAR))
#define KexRtlAnsiStringCch(AnsiString) ((AnsiString)->Length)
#define KexRtlAnsiStringBufferCch(AnsiString) ((AnsiString)->MaximumLength)
#define KexRtlEndOfUnicodeString(UnicodeString) ((UnicodeString)->Buffer + KexRtlUnicodeStringCch(UnicodeString))
#define KexRtlCopyMemory(Destination, Source, Cb) __movsb((PUCHAR) (Destination), (PUCHAR) (Source), (Cb))
#define KexRtlFillMemory(Destination, Length, Fill) __stosb((PUCHAR) (Destination), (Fill), (Length))
#define KexRtlZeroMemory(Destination, Length) KexRtlFillMemory((Destination), (Length), 0)

#define ForEachArrayItem(Array, Index) for (Index = 0; Index < ARRAYSIZE(Array); ++Index)

#pragma endregion

#pragma region KexLdr* functions

KEXAPI PVOID NTAPI KexLdrGetSystemDllBase(
	VOID);

KEXAPI PVOID NTAPI KexLdrGetRemoteSystemDllBase(
	IN	HANDLE	ProcessHandle);

KEXAPI PVOID NTAPI KexLdrGetNativeSystemDllBase(
	VOID);

KEXAPI NTSTATUS NTAPI KexLdrMiniGetProcedureAddress(
	IN	PVOID	DllBase,
	IN	PCSTR	ProcedureName,
	OUT	PPVOID	ProcedureAddress);

KEXAPI NTSTATUS NTAPI KexLdrFindDllInitRoutine(
	IN	PVOID	DllBase,
	OUT	PPVOID	InitRoutine);

NTSTATUS NTAPI KexLdrGetDllFullName(
	IN	PVOID			DllBase OPTIONAL,
	OUT	PUNICODE_STRING	DllFullPath);

KEXAPI NTSTATUS NTAPI KexLdrGetDllFullNameFromAddress(
	IN	PCVOID			Address,
	OUT	PUNICODE_STRING	DllFullPath);

KEXAPI NTSTATUS NTAPI KexLdrGetImageImportSection(
	IN	PVOID	ImageBase,
	OUT	PPVOID	ImportSectionBase,
	OUT	PSIZE_T	ImportSectionSize);

#pragma endregion

#pragma region KexHk* functions

#ifdef KEX_ARCH_X64
#  define BASIC_HOOK_LENGTH 14
#  define BASIC_HOOK_DESTINATION_OFFSET 6
#else
#  define BASIC_HOOK_LENGTH 6
#  define BASIC_HOOK_DESTINATION_OFFSET 1
#endif

typedef struct _KEX_BASIC_HOOK_CONTEXT {
	PVOID OriginalApiAddress;
	BYTE OriginalInstructions[BASIC_HOOK_LENGTH];
} TYPEDEF_TYPE_NAME(KEX_BASIC_HOOK_CONTEXT);

KEXAPI NTSTATUS NTAPI KexHkInstallHook(
	IN		HANDLE					ProcessHandle,
	IN		PVOID					ApiAddress,
	IN		PVOID					RedirectedAddress,
	OUT		PKEX_BASIC_HOOK_CONTEXT	HookContext OPTIONAL);

KEXAPI NTSTATUS NTAPI KexHkInstallBasicHook(
	IN		PVOID					ApiAddress,
	IN		PVOID					RedirectedAddress,
	OUT		PKEX_BASIC_HOOK_CONTEXT	HookContext OPTIONAL);

KEXAPI NTSTATUS NTAPI KexHkRemoveHook(
	IN		HANDLE					ProcessHandle,
	IN		PKEX_BASIC_HOOK_CONTEXT	HookContext);

KEXAPI NTSTATUS NTAPI KexHkRemoveBasicHook(
	IN		PKEX_BASIC_HOOK_CONTEXT	HookContext);

#pragma endregion

#pragma region Ash* functions

KEXAPI BOOLEAN NTAPI AshExeBaseNameIs(
	IN	PCWSTR	ExeName);

KEXAPI BOOLEAN NTAPI AshModuleBaseNameIs(
	IN	PCVOID	AddressInsideModule,
	IN	PCWSTR	ModuleName);

KEXAPI BOOLEAN NTAPI AshModuleIsWindowsModule(
	IN	PVOID	AddressInsideModule);

KEXAPI BOOLEAN NTAPI AshModuleIsDynamicRewriteExemptedModule(
	IN	PCVOID	AddressInsideModule);

#pragma endregion

#pragma region Vxl* functions

//
// vxlopcl.c
//

KEXAPI NTSTATUS NTAPI VxlOpenLog(
	OUT		PVXLHANDLE			LogHandle,
	IN		PUNICODE_STRING		SourceApplication OPTIONAL,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	IN		ACCESS_MASK			DesiredAccess,
	IN		ULONG				CreateDisposition);

KEXAPI NTSTATUS NTAPI VxlCloseLog(
	IN OUT	PVXLHANDLE		LogHandle);

//
// vxlquery.c
//

KEXAPI NTSTATUS NTAPI VxlQueryInformationLog(
	IN		VXLHANDLE		LogHandle,
	IN		VXLLOGINFOCLASS	LogInformationClass,
	OUT		PVOID			Buffer OPTIONAL,
	IN OUT	PULONG			BufferSize);

//
// vxlwrite.c
//

#define VxlWriteLog(LogHandle, SourceComponent, Severity, ...) \
	VxlWriteLogEx( \
		LogHandle, \
		SourceComponent, \
		__FILEW__, \
		__LINE__, \
		__FUNCTIONW__, \
		Severity, \
		__VA_ARGS__)

#define KexLogEvent(Severity, ...) \
	VxlWriteLog(KexData->LogHandle, KEX_COMPONENT, Severity, __VA_ARGS__)

#define KexLogCriticalEvent(...)	KexLogEvent(LogSeverityCritical, __VA_ARGS__)
#define KexLogErrorEvent(...)		KexLogEvent(LogSeverityError, __VA_ARGS__)
#define KexLogWarningEvent(...)		KexLogEvent(LogSeverityWarning, __VA_ARGS__)
#define KexLogInformationEvent(...)	KexLogEvent(LogSeverityInformation, __VA_ARGS__)
#define KexLogDetailEvent(...)		KexLogEvent(LogSeverityDetail, __VA_ARGS__)

#if defined(_DEBUG) || defined(RELEASE_DEBUGLOGS_ENABLED)
#  define KexLogDebugEvent(...)		KexLogEvent(LogSeverityDebug, __VA_ARGS__)
#else
#  define KexLogDebugEvent(...)
#endif

KEXAPI NTSTATUS CDECL VxlWriteLogEx(
	IN		VXLHANDLE		LogHandle OPTIONAL,
	IN		PCWSTR			SourceComponent OPTIONAL,
	IN		PCWSTR			SourceFile OPTIONAL,
	IN		ULONG			SourceLine,
	IN		PCWSTR			SourceFunction OPTIONAL,
	IN		VXLSEVERITY		Severity,
	IN		PCWSTR			Format,
	IN		...);

//
// vxlread.c
//

KEXAPI NTSTATUS NTAPI VxlReadLog(
	IN		VXLHANDLE		LogHandle,
	IN		ULONG			LogEntryIndex,
	OUT		PVXLLOGENTRY	Entry);

KEXAPI NTSTATUS NTAPI VxlReadMultipleEntriesLog(
	IN		VXLHANDLE		LogHandle,
	IN		ULONG			LogEntryIndexStart,
	IN		ULONG			LogEntryIndexEnd,
	OUT		PVXLLOGENTRY	Entry[]);

//
// vxlsever.c
//

KEXAPI PCWSTR NTAPI VxlSeverityToText_ENG(
	IN		VXLSEVERITY		Severity,
	IN		BOOLEAN			LongDescription);

//
// dllrewrt.c
//

KEXAPI NTSTATUS NTAPI KexRewriteDllPath(
	IN	PCUNICODE_STRING	DllPath,
	OUT	PUNICODE_STRING		RewrittenDllName);

KEXAPI BOOLEAN NTAPI KexIsWindowsDll(
	IN	PCUNICODE_STRING	FullDllName,
	IN	PCUNICODE_STRING	BaseDllName);

//
// kexhe.c
//

ULONG KexMessageBox(
	IN	ULONG	Flags,
	IN	PCWSTR	Caption OPTIONAL,
	IN	PCWSTR	Message OPTIONAL);

VOID KexMessageBoxF(
	IN	ULONG	Flags,
	IN	PCWSTR	Caption OPTIONAL,
	IN	PCWSTR	Message OPTIONAL,
	IN	...);

//
// cpiwbypa.c
//

KEXAPI NTSTATUS NTAPI KexPatchCpiwSubsystemVersionCheck(
	VOID);

//
// System Service Extensions/Hooks
//

NTSTATUS NTAPI Ext_NtQueryInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

NTSTATUS NTAPI Ext_NtSetInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength);

NTSTATUS NTAPI Ext_NtNotifyChangeKey(
	IN	HANDLE				KeyHandle,
	IN	HANDLE				Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID				ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				CompletionFilter,
	IN	BOOLEAN				WatchTree,
	OUT	PVOID				Buffer OPTIONAL,
	IN	ULONG				BufferSize,
	IN	BOOLEAN				Asynchronous);

NTSTATUS NTAPI Ext_NtNotifyChangeMultipleKeys(
	IN	HANDLE				MasterKeyHandle,
	IN	ULONG				Count OPTIONAL,
	IN	OBJECT_ATTRIBUTES	SlaveObjects[] OPTIONAL,
	IN	HANDLE				Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID				ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				CompletionFilter,
	IN	BOOLEAN				WatchTree,
	OUT	PVOID				Buffer OPTIONAL,
	IN	ULONG				BufferSize,
	IN	BOOLEAN				Asynchronous);

NTSTATUS NTAPI Ext_NtCreateSection(
	OUT	PHANDLE				SectionHandle,
	IN	ULONG				DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
	IN	PLONGLONG			MaximumSize OPTIONAL,
	IN	ULONG				PageAttributes,
	IN	ULONG				SectionAttributes,
	IN	HANDLE				FileHandle OPTIONAL);

NTSTATUS NTAPI Ext_NtQueryInformationProcess(
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

KEXAPI NTSTATUS NTAPI Ext_NtAssignProcessToJobObject(
	IN	HANDLE	JobHandle,
	IN	HANDLE	ProcessHandle);

KEXAPI NTSTATUS NTAPI Ext_RtlInitializeCriticalSectionEx(
	IN	PRTL_CRITICAL_SECTION	CriticalSection,
	IN	ULONG					SpinCount,
	IN	ULONG					Flags);

KEXAPI NTSTATUS NTAPI Ext_RtlInitializeCriticalSection(
	IN	PRTL_CRITICAL_SECTION	CriticalSection);

#pragma endregion

#pragma region KexNt* functions

#if defined(KEX_ARCH_X64)

#define DECLARE_SYSCALL(SyscallName, ...) \
KEXAPI NTSTATUS NTAPI Kex##SyscallName##_Win7(__VA_ARGS__); \
KEXAPI NTSTATUS NTAPI Kex##SyscallName##_Win81(__VA_ARGS__); \
KEXAPI NTSTATUS NTAPI Kex##SyscallName(__VA_ARGS__);

#else

#define DECLARE_SYSCALL(SyscallName, ...) \
KEXAPI NTSTATUS NTAPI Kex##SyscallName##_Win7_Native32(__VA_ARGS__); \
KEXAPI NTSTATUS NTAPI Kex##SyscallName##_Win7_Wow64(__VA_ARGS__); \
KEXAPI NTSTATUS NTAPI Kex##SyscallName##_Win81_Native32(__VA_ARGS__); \
KEXAPI NTSTATUS NTAPI Kex##SyscallName##_Win81_Wow64(__VA_ARGS__); \
KEXAPI NTSTATUS NTAPI Kex##SyscallName(__VA_ARGS__);

#endif

DECLARE_SYSCALL(NtQuerySystemTime,
	OUT		PLONGLONG	CurrentTime);

DECLARE_SYSCALL(NtCreateUserProcess,
	OUT		PHANDLE							ProcessHandle,
	OUT		PHANDLE							ThreadHandle,
	IN		ACCESS_MASK						ProcessDesiredAccess,
	IN		ACCESS_MASK						ThreadDesiredAccess,
	IN		POBJECT_ATTRIBUTES				ProcessObjectAttributes OPTIONAL,
	IN		POBJECT_ATTRIBUTES				ThreadObjectAttributes OPTIONAL,
	IN		ULONG							ProcessFlags,
	IN		ULONG							ThreadFlags,
	IN		PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,
	IN OUT	PPS_CREATE_INFO					CreateInfo,
	IN		PPS_ATTRIBUTE_LIST				AttributeList OPTIONAL);

DECLARE_SYSCALL(NtProtectVirtualMemory,
	IN		HANDLE		ProcessHandle,
	IN OUT	PPVOID		BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		NewProtect,
	OUT		PULONG		OldProtect);

DECLARE_SYSCALL(NtAllocateVirtualMemory,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN		ULONG_PTR	ZeroBits,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		AllocationType,
	IN		ULONG		Protect);

DECLARE_SYSCALL(NtQueryVirtualMemory,
	IN		HANDLE			ProcessHandle,
	IN		PVOID			BaseAddress OPTIONAL,
	IN		MEMINFOCLASS	MemoryInformationClass,
	OUT		PVOID			MemoryInformation,
	IN		SIZE_T			MemoryInformationLength,
	OUT		PSIZE_T			ReturnLength OPTIONAL);

DECLARE_SYSCALL(NtFreeVirtualMemory,
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		FreeType);

DECLARE_SYSCALL(NtQueryObject,
	IN		HANDLE						ObjectHandle,
	IN		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
	OUT		PVOID						ObjectInformation,
	IN		ULONG						Length,
	OUT		PULONG						ReturnLength OPTIONAL);

DECLARE_SYSCALL(NtOpenFile,
	OUT		PHANDLE				FileHandle,
	IN		ACCESS_MASK			DesiredAccess,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		ULONG				ShareAccess,
	IN		ULONG				OpenOptions);

DECLARE_SYSCALL(NtWriteFile,
	IN		HANDLE				FileHandle,
	IN		HANDLE				Event OPTIONAL,
	IN		PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN		PVOID				ApcContext OPTIONAL,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		PVOID				Buffer,
	IN		ULONG				Length,
	IN		PLONGLONG			ByteOffset OPTIONAL,
	IN		PULONG				Key OPTIONAL);

DECLARE_SYSCALL(NtRaiseHardError,
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response);

DECLARE_SYSCALL(NtQueryInformationThread,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

DECLARE_SYSCALL(NtSetInformationThread,
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength);

DECLARE_SYSCALL(NtNotifyChangeKey,
	IN	HANDLE				KeyHandle,
	IN	HANDLE				Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID				ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				CompletionFilter,
	IN	BOOLEAN				WatchTree,
	OUT	PVOID				Buffer OPTIONAL,
	IN	ULONG				BufferSize,
	IN	BOOLEAN				Asynchronous);

DECLARE_SYSCALL(NtNotifyChangeMultipleKeys,
	IN	HANDLE				MasterKeyHandle,
	IN	ULONG				Count OPTIONAL,
	IN	OBJECT_ATTRIBUTES	SlaveObjects[] OPTIONAL,
	IN	HANDLE				Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID				ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				CompletionFilter,
	IN	BOOLEAN				WatchTree,
	OUT	PVOID				Buffer OPTIONAL,
	IN	ULONG				BufferSize,
	IN	BOOLEAN				Asynchronous);

DECLARE_SYSCALL(NtCreateSection,
	OUT	PHANDLE				SectionHandle,
	IN	ULONG				DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
	IN	PLONGLONG			MaximumSize OPTIONAL,
	IN	ULONG				PageAttributes,
	IN	ULONG				SectionAttributes,
	IN	HANDLE				FileHandle OPTIONAL);

DECLARE_SYSCALL(NtQueryInformationProcess,
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

DECLARE_SYSCALL(NtAssignProcessToJobObject,
	IN	HANDLE				JobHandle,
	IN	HANDLE				ProcessHandle);

//
// This function was added to the RTL in Win10.
// Added here as an inline function for use by KxCfgHlp, and it is also
// exported from KexDll and KxNt.
// The KexRtl* variant is the inline function, and the Rtl* variant would
// be imported from KexDll.
// Based on Win10 NTDLL decompilation.
//

INLINE BOOLEAN KexRtlIsZeroMemory(
	IN	PCVOID	Buffer,
	IN	SIZE_T	BufferCb)
{
	// Align the input buffer to a multiple of the pointer size.
	while (BufferCb > 0 && ((ULONG_PTR) Buffer & (sizeof(PVOID) - 1)) != 0) {
		if (*(PBYTE) Buffer != 0) {
			return FALSE;
		}

		Buffer = (PBYTE) Buffer + 1;
		--BufferCb;
	}

	// Process the buffer in pointer-sized pieces
	while (BufferCb >= sizeof(PVOID)) {
		if (*(PULONG_PTR) Buffer != 0) {
			return FALSE;
		}

		Buffer = (PULONG_PTR) Buffer + 1;
		BufferCb -= sizeof(PVOID);
	}

	if (BufferCb == 0) {
		return TRUE;
	}

	// Handle remaining bytes at the end.
	do {
		if (*(PBYTE) Buffer != 0) {
			return FALSE;
		}

		Buffer = (PBYTE) Buffer + 1;
		--BufferCb;
	} while (BufferCb > 0);

	return TRUE;
}

KEXAPI BOOLEAN NTAPI RtlIsZeroMemory(
	IN	PCVOID	Buffer,
	IN	SIZE_T	BufferCb);

FORCEINLINE PKEX_TEB_EXTENSION KexCurrentTebExtension(
	VOID)
{
	return (PKEX_TEB_EXTENSION) (&NtCurrentTeb()[1]);
}

#pragma endregion