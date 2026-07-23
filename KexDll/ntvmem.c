#include "buildcfg.h"
#include "kexdllp.h"

#define MEM_EXTENDED_PARAMETER_EC_CODE          0x40UL

STATIC NTSTATUS ProcessExtendedParameters(
	PMEM_EXTENDED_PARAMETER	ExtendedParameters,
	ULONG					ExtendedParameterCount)
{
	ULONG Index;
	PMEM_EXTENDED_PARAMETER Param;
	PMEM_ADDRESS_REQUIREMENTS AddrReq;

	// No extended parameters -> trivially supported
	if (ExtendedParameters == NULL || ExtendedParameterCount == 0) {
		return STATUS_SUCCESS;
	}

	// Validate each parameter
	for (Index = 0; Index < ExtendedParameterCount; Index++) {
		Param = &ExtendedParameters[Index];

		switch (Param->Type) {
		case MemExtendedParameterAddressRequirements:
			// Address bounds / alignment - only supported if all fields are zero/NULL
			if (Param->Pointer == NULL) {
				return STATUS_INVALID_PARAMETER;
			}
			AddrReq = (PMEM_ADDRESS_REQUIREMENTS)Param->Pointer;

			// Non-NULL lowest address not supported on Windows 7
			if (AddrReq->LowestStartingAddress != NULL) {
				return STATUS_NOT_SUPPORTED;
			}

			// Non-NULL highest address not supported on Windows 7
			if (AddrReq->HighestEndingAddress != NULL) {
				return STATUS_NOT_SUPPORTED;
			}

			// Non-zero alignment not supported
			if (AddrReq->Alignment != 0) {
				if ((AddrReq->Alignment & (AddrReq->Alignment - 1)) != 0) {
					return STATUS_INVALID_PARAMETER;
				}
				return STATUS_NOT_SUPPORTED;
			}
			// All fields zero/NULL -> okay (no effective restriction)
			break;

		case MemExtendedParameterNumaNode:
		case MemExtendedParameterPartitionHandle:
		case MemExtendedParameterUserPhysicalHandle:
		case MemExtendedParameterAttributeFlags:
		case MemExtendedParameterImageMachine:
			// These features are not available on Windows 7
			return STATUS_NOT_SUPPORTED;

		case MemExtendedParameterInvalidType:
		default:
			return STATUS_INVALID_PARAMETER;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtAllocateVirtualMemoryEx(
	HANDLE						ProcessHandle,
	PVOID						*BaseAddress,
	PSIZE_T						RegionSize,
	ULONG						AllocationType,
	ULONG						PageProtection,
	PMEM_EXTENDED_PARAMETER		ExtendedParameters,
	ULONG						ExtendedParameterCount)
{
	NTSTATUS Status;

	// Validate base parameters
	if (BaseAddress == NULL || RegionSize == NULL || *RegionSize == 0) {
		return STATUS_INVALID_PARAMETER;
	}

	// Validate extended parameters
	Status = ProcessExtendedParameters(
		ExtendedParameters,
		ExtendedParameterCount);
	if (Status != STATUS_SUCCESS) {
		return Status;
	}

	// Forward to Windows 7 native API (ZeroBits = 0)
	Status = NtAllocateVirtualMemory(
		ProcessHandle,
		BaseAddress,
		0,
		RegionSize,
		AllocationType,
		PageProtection);

	return Status;
}