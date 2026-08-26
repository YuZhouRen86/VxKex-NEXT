///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexldr.c
//
// Abstract:
//
//     Functions for dealing with PE images and the loader subsystem.
//
// Author:
//
//     vxiiduu (06-Nov-2022)
//
// Revision History:
//
//     vxiiduu              06-Nov-2022  Initial creation.
//     vxiiduu              06-Nov-2022  Rework KexLdrGetDllFullNameFromAddress
//     vxiiduu              22-Feb-2024  Add some asserts.
//     vxiiduu              27-Feb-2024  Improve efficiency of routines which
//                                       find NTDLL base addresses.
//     vxiiduu              29-Feb-2024  Revert previous change (wrong assumption).
//     vxiiduu              21-Mar-2024  Properly handle situations where an empty
//                                       DLL name is passed to KexLdrLoadDll
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// Compatible with Win8.
// In Windows 8, this function forms the basis of GetModuleFileName,
// whereas in Win7 and before, the scanning of the loader data table
// is done directly in kernelbase.dll (or kernel32.dll).
//
NTSTATUS NTAPI KexLdrGetDllFullName(
	IN	PVOID			DllBase OPTIONAL,
	OUT	PUNICODE_STRING	DllFullPath)
{
	PUNICODE_STRING FullPath;

	if (DllBase) {
		PLDR_DATA_TABLE_ENTRY Entry;

		if (!LdrpFindLoadedDllByHandle(DllBase, &Entry)) {
			return STATUS_DLL_NOT_FOUND;
		}

		FullPath = &Entry->FullDllName;
	} else {
		FullPath = &NtCurrentPeb()->ProcessParameters->ImagePathName;
	}

	if (FullPath) {
		RtlCopyUnicodeString(DllFullPath, FullPath);
	} else {
		DllFullPath->Length = 0;
	}

	if (FullPath->Length > DllFullPath->MaximumLength) {
		// should technically be STATUS_BUFFER_OVERFLOW because at this
		// point we've already written the data, but go complain to the
		// win8 devs.
		return STATUS_BUFFER_TOO_SMALL;
	}

	return STATUS_SUCCESS;
}

//
// This function matches for any address inside the DLL instead of
// only its base address. It is therefore less efficient than using
// KexLdrGetDllFullName, but required for e.g. figuring out which
// DLL a function call comes from.
//
NTSTATUS NTAPI KexLdrGetDllFullNameFromAddress(
	IN	PCVOID			Address,
	OUT	PUNICODE_STRING	DllFullPath)
{
	NTSTATUS Status;
	PLDR_DATA_TABLE_ENTRY Entry;

	if (!Address || !DllFullPath) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = LdrFindEntryForAddress(Address, &Entry);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	if (Entry->FullDllName.Length > DllFullPath->MaximumLength) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	RtlCopyUnicodeString(DllFullPath, &Entry->FullDllName);
	return STATUS_SUCCESS;
}

//
// Find location of a DLL's init routine (i.e. DllMain).
//
NTSTATUS NTAPI KexLdrFindDllInitRoutine(
	IN	PVOID	DllBase,
	OUT	PPVOID	InitRoutine)
{
	PIMAGE_NT_HEADERS NtHeaders;

	ASSERT (DllBase != NULL);
	ASSERT (InitRoutine != NULL);

	if (!DllBase || !InitRoutine) {
		return STATUS_INVALID_PARAMETER;
	}

	*InitRoutine = NULL;

	NtHeaders = RtlImageNtHeader(DllBase);
	ASSERT (NtHeaders != NULL);

	if (!NtHeaders) {
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	if (NtHeaders->OptionalHeader.AddressOfEntryPoint == 0) {
		return STATUS_ENTRYPOINT_NOT_FOUND;
	}

	*InitRoutine = RVA_TO_VA(DllBase, NtHeaders->OptionalHeader.AddressOfEntryPoint);
	return STATUS_SUCCESS;
}

#ifndef KEX_ARCH_X64
typedef struct _IMAGE_NT_HEADERS_COMMON {
	DWORD				Signature;
	IMAGE_FILE_HEADER	FileHeader;
} TYPEDEF_TYPE_NAME(IMAGE_NT_HEADERS_COMMON);

NTSTATUS NTAPI KexLdrMiniGetProcedureAddress_Wow64(
	IN	PVOID64	DllBase,
	IN	PCSTR	ProcedureName,
	OUT	PVOID64	*ProcedureAddress)
{
	NTSTATUS Status;
	NT_WOW64_READ_VIRTUAL_MEMORY64 NtWow64ReadVirtualMemory64;
	HANDLE ProcessHandle;
	IMAGE_DOS_HEADER DosHeader;
	ULONGLONG ReadAddr;
	ULONGLONG BytesRead;
	IMAGE_NT_HEADERS_COMMON NtCommon;
	ULONG ExportDirRva = 0;
	IMAGE_EXPORT_DIRECTORY ExportDir;
	ULONG NameCount;
	ULONG Index;
	CHAR NameBuffer[256];
	ULONG NameRva, FuncRva;
	USHORT Ordinal;
	ULONGLONG NameArrayAddr, OrdArrayAddr, FuncArrayAddr, StringAddr;

	PVOID SystemDllBase = KexLdrGetSystemDllBase();
	ANSI_STRING NtWow64ReadVirtualMemory64Name;

	if (!DllBase || !ProcedureName || !ProcedureAddress) return STATUS_INVALID_PARAMETER;
	*ProcedureAddress = 0;

	RtlInitConstantAnsiString(&NtWow64ReadVirtualMemory64Name, "NtWow64ReadVirtualMemory64");
	ASSERT(SystemDllBase != NULL);

	Status = LdrGetProcedureAddress(
		SystemDllBase,
		&NtWow64ReadVirtualMemory64Name,
		0,
		(PPVOID)&NtWow64ReadVirtualMemory64);

	ASSERT(NT_SUCCESS(Status));
	ASSERT(NtWow64ReadVirtualMemory64 != NULL);

	ProcessHandle = GetCurrentProcess();

	Status = NtWow64ReadVirtualMemory64(
		ProcessHandle,
		(PVOID64)DllBase,
		&DosHeader,
		sizeof(DosHeader),
		&BytesRead);
	if (!NT_SUCCESS(Status) || BytesRead != sizeof(DosHeader) || DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
		return STATUS_INVALID_IMAGE_FORMAT;

	ReadAddr = (ULONGLONG)DllBase + DosHeader.e_lfanew;
	Status = NtWow64ReadVirtualMemory64(
		ProcessHandle,
		(PVOID64)ReadAddr,
		&NtCommon,
		sizeof(NtCommon),
		&BytesRead);
	if (!NT_SUCCESS(Status) || BytesRead != sizeof(NtCommon) ||
		NtCommon.Signature != IMAGE_NT_SIGNATURE)
		return STATUS_INVALID_IMAGE_FORMAT;

	if (NtCommon.FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
		IMAGE_NT_HEADERS32 NtHeaders32;
		Status = NtWow64ReadVirtualMemory64(
			ProcessHandle,
			(PVOID64)ReadAddr,
			&NtHeaders32,
			sizeof(NtHeaders32),
			&BytesRead);
		if (!NT_SUCCESS(Status) || BytesRead != sizeof(NtHeaders32) ||
			NtHeaders32.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			return STATUS_INVALID_IMAGE_FORMAT;

		ExportDirRva = NtHeaders32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	} else if (NtCommon.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
		IMAGE_NT_HEADERS64 NtHeaders64;
		Status = NtWow64ReadVirtualMemory64(
			ProcessHandle,
			(PVOID64)ReadAddr,
			&NtHeaders64,
			sizeof(NtHeaders64),
			&BytesRead);
		if (!NT_SUCCESS(Status) || BytesRead != sizeof(NtHeaders64) ||
			NtHeaders64.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			return STATUS_INVALID_IMAGE_FORMAT;

		ExportDirRva = NtHeaders64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	} else return STATUS_INVALID_IMAGE_FORMAT;

	if (ExportDirRva == 0) return STATUS_ENTRYPOINT_NOT_FOUND;

	ReadAddr = (ULONGLONG)DllBase + ExportDirRva;
	Status = NtWow64ReadVirtualMemory64(
		ProcessHandle,
		(PVOID64)ReadAddr,
		&ExportDir,
		sizeof(ExportDir),
		&BytesRead);
	if (!NT_SUCCESS(Status) || BytesRead != sizeof(ExportDir)) return STATUS_INVALID_IMAGE_FORMAT;

	NameCount = ExportDir.NumberOfNames;
	for (Index = 0; Index < NameCount; ++Index) {
		ULONGLONG ReadLen = 0;
		NameArrayAddr = (ULONGLONG)DllBase + ExportDir.AddressOfNames + Index * sizeof(ULONG);

		Status = NtWow64ReadVirtualMemory64(
			ProcessHandle,
			(PVOID64)NameArrayAddr,
			&NameRva,
			sizeof(NameRva),
			&BytesRead);
		if (!NT_SUCCESS(Status) || BytesRead != sizeof(NameRva)) continue;

		OrdArrayAddr = (ULONGLONG)DllBase + ExportDir.AddressOfNameOrdinals + Index * sizeof(USHORT);
		Status = NtWow64ReadVirtualMemory64(
			ProcessHandle,
			(PVOID64)OrdArrayAddr,
			&Ordinal,
			sizeof(Ordinal),
			&BytesRead);
		if (!NT_SUCCESS(Status) || BytesRead != sizeof(Ordinal)) continue;

		StringAddr = (ULONGLONG)DllBase + NameRva;
		while (ReadLen < sizeof(NameBuffer) - 1) {
			CHAR Char;
			Status = NtWow64ReadVirtualMemory64(
				ProcessHandle,
				(PVOID64)(StringAddr + ReadLen),
				&Char,
				1,
				&BytesRead);
			if (!NT_SUCCESS(Status) || BytesRead != 1)
				break;
			NameBuffer[ReadLen] = Char;
			ReadLen++;
			if (Char == '\0')
				break;
		}
		NameBuffer[ReadLen] = '\0';

		if (StringEqualA(ProcedureName, NameBuffer)) {
			FuncArrayAddr = (ULONGLONG)DllBase + ExportDir.AddressOfFunctions + Ordinal * sizeof(ULONG);
			Status = NtWow64ReadVirtualMemory64(ProcessHandle, (PVOID64)FuncArrayAddr, &FuncRva, sizeof(FuncRva), &BytesRead);
			if (!NT_SUCCESS(Status) || BytesRead != sizeof(FuncRva)) return STATUS_UNSUCCESSFUL;

			*ProcedureAddress = (PVOID64)((ULONGLONG)DllBase + FuncRva);
			return STATUS_SUCCESS;
		}
	}

	return STATUS_ENTRYPOINT_NOT_FOUND;
}
#endif

//
// Main reason for using this is to:
//   - get proc address in DLLs mapped but not registered with loader
//   - get proc address in "wrong" bitness dlls (e.g. native ntdll.dll from
//     wow64 process or vice versa)
//
// However, for DLLs registered with loader, this function is not useful
// as it is far slower than LdrGetProcedureAddress.
//
NTSTATUS NTAPI KexLdrMiniGetProcedureAddress(
	IN	PVOID64	DllBase,
	IN	PCSTR	ProcedureName,
	OUT	PVOID64	*ProcedureAddress)
{
	PIMAGE_EXPORT_DIRECTORY ExportDirectory;
	ULONG ExportDirectorySize;
	PULONG NameRvas;
	PULONG FunctionRvas;
	PUSHORT NameOrdinals;
	ULONG Index;

	ASSERT (DllBase != NULL);
	ASSERT (ProcedureName != NULL);
	ASSERT (ProcedureAddress != NULL);

	if (!DllBase || !ProcedureName || !ProcedureAddress) {
		return STATUS_INVALID_PARAMETER;
	}

	*ProcedureAddress = NULL;

	ExportDirectory = (PIMAGE_EXPORT_DIRECTORY) RtlImageDirectoryEntryToData(
		DllBase, 
		TRUE, 
		IMAGE_DIRECTORY_ENTRY_EXPORT, 
		&ExportDirectorySize);

	if (KexRtlOperatingSystemBitness() == KexRtlCurrentProcessBitness())
		ASSERT (ExportDirectory != NULL);

	if (!ExportDirectory) {
#ifndef KEX_ARCH_X64
		if (KexRtlOperatingSystemBitness() != KexRtlCurrentProcessBitness())
			return KexLdrMiniGetProcedureAddress_Wow64(DllBase, ProcedureName, ProcedureAddress);
#endif
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	NameRvas = (PULONG) RVA_TO_VA(DllBase, ExportDirectory->AddressOfNames);
	FunctionRvas = (PULONG) RVA_TO_VA(DllBase, ExportDirectory->AddressOfFunctions);
	NameOrdinals = (PUSHORT) RVA_TO_VA(DllBase, ExportDirectory->AddressOfNameOrdinals);

	for (Index = 0; Index < ExportDirectory->NumberOfNames; ++Index) {
		PCSTR CurrentProcedureName;

		CurrentProcedureName = (PCSTR) RVA_TO_VA(DllBase, NameRvas[Index]);

		if (StringEqualA(ProcedureName, CurrentProcedureName)) {
			*ProcedureAddress = RVA_TO_VA(DllBase, FunctionRvas[NameOrdinals[Index]]);
			return STATUS_SUCCESS;
		}
	}

	return STATUS_ENTRYPOINT_NOT_FOUND;
}

//
// Get the base address of NTDLL.
//
KEXAPI PVOID NTAPI KexLdrGetSystemDllBase(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING NtdllBaseName;
	PVOID NtdllBaseAddress;

	RtlInitConstantUnicodeString(&NtdllBaseName, L"ntdll.dll");

	Status = LdrGetDllHandle(NULL, NULL, &NtdllBaseName, &NtdllBaseAddress);
	ASSERT (NT_SUCCESS(Status));

	if (NT_SUCCESS(Status)) {
		return NtdllBaseAddress;
	} else {
		return NULL;
	}
}

//
// Get the base address of NTDLL for another process.
// This function returns the address of 32-bit NTDLL for a
// 32-bit process, and 64-bit NTDLL for a 64-bit process.
//
KEXAPI PVOID64 NTAPI KexLdrGetRemoteSystemDllBase(
	IN	HANDLE	ProcessHandle)
{
	NTSTATUS Status;
	UNICODE_STRING NtdllPathFragment;
	UNICODE_STRING NtdllBaseName;
	ULONG_PTR NtdllBaseAddress;
	PUNICODE_STRING MappedFileNameInformation;
	ULONG MappedFileNameLength;
	ULONG RemoteProcessBitness;

	RtlInitConstantUnicodeString(&NtdllBaseName, L"ntdll.dll");
	
	RemoteProcessBitness = KexRtlRemoteProcessBitness(ProcessHandle);
	
	//
	// We can avoid scanning for NTDLL if we are the same bitness as the
	// remote process, or if we are a WOW64 process
	//

	if (KexRtlCurrentProcessBitness() == RemoteProcessBitness) {
		ASSERT (KexData->SystemDllBase != NULL);
		
		return KexData->SystemDllBase;
	} else if (KexRtlCurrentProcessBitness() != KexRtlOperatingSystemBitness()) {
		ASSERT (KexRtlCurrentProcessBitness() == 32);
		ASSERT (RemoteProcessBitness == 64);
		ASSERT (KexData->NativeSystemDllBase != NULL);

		return KexData->NativeSystemDllBase;
	}

	//
	// We must search for NTDLL if we are a 64 bit process looking for the
	// WOW64 NTDLL.
	//

	ASSUME (KexRtlCurrentProcessBitness() == 64);
	ASSUME (KexRtlOperatingSystemBitness() == 64);
	ASSUME (RemoteProcessBitness == 32);

	MappedFileNameLength = 256;
	MappedFileNameInformation = (PUNICODE_STRING) StackAlloc(BYTE, MappedFileNameLength);

	RtlInitConstantUnicodeString(&NtdllPathFragment, L"syswow64\\ntdll.dll");

	for (NtdllBaseAddress = 0x7FFD0000; NtdllBaseAddress >= 0x70000000; NtdllBaseAddress -= 0x10000) {
		MEMORY_BASIC_INFORMATION BasicInformation;

		Status = NtQueryVirtualMemory(
			ProcessHandle,
			(PVOID) NtdllBaseAddress,
			MemoryMappedFilenameInformation,
			MappedFileNameInformation,
			MappedFileNameLength,
			NULL);

		if (!NT_SUCCESS(Status)) {
			continue;
		}

		KexLogDebugEvent(
			L"Found mapped file in remote process: %wZ",
			MappedFileNameInformation);

		//
		// Check if this memory-mapped image is NTDLL.
		//

		if (!KexRtlUnicodeStringEndsWith(MappedFileNameInformation, &NtdllPathFragment, TRUE)) {
			continue;
		}

		//
		// We will now confirm that this file is an image, and find
		// the base address of this image file.
		//

		Status = NtQueryVirtualMemory(
			ProcessHandle,
			(PVOID) NtdllBaseAddress,
			MemoryBasicInformation,
			&BasicInformation,
			sizeof(BasicInformation),
			NULL);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			continue;
		}

		if (BasicInformation.Type != MEM_IMAGE) {
			continue;
		}

		NtdllBaseAddress = (ULONG_PTR) BasicInformation.AllocationBase;

		return (PVOID) NtdllBaseAddress;
	}

	//
	// Could not find.
	//

	ASSERT (FALSE);
	return NULL;
}

#ifndef KEX_ARCH_X64
typedef struct _LDR64 {
	ULONG			Length;
	BOOLEAN			Initialized;
	ULONGLONG		SsHandle;
	LIST_ENTRY64	InLoadOrderModuleList;
	LIST_ENTRY64	InMemoryOrderModuleList;
	LIST_ENTRY64	InInitializationOrderModuleList;
	ULONGLONG		EntryInProgress;
} TYPEDEF_TYPE_NAME(LDR64);

typedef struct _UNICODE_STRING64 {
	USHORT	Length;
	USHORT	MaximumLength;
	PVOID64	Buffer;
} TYPEDEF_TYPE_NAME(UNICODE_STRING64);

typedef struct _LDR_DATA_TABLE_ENTRY64 {
	LIST_ENTRY64		InLoadOrderLinks;
	LIST_ENTRY64		InMemoryOrderLinks;
	LIST_ENTRY64		InInitializationOrderLinks;
	PVOID64				DllBase;
	PVOID64				EntryPoint;
	ULONG				SizeOfImage;
	UNICODE_STRING64	FullDllName;
	UNICODE_STRING64	BaseDllName;
	ULONG				Flags;
	USHORT				LoadCount;
	USHORT				TlsIndex;
} TYPEDEF_TYPE_NAME(LDR_DATA_TABLE_ENTRY64);

PVOID64 GetNativeNtdllAddress_Wow64(
	VOID)
{
	HANDLE ProcessHandle;
	NT_WOW64_QUERY_INFORMATION_PROCESS64 NtWow64QueryInformationProcess64;
	NT_WOW64_READ_VIRTUAL_MEMORY64 NtWow64ReadVirtualMemory64;
	ULONGLONG Peb64;
	CONST ULONG InfoSize = 48;
	BYTE BufferArray[48];
	ULONG ReturnLength = 0;
	NTSTATUS Status;
	PVOID64 LdrAddress;
	ULONGLONG BytesRead = 0;
	LDR64 LdrData;
	ULONGLONG HeadLink;
	ULONGLONG CurrentEntry;
	LDR_DATA_TABLE_ENTRY64 EntryData;
	PWSTR FullPath = NULL;
	PVOID64 Result = NULL;
	PVOID SystemDllBase = KexLdrGetSystemDllBase();
	ANSI_STRING NtWow64QueryInformationProcess64Name;
	ANSI_STRING NtWow64ReadVirtualMemory64Name;

	ProcessHandle = GetCurrentProcess();
	RtlZeroMemory(BufferArray, sizeof(BufferArray));
	RtlZeroMemory(&LdrData, sizeof(LdrData));
	RtlZeroMemory(&EntryData, sizeof(EntryData));

	if (!SystemDllBase) {
		return Result;
	}

	RtlInitConstantAnsiString(&NtWow64QueryInformationProcess64Name, "NtWow64QueryInformationProcess64");
	RtlInitConstantAnsiString(&NtWow64ReadVirtualMemory64Name, "NtWow64ReadVirtualMemory64");
	ASSERT (SystemDllBase != NULL);

	Status = LdrGetProcedureAddress(
		SystemDllBase,
		&NtWow64QueryInformationProcess64Name,
		0,
		(PPVOID) &NtWow64QueryInformationProcess64);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (NtWow64QueryInformationProcess64 != NULL);

	Status = LdrGetProcedureAddress(
		SystemDllBase,
		&NtWow64ReadVirtualMemory64Name,
		0,
		(PPVOID) &NtWow64ReadVirtualMemory64);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (NtWow64ReadVirtualMemory64 != NULL);

	if (!NtWow64QueryInformationProcess64 || !NtWow64ReadVirtualMemory64) return Result;

	Status = NtWow64QueryInformationProcess64(ProcessHandle, 0, BufferArray, InfoSize, &ReturnLength);
	if (NT_SUCCESS(Status) && ReturnLength >= 16) {
		Peb64 = *(ULONGLONG*)(BufferArray + 8);
	} else return Result;

	BytesRead = 0;
	Status = NtWow64ReadVirtualMemory64(ProcessHandle, (PVOID64)(Peb64 + 0x18), &LdrAddress, sizeof(LdrAddress), &BytesRead);
	if (Status != 0 || BytesRead != sizeof(LdrAddress)) return Result;

	BytesRead = 0;
	Status = NtWow64ReadVirtualMemory64(ProcessHandle, LdrAddress, (PVOID)&LdrData, sizeof(LdrData), &BytesRead);
	if (Status != 0 || BytesRead != sizeof(LdrData)) return Result;

	HeadLink = LdrData.InLoadOrderModuleList.Flink;
	CurrentEntry = HeadLink;

	while (CurrentEntry != 0) {
		SIZE_T FullPathLength = 0;
		BytesRead = 0;
		Status = NtWow64ReadVirtualMemory64(ProcessHandle, (PVOID64)CurrentEntry, &EntryData, sizeof(EntryData), &BytesRead);
		if (Status != 0 || BytesRead != sizeof(EntryData)) return Result;

		if (EntryData.InLoadOrderLinks.Flink == HeadLink) {
			break;
		}

		FullPath = NULL;
		if (EntryData.FullDllName.Buffer != 0 && EntryData.FullDllName.Length != 0) {
			ULONG FullDllNameLength = EntryData.FullDllName.Length;
			FullPathLength = FullDllNameLength / sizeof(WCHAR);
			FullPath = SafeAlloc(WCHAR, FullPathLength + 1);
			if (FullPath != NULL) {
				Status = NtWow64ReadVirtualMemory64(
					ProcessHandle,
					EntryData.FullDllName.Buffer,
					FullPath,
					FullDllNameLength,
					&BytesRead);
				if (NT_SUCCESS(Status) && BytesRead == FullDllNameLength) {
					FullPath[FullPathLength] = L'\0';
				}
				else SafeFree(FullPath);
			}
		}

		if (FullPath != NULL) {
			BOOL IsValidNtdllPath = FALSE;
			if (wcslen(FullPath) > 0) {
				PCWSTR Suffix = L"\\system32\\ntdll.dll";
				SIZE_T SuffixLength = wcslen(Suffix);
				if (FullPathLength >= SuffixLength && StringEqualI(FullPath + FullPathLength - SuffixLength, Suffix)) {
					IsValidNtdllPath = TRUE;
				}
			}
			SafeFree(FullPath);
			if (IsValidNtdllPath) {
				Result = (PVOID64)EntryData.DllBase;
				break;
			}
		}

		CurrentEntry = EntryData.InLoadOrderLinks.Flink;
	}

	return Result;
}
#endif

//
// Get the base address of the native NTDLL. In other words:
// if this is a 32-bit process running on a 64-bit operating
// system, get the 64-bit NTDLL, and so on.
//
KEXAPI PVOID64 NTAPI KexLdrGetNativeSystemDllBase(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING NtdllPathFragment;
	UNICODE_STRING NtdllBaseName;
	ULONG_PTR NtdllBaseAddress;
	PUNICODE_STRING MappedFileNameInformation;
	ULONG MappedFileNameLength;

	//
	// 64-bit NTDLL is mapped between 0x7FFD0000 and 0x70000000 on
	// boundaries of 0x10000 (due to ASLR). This gives us 256
	// possibilities we need to search for. To avoid this penalty,
	// we will avoid performing the search if we could just get
	// NTDLL's base address from the loader subsystem.
	//

	RtlInitConstantUnicodeString(&NtdllBaseName, L"ntdll.dll");

	if (KexRtlCurrentProcessBitness() == KexRtlOperatingSystemBitness()) {
		Status = LdrGetDllHandle(NULL, NULL, &NtdllBaseName, (PPVOID) &NtdllBaseAddress);
		ASSERT (NT_SUCCESS(Status));
		return (PVOID64) NtdllBaseAddress;
	}

	ASSUME (KexRtlCurrentProcessBitness() == 32);
	ASSUME (KexRtlOperatingSystemBitness() == 64);
#ifndef KEX_ARCH_X64
	return GetNativeNtdllAddress_Wow64();
#endif
	/*
	//
	// This is a 32-bit process running on a 64-bit operating system. We must
	// search for the 64-bit NTDLL as described.
	//

	MappedFileNameLength = 256;
	MappedFileNameInformation = (PUNICODE_STRING) StackAlloc(BYTE, MappedFileNameLength);
	RtlInitConstantUnicodeString(&NtdllPathFragment, L"system32\\ntdll.dll");

	for (NtdllBaseAddress = 0x7FFD0000; NtdllBaseAddress >= 0x70000000; NtdllBaseAddress -= 0x10000) {
		MEMORY_BASIC_INFORMATION BasicInformation;

		Status = NtQueryVirtualMemory(
			NtCurrentProcess(),
			(PVOID) NtdllBaseAddress,
			MemoryMappedFilenameInformation,
			MappedFileNameInformation,
			MappedFileNameLength,
			NULL);

		if (!NT_SUCCESS(Status)) {
			continue;
		}

		//
		// Confirm that this memory-mapped image is in fact the native
		// NTDLL inside the system32 directory.
		//

		if (!KexRtlUnicodeStringEndsWith(MappedFileNameInformation, &NtdllPathFragment, TRUE)) {
			continue;
		}

		//
		// We now have a pointer to a memory mapped file.
		// We will now determine whether this file is an image, and also
		// the base address of this image file.
		//

		Status = NtQueryVirtualMemory(
			NtCurrentProcess(),
			(PVOID) NtdllBaseAddress,
			MemoryBasicInformation,
			&BasicInformation,
			sizeof(BasicInformation),
			NULL);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			continue;
		}

		if (BasicInformation.Type != MEM_IMAGE) {
			continue;
		}

		NtdllBaseAddress = (ULONG_PTR) BasicInformation.AllocationBase;
		return (PVOID64) NtdllBaseAddress;
	}*/

	//
	// Could not find.
	//

	ASSERT (FALSE);
	UNREFERENCED_PARAMETER(NtdllPathFragment);
	UNREFERENCED_PARAMETER(MappedFileNameInformation);
	UNREFERENCED_PARAMETER(MappedFileNameLength);
	return NULL;
}

//
// Get the allocation base and region size for a memory region which contains import
// descriptors for the given PE image.
//

KEXAPI NTSTATUS NTAPI KexLdrGetImageImportSection(
	IN	PVOID	ImageBase,
	OUT	PPVOID	ImportSectionBase,
	OUT	PSIZE_T	ImportSectionSize)
{
	NTSTATUS Status;
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_FILE_HEADER CoffHeader;
	PIMAGE_OPTIONAL_HEADER OptionalHeader;
	PIMAGE_DATA_DIRECTORY ImportDirectory;
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
	MEMORY_BASIC_INFORMATION BasicInformation;

	ASSERT (ImageBase != NULL);
	ASSERT (ImportSectionBase != NULL);
	ASSERT (ImportSectionSize != NULL);

	Status = RtlImageNtHeaderEx(
		RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK,
		ImageBase,
		0,
		&NtHeaders);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		KexLogErrorEvent(
			L"Failed to retrieve the address of the image NT headers for the "
			L"image at base: 0x%p\r\n\r\n"
			L"NTSTATUS error code: %s",
			ImageBase,
			KexRtlNtStatusToString(Status));

		return Status;
	}

	CoffHeader = &NtHeaders->FileHeader;
	OptionalHeader = &NtHeaders->OptionalHeader;
	ImportDirectory = &OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR) RVA_TO_VA(ImageBase, ImportDirectory->VirtualAddress);
	
	if (ImportDescriptor->Name == 0) {
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	//
	// ImportDescriptor now points to the first import descriptor.
	// Use NtQueryVirtualMemory to find the size of the memory zone which contains
	// all the import descriptors.
	//

	Status = NtQueryVirtualMemory(
		NtCurrentProcess(),
		RVA_TO_VA(ImageBase, ImportDescriptor->Name),
		MemoryBasicInformation,
		&BasicInformation,
		sizeof(BasicInformation),
		NULL);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	if (BasicInformation.Type != MEM_IMAGE) {
		//
		// Software (such as MacType versions 2025.01 and earlier) which modifies the
		// import table (just like we do) may break VxKex-enabled applications. As a
		// symptom of this, BasicInformation.Type will be equal to MEM_PRIVATE. In this
		// case, we will consider that we have failed to find the import section.
		//
		// Failing here will allow us to show the user an error message rather than just
		// crashing with an access violation later.
		//

		return STATUS_ACCESS_VIOLATION;
	}

	ASSERT (BasicInformation.State == MEM_COMMIT);
	ASSERT (BasicInformation.Type == MEM_IMAGE);

	*ImportSectionBase = BasicInformation.BaseAddress;
	*ImportSectionSize = BasicInformation.RegionSize;

	return STATUS_SUCCESS;
}

//
// DllPath is a string containing semicolon-separated Win32 directory paths (similar
// to the PATH environment variable).
// DllName is a Win32 path.
//
// DllCharacteristics can be a combination of the following:
//
//   DLL_CHARACTERISTIC_IGNORE_CODE_AUTHZ_LEVEL	- equivalent to LOAD_IGNORE_CODE_AUTHZ_LEVEL
//   DLL_CHARACTERISTIC_LOAD_AS_DATA			- equivalent to DONT_RESOLVE_DLL_REFERENCES
//   DLL_CHARACTERISTIC_REQUIRE_SIGNATURE		- equivalent to LOAD_LIBRARY_REQUIRE_SIGNED_TARGET
//
KEXAPI NTSTATUS NTAPI KexLdrLoadDll(
	IN	PCWSTR				DllPath OPTIONAL,
	IN	PULONG				DllCharacteristicsIndirect OPTIONAL,
	IN	PCUNICODE_STRING	DllName,
	OUT	PPVOID				DllHandle)
{
	NTSTATUS Status;
	PCWSTR OriginalDllPath;
	ULONG DllCharacteristics;
	UNICODE_STRING RewrittenDll;
	UNICODE_STRING TbsDllName;
	BOOLEAN ShouldRewrite = NtCurrentTeb()->KexLdrShouldRewriteDll;
	BOOLEAN RewriteTbs = FALSE;

	ASSERT (VALID_UNICODE_STRING(DllName));
	ASSERT (DllHandle != NULL);

	OriginalDllPath = DllPath;
	DllCharacteristics = DllCharacteristicsIndirect ? *DllCharacteristicsIndirect : 0;

	//
	// Very weird shit going on here, not sure why this is needed, but it is.
	// This stuff was added in an update after Win7 SP1. If the least significant
	// bit of DllPath is set, that means DllPath is actually a pointer to an array
	// of 2 PCWSTRs. The first one is the regular DLL path and the 2nd one is an
	// alternate path.
	//

	if (DllPath && (((ULONG_PTR) DllPath) & 1)) {
		ULONG_PTR DllPathPointer;
		PPCWSTR DllPathIndirect;

		DllPathPointer = (ULONG_PTR) DllPath;
		DllPathPointer &= ~1;
		DllPathIndirect = (PPCWSTR) DllPathPointer;
		DllPath = *DllPathIndirect;
	}

	if (DllName->Length == 0) {
		goto BailOut;
	}

	if (DllCharacteristics & DLL_CHARACTERISTIC_LOAD_AS_DATA) {
		// They are probably trying to get resources or something out of
		// the DLL.
		goto BailOut;
	}

	/*if (!ShouldRewrite && !AshModuleIsDynamicRewriteExemptedModule(ReturnAddress())) {
		// An app (e.g. Thunderbird) has called LdrLoadDll directly.
		ShouldRewrite = TRUE;
	}*/

	if (!ShouldRewrite) {
		// Skip past DLL rewriting.
		goto BailOut;
	}

	//
	// Try to rewrite the DLL name or path.
	//

	RtlInitEmptyUnicodeStringFromTeb(&RewrittenDll);

	RtlInitConstantUnicodeString(&TbsDllName, L"tbs.dll");
	unless (KexData->IfeoParameters.DisableAppSpecific) {
		if ((KexData->Flags & KEXDATA_FLAG_CHROMIUM) && RtlEqualUnicodeString(&TbsDllName, DllName, TRUE))
			RewriteTbs = TRUE;
	}

	if (RewriteTbs) {
		UNICODE_STRING KxmiDllName;
		RtlInitConstantUnicodeString(&KxmiDllName, L"kxmi");
		RtlCopyUnicodeString(&RewrittenDll, &KxmiDllName);
		Status = STATUS_SUCCESS;
	} else {
		Status = KexRewriteDllPath(DllName, &RewrittenDll);
	}

	ASSERT (NT_SUCCESS(Status) ||
			Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND ||
			Status == STATUS_DLL_NOT_IN_SYSTEM_ROOT);

	if (!NT_SUCCESS(Status)) {
		goto BailOut;
	}

	//
	// We've successfully rewritten the DLL name.
	// Set the DLL search path to the default (NULL).
	// Remove any problematic DLL characteristics.
	//

	DllName = &RewrittenDll;
	DllPath = NULL;
	DllCharacteristics &= ~DLL_CHARACTERISTIC_REQUIRE_SIGNATURE;
	DllCharacteristicsIndirect = &DllCharacteristics;

BailOut:
	if (DllPath) {
		PWSTR NewDllPathBuffer;
		SIZE_T NewDllPathCch;
		UNICODE_STRING NewDllPath;

		//
		// Prepend Kex3264Dir in front of the original DLL path.
		//

		NewDllPathCch = wcslen(DllPath) + (KexRtlUnicodeStringCch(&KexData->Kex3264DirPath) + 1) + 1;
		NewDllPathBuffer = StackAlloc(WCHAR, NewDllPathCch);
		RtlInitEmptyUnicodeString(&NewDllPath, NewDllPathBuffer, NewDllPathCch * sizeof(WCHAR));

		RtlCopyUnicodeString(&NewDllPath, &KexData->Kex3264DirPath);
		RtlAppendUnicodeToString(&NewDllPath, L";");
		RtlAppendUnicodeToString(&NewDllPath, DllPath);

		KexRtlNullTerminateUnicodeString(&NewDllPath);
		DllPath = NewDllPath.Buffer;

		ASSERT (((ULONG_PTR) DllPath & 1) == 0);
	}

	Status = LdrLoadDll(
		DllPath,
		DllCharacteristicsIndirect,
		DllName,
		DllHandle);

	if (!NT_SUCCESS(Status)) {
		//
		// Use Detail severity when the call originates from a non-rewritten module.
		// Use Warning severity when the call originates from a module for which we
		// have rewritten imports (i.e. target application's modules).
		//

		KexLogEvent(
			NtCurrentTeb()->KexLdrShouldRewriteDll ? LogSeverityWarning : LogSeverityDetail,
			L"Failed to dynamically load %wZ.\r\n\r\n"
			L"DllPath:            \"%s\"\r\n"
			L"DllCharacteristics: 0x%08lx\r\n"
			L"NTSTATUS error code: %s (0x%08lx)",
			DllName,
			DllPath,
			DllCharacteristics,
			KexRtlNtStatusToString(Status), Status);
	}

	return Status;
}

KEXAPI NTSTATUS NTAPI KexLdrGetDllHandleEx(
	IN	ULONG				Flags,
	IN	PCWSTR				DllPath OPTIONAL,
	IN	PULONG				DllCharacteristics OPTIONAL,
	IN	PCUNICODE_STRING	DllName,
	OUT	PPVOID				DllHandle)
{
	NTSTATUS Status;
	UNICODE_STRING RewrittenDll;
	BOOLEAN ShouldRewrite = NtCurrentTeb()->KexLdrShouldRewriteDll;

	ASSERT (VALID_UNICODE_STRING(DllName));

	if (DllName->Length == 0) {
		goto BailOut;
	}

	/*if (!ShouldRewrite && !AshModuleIsDynamicRewriteExemptedModule(ReturnAddress())) {
		// An app called LdrGetDllHandle(Ex) directly
		ShouldRewrite = TRUE;
	}*/

	if (!ShouldRewrite) {
		goto BailOut;
	}

	//
	// Try to rewrite DLL.
	//

	RtlInitEmptyUnicodeStringFromTeb(&RewrittenDll);

	Status = KexRewriteDllPath(DllName, &RewrittenDll);

	ASSERT (NT_SUCCESS(Status) ||
			Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND ||
			Status == STATUS_DLL_NOT_IN_SYSTEM_ROOT);

	if (!NT_SUCCESS(Status)) {
		goto BailOut;
	}

	//
	// The DLL was rewritten.
	//

	DllPath = NULL;
	DllCharacteristics = NULL;
	DllName = &RewrittenDll;

BailOut:
	return LdrGetDllHandleEx(
		Flags,
		DllPath,
		DllCharacteristics,
		DllName,
		DllHandle);
}

KEXAPI NTSTATUS NTAPI KexLdrGetDllHandle(
	IN	PCWSTR				DllPath OPTIONAL,
	IN	PULONG				DllCharacteristics OPTIONAL,
	IN	PCUNICODE_STRING	DllName,
	OUT	PPVOID				DllHandle)
{
	return KexLdrGetDllHandleEx(
		LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT,
		DllPath,
		DllCharacteristics,
		DllName,
		DllHandle);
}

KEXAPI NTSTATUS NTAPI KexLdrGetProcedureAddressEx(
	IN	PVOID				DllHandle,
	IN	PCANSI_STRING		ProcedureName OPTIONAL,
	IN	ULONG				ProcedureNumber OPTIONAL,
	OUT	PPVOID				ProcedureAddress,
	IN	ULONG				Flags)
{
	NTSTATUS Status;

	Status = LdrGetProcedureAddressEx(
		DllHandle,
		ProcedureName,
		ProcedureNumber,
		ProcedureAddress,
		Flags);

	if (!NT_SUCCESS(Status)) {
		NTSTATUS Status2;
		UNICODE_STRING FullDllName;
		UNICODE_STRING BaseDllName;

		RtlInitEmptyUnicodeStringFromTeb(&FullDllName);
		Status2 = KexLdrGetDllFullName(DllHandle, &FullDllName);
		ASSERT (NT_SUCCESS(Status2));

		if (!NT_SUCCESS(Status2)) {
			RtlInitConstantUnicodeString(&FullDllName, L"(unknown)");
		}

		Status2 = KexRtlPathFindFileName(&FullDllName, &BaseDllName);
		ASSERT (NT_SUCCESS(Status2));

		if (!NT_SUCCESS(Status2)) {
			RtlInitConstantUnicodeString(&BaseDllName, L"(unknown)");
		}

		KexLogWarningEvent(
			L"Failed to resolve %hZ (#%lu) from %wZ\r\n\r\n"
			L"DLL base address:     0x%p\r\n"
			L"Full path to the DLL: %wZ\r\n"
			L"Flags:                0x%08lx\r\n"
			L"NTSTATUS error code:  %s (0x%08lx)",
			ProcedureName,
			ProcedureNumber,
			&BaseDllName,
			DllHandle,
			&FullDllName,
			Flags,
			KexRtlNtStatusToString(Status), Status);
	}

	return Status;
}

KEXAPI NTSTATUS NTAPI KexLdrGetProcedureAddress(
	IN	PVOID				DllHandle,
	IN	PCANSI_STRING		ProcedureName OPTIONAL,
	IN	ULONG				ProcedureNumber OPTIONAL,
	OUT	PPVOID				ProcedureAddress)
{
	return KexLdrGetProcedureAddressEx(
		DllHandle,
		ProcedureName,
		ProcedureNumber,
		ProcedureAddress,
		0);
}