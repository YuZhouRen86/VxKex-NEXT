///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     strmap.c
//
// Abstract:
//
//     This module contains the String Mapper API.
//
//     A string mapper is a data structure that lets you map one string to
//     another. In VxKex it is used as part of the mechanism that rewrites
//     the import directory of an image file.
//
// Author:
//
//     vxiiduu (21-Oct-2022)
//
// Environment:
//
//     String mappers can be used after the RTL heap system is initialized and
//     the process heap has been created.
//
// Revision History:
//
//     vxiiduu              21-Oct-2022  Initial creation.
//     vxiiduu              16-Mar-2024  Remove erroneous OPTIONAL qualifier on
//                                       the 2nd argument to InsertEntry.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

BOOLEAN CreateHashTable(
	IN OUT	PPRTL_DYNAMIC_HASH_TABLE	HashTable,
	IN		ULONG						Shift,
	IN		ULONG						Flags)
{
	ULONG Index;
    PRTL_DYNAMIC_HASH_TABLE Table;
	PLIST_ENTRY Directory;

    // Validate input parameters
    if (!HashTable || Shift >= 0x40) return FALSE;
	
    // Allocate hash table structure
	if (!*HashTable) {
		Table = SafeAlloc(RTL_DYNAMIC_HASH_TABLE, 1);
		if (!Table) return FALSE;
		*HashTable = Table;
		Flags |= 1;
	}
    Table = *HashTable;
    RtlZeroMemory(Table, sizeof(RTL_DYNAMIC_HASH_TABLE));

    // Initialize basic parameters
    Table->Flags = Flags;
    Table->Shift = Shift;
    Table->TableSize = 0x80;
    Table->DivisorMask = Table->TableSize - 1;
    Table->Pivot = 0;

    // Allocate and initialize directory array
	Directory = SafeAlloc(LIST_ENTRY, Table->TableSize);
    if (!Directory) {
        SafeFree(Table);
        return FALSE;
    }

	for (Index = 0; Index < Table->TableSize; ++Index) {
		Directory[Index].Blink = &Directory[Index];
		Directory[Index].Flink = &Directory[Index];
	}

	Table->Directory = Directory;

    return TRUE;
}

ULONG DeleteHashTable(
	IN	PRTL_DYNAMIC_HASH_TABLE	HashTable)
{
	if (!HashTable || HashTable->NumEnumerators != 0) return FALSE;
	SafeFree(HashTable->Directory);
	SafeFree(HashTable);
	return TRUE;
}

BOOLEAN InitEnumerationHashTable(
	IN	PRTL_DYNAMIC_HASH_TABLE				HashTable,
	OUT	PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR	Enumerator)
{
	ULONG Index;

	if (!HashTable || !Enumerator) return FALSE;

	RtlZeroMemory(Enumerator, sizeof(RTL_DYNAMIC_HASH_TABLE_ENUMERATOR));

	for (Index = 0; Index < HashTable->TableSize; ++Index) {
		PLIST_ENTRY Chain = &((PLIST_ENTRY)HashTable->Directory)[Index];
		if (!IsListEmpty(Chain)) {
			Enumerator->BucketIndex = Index;
			Enumerator->ChainHead = Chain;
			Enumerator->HashEntry.Linkage.Flink = Chain->Flink;
			InterlockedIncrement((PLONG)&HashTable->NumEnumerators);
			return TRUE;
		}
	}
	return FALSE;
}

VOID EndEnumerationHashTable(
	IN		PRTL_DYNAMIC_HASH_TABLE				HashTable,
	IN OUT	PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR	Enumerator)
{
	if (HashTable && Enumerator) {
		InterlockedDecrement((PLONG)&HashTable->NumEnumerators);
		RtlZeroMemory(Enumerator, sizeof(RTL_DYNAMIC_HASH_TABLE_ENUMERATOR));
	}
}

PRTL_DYNAMIC_HASH_TABLE_ENTRY EnumerateEntryHashTable(
	IN		PRTL_DYNAMIC_HASH_TABLE				HashTable,
	IN OUT	PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR	Enumerator)
{
	if (!HashTable || !Enumerator) return NULL;

	while (Enumerator->BucketIndex < HashTable->TableSize) {
		PLIST_ENTRY NextChain;

		if (Enumerator->ChainHead) {
			PLIST_ENTRY Current = Enumerator->HashEntry.Linkage.Flink;
			PLIST_ENTRY Chain = Enumerator->ChainHead;

			if (Current != Chain) {
				PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry = CONTAINING_RECORD(
					Current,
					RTL_DYNAMIC_HASH_TABLE_ENTRY,
					Linkage);
				Enumerator->HashEntry.Linkage.Flink = Current->Flink;
				return Entry;
			}
		}

		Enumerator->BucketIndex++;
		if (Enumerator->BucketIndex >= HashTable->TableSize) break;

		NextChain = &((PLIST_ENTRY)HashTable->Directory)[Enumerator->BucketIndex];
		if (!IsListEmpty(NextChain)) {
			Enumerator->ChainHead = NextChain;
			Enumerator->HashEntry.Linkage.Flink = NextChain->Flink;
		}
	}
	return NULL;
}

BOOLEAN InsertEntryHashTable(
	IN		PRTL_DYNAMIC_HASH_TABLE			HashTable,
	IN		PRTL_DYNAMIC_HASH_TABLE_ENTRY	Entry,
	IN		ULONG_PTR						Signature,
	IN OUT	PRTL_DYNAMIC_HASH_TABLE_CONTEXT	Context OPTIONAL)
{
	PLIST_ENTRY Chain;
	BOOLEAN WasEmpty;

	if (!HashTable || !Entry) return FALSE;

	Chain = &((PLIST_ENTRY)HashTable->Directory)[Signature & HashTable->DivisorMask];
	WasEmpty = IsListEmpty(Chain);

	Entry->Signature = Signature;
	InsertHeadList(Chain, &Entry->Linkage);

	HashTable->NumEntries++;
	if (WasEmpty) HashTable->NonEmptyBuckets++;

	if (HashTable->NumEntries > HashTable->TableSize * 2) {
		ULONG NewShift = HashTable->Shift + 1;
		ULONG NewSize = 1 << NewShift;
		PLIST_ENTRY NewDir = SafeAlloc(LIST_ENTRY, NewSize);

		if (NewDir) {
			ULONG Index;
			ULONG OldBucket;
			for (Index = 0; Index < NewSize; ++Index) {
				InitializeListHead(&NewDir[Index]);
			}

			for (OldBucket = 0; OldBucket < HashTable->TableSize; ++OldBucket) {
				PLIST_ENTRY OldChain = &((PLIST_ENTRY)HashTable->Directory)[OldBucket];
				while (!IsListEmpty(OldChain)) {
					PLIST_ENTRY Entry = RemoveHeadList(OldChain);
					PRTL_DYNAMIC_HASH_TABLE_ENTRY Item = CONTAINING_RECORD(
						Entry,
						RTL_DYNAMIC_HASH_TABLE_ENTRY,
						Linkage);
					ULONG NewBucket = Item->Signature & (NewSize - 1);
					InsertHeadList(&NewDir[NewBucket], Entry);
				}
			}

			SafeFree(HashTable->Directory);
			HashTable->Directory = NewDir;
			HashTable->Shift = NewShift;
			HashTable->TableSize = NewSize;
			HashTable->DivisorMask = NewSize - 1;
		}
	}

	if (Context) {
		Context->ChainHead = Chain;
		Context->PrevLinkage = &Entry->Linkage;
		Context->Signature = Signature;
	}

	return TRUE;
}

PRTL_DYNAMIC_HASH_TABLE_ENTRY LookupEntryHashTable(
	IN	PRTL_DYNAMIC_HASH_TABLE			HashTable,
	IN	ULONG_PTR						Signature,
	OUT	PRTL_DYNAMIC_HASH_TABLE_CONTEXT	Context OPTIONAL)
{
	PLIST_ENTRY Entry;
	PLIST_ENTRY Chain;

	if (!HashTable) return NULL;
	
	Chain = &((PLIST_ENTRY)HashTable->Directory)[Signature & HashTable->DivisorMask];

	for (Entry = Chain->Flink; Entry != Chain; Entry = Entry->Flink) {
		PRTL_DYNAMIC_HASH_TABLE_ENTRY Item = CONTAINING_RECORD(
			Entry,
			RTL_DYNAMIC_HASH_TABLE_ENTRY,
			Linkage);
		if (Item->Signature == Signature) {
			if (Context) {
				Context->ChainHead = Chain;
				Context->PrevLinkage = Entry->Blink;
				Context->Signature = Signature;
			}
			return Item;
		}
	}
	return NULL;
}

PRTL_DYNAMIC_HASH_TABLE_ENTRY GetNextEntryHashTable(
	IN	PRTL_DYNAMIC_HASH_TABLE			HashTable,
	IN	PRTL_DYNAMIC_HASH_TABLE_CONTEXT	Context)
{
	PLIST_ENTRY Current;
	PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry;

	if (!HashTable || !Context) return NULL;
	Current = Context->PrevLinkage->Flink;
	if (Current == Context->ChainHead) return NULL;

	Entry = CONTAINING_RECORD(Current, RTL_DYNAMIC_HASH_TABLE_ENTRY, Linkage);
	Context->PrevLinkage = Current;
	return Entry;
}

BOOLEAN RemoveEntryHashTable(
	IN		PRTL_DYNAMIC_HASH_TABLE			HashTable,
	IN		PRTL_DYNAMIC_HASH_TABLE_ENTRY	Entry,
	IN OUT	PRTL_DYNAMIC_HASH_TABLE_CONTEXT	Context OPTIONAL)
{
	PLIST_ENTRY Chain;
	PLIST_ENTRY Current;
	PLIST_ENTRY EntryLink;

	if (!HashTable || !Entry) return FALSE;

	Chain = Context ? Context->ChainHead : &((PLIST_ENTRY)HashTable->Directory)[Entry->Signature & HashTable->DivisorMask];

	if (IsListEmpty(Chain)) return FALSE;

	EntryLink = &Entry->Linkage;
	for (Current = Chain->Flink; Current != Chain; Current = Current->Flink) {
		if (Current == EntryLink) {
			RemoveEntryList(Current);
			HashTable->NumEntries--;
			if (IsListEmpty(Chain)) HashTable->NonEmptyBuckets--;
			return TRUE;
		}
	}
	return FALSE;
}

//
// Create a new string mapper.
//
//   StringMapper
//     Pointer that receives the address of the opaque string
//     mapper object.
//
//   Flags
//     May contain any of the KEX_RTL_STRING_MAPPER_* flags.
//     Invalid flags will cause STATUS_INVALID_PARAMETER_2.
//
KEXAPI NTSTATUS NTAPI KexRtlCreateStringMapper(
	OUT		PPKEX_RTL_STRING_MAPPER		StringMapper,
	IN		ULONG						Flags OPTIONAL)
{
	PKEX_RTL_STRING_MAPPER Mapper;
	PRTL_DYNAMIC_HASH_TABLE HashTable;
	BOOLEAN Success;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	*StringMapper = NULL;

	if (Flags & ~(KEX_RTL_STRING_MAPPER_FLAGS_VALID_MASK)) {
		return STATUS_INVALID_PARAMETER_2;
	}

	Mapper = SafeAlloc(KEX_RTL_STRING_MAPPER, 1);
	if (!Mapper) {
		return STATUS_NO_MEMORY;
	}

	HashTable = &Mapper->HashTable;
	Success = CreateHashTable(&HashTable, 0, 0);
	if (!Success) {
		// The only way RtlCreateHashTable can fail is by running out of memory.
		// (Or an invalid parameter, but that won't happen to us.)
		SafeFree(Mapper);
		return STATUS_NO_MEMORY;
	}

	Mapper->Flags = Flags;
	*StringMapper = Mapper;

	return STATUS_SUCCESS;
}

//
// Delete a string mapper and free its resources.
//
//   StringMapper
//     Pointer to the opaque string mapper object.
//
KEXAPI NTSTATUS NTAPI KexRtlDeleteStringMapper(
	IN		PPKEX_RTL_STRING_MAPPER		StringMapper)
{
	PKEX_RTL_STRING_MAPPER Mapper;
	RTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator;
	PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry;

	if (!StringMapper || !*StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	Mapper = *StringMapper;

	//
	// Enumerate entries in the hash table and free all the memory.
	//

	InitEnumerationHashTable(&Mapper->HashTable, &Enumerator);

	do {
		Entry = EnumerateEntryHashTable(&Mapper->HashTable, &Enumerator);
		RtlFreeHeap(RtlProcessHeap(), 0, Entry);
	} until (Entry == NULL);

	EndEnumerationHashTable(&Mapper->HashTable, &Enumerator);

	//
	// Free the hash table itself.
	//

	DeleteHashTable(&(*StringMapper)->HashTable);
	SafeFree(*StringMapper);

	return STATUS_SUCCESS;
}

//
// Insert a new key-value pair into the string mapper.
//
// The UNICODE_STRING structures themselves are copied into the string
// mapper, but the actual string data that is pointed to by the Buffer
// member is not managed by the mapper - you must ensure that this data
// is not freed before you destroy the string mapper.
//
//   StringMapper
//     Pointer to a string mapper object
//
//   Key
//     A string which will be hashed. Do not specify a key which is a
//     duplicate of another key you have inserted earlier; this will make
//     lookups unreliable (may return any of the values associated with
//     the same key).
//
//   Value
//     Pointer to an uninterpreted UNICODE_STRING which can be retrieved
//     if you know the Key, using the KexRtlLookupEntryStringMapper API.
//
KEXAPI NTSTATUS NTAPI KexRtlInsertEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER		StringMapper,
	IN		PCUNICODE_STRING			Key,
	IN		PCUNICODE_STRING			Value)
{
	PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY Entry;
	ULONG KeySignature;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!Key) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!Value) {
		return STATUS_INVALID_PARAMETER_3;
	}

	Entry = SafeAlloc(KEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY, 1);
	if (!Entry) {
		return STATUS_NO_MEMORY;
	}

	//
	// hash the key and use that as the hash table entry's signature.
	// No need to check RtlHashUnicodeString return value because it can't
	// fail unless an invalid parameter is supplied, which cannot happen.
	//
	RtlHashUnicodeString(
		Key,
		(StringMapper->Flags & KEX_RTL_STRING_MAPPER_CASE_INSENSITIVE_KEYS),
		HASH_STRING_ALGORITHM_DEFAULT,
		&KeySignature);

	Entry->Key = *Key;
	Entry->Value = *Value;

	InsertEntryHashTable(
		&StringMapper->HashTable,
		&Entry->HashTableEntry,
		KeySignature,
		NULL);

	return STATUS_SUCCESS;
}

STATIC NTSTATUS NTAPI KexRtlpLookupRawEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER						StringMapper,
	IN		PCUNICODE_STRING							Key,
	OUT		PPKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY	EntryOut)
{
	BOOLEAN Success;
	BOOLEAN CaseInsensitive;
	PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY Entry;
	RTL_DYNAMIC_HASH_TABLE_CONTEXT Context;
	ULONG KeySignature;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!Key) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!EntryOut) {
		return STATUS_INTERNAL_ERROR;
	}

	CaseInsensitive = (StringMapper->Flags & KEX_RTL_STRING_MAPPER_CASE_INSENSITIVE_KEYS);

	RtlHashUnicodeString(
		Key,
		CaseInsensitive,
		HASH_STRING_ALGORITHM_DEFAULT,
		&KeySignature);

	Entry = (PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY) LookupEntryHashTable(
		&StringMapper->HashTable,
		KeySignature,
		&Context);

	while (TRUE) {
		if (!Entry || Entry->HashTableEntry.Signature != KeySignature) {
			*EntryOut = NULL;
			return STATUS_STRING_MAPPER_ENTRY_NOT_FOUND;
		}

		Success = RtlEqualUnicodeString(
			Key,
			&Entry->Key,
			CaseInsensitive);

		if (Success) {
			break;
		}

		//
		// If this loop continues more than once, that means there is a hash collision.
		// We will handle the situation by checking the rest of the entries in this
		// bucket.
		//
		// With a decent hash function, this code should very rarely be executed.
		//

		Entry = (PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY) GetNextEntryHashTable(
			&StringMapper->HashTable,
			&Context);
	}

	*EntryOut = Entry;

	return STATUS_SUCCESS;
}

//
// Look up a single value by key.
//
//   StringMapper
//     Pointer to a string mapper object
//
//   Key
//     Should be the same as what you specified to an earlier insert call.
//     If no value with the specified key is found, this function will return
//     the STATUS_STRING_MAPPER_ENTRY_NOT_FOUND error code.
//
//   Value
//     Pointer to a structure which will receive the retrieved value data.
//
KEXAPI NTSTATUS NTAPI KexRtlLookupEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN		PCUNICODE_STRING				Key,
	OUT		PUNICODE_STRING					Value OPTIONAL)
{
	NTSTATUS Status;
	PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY Entry;

	Status = KexRtlpLookupRawEntryStringMapper(
		StringMapper,
		Key,
		&Entry);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	if (Value) {
		*Value = Entry->Value;
	}

	return Status;
}

//
// Remove a single value by key.
//
//   StringMapper
//     Pointer to a string mapper object
//
//   Key
//     Should be the same as what you specified to an earlier insert call.
//     If no value with the specified key is found, this function will return
//     the STATUS_STRING_MAPPER_ENTRY_NOT_FOUND error code.
//
KEXAPI NTSTATUS NTAPI KexRtlRemoveEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN		PCUNICODE_STRING				Key)
{
	NTSTATUS Status;
	BOOLEAN Success;
	PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY Entry;

	Status = KexRtlpLookupRawEntryStringMapper(
		StringMapper,
		Key,
		&Entry);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Success = RemoveEntryHashTable(
		&StringMapper->HashTable,
		&Entry->HashTableEntry,
		NULL);

	SafeFree(Entry);

	if (!Success) {
		return STATUS_INTERNAL_ERROR;
	}

	return STATUS_SUCCESS;
}

//
// This is a convenience function which takes input and returns output
// from a single UNICODE_STRING structure. It is equivalent to calling
// KexRtlLookupEntryStringMapper with the Key and Value parameters pointing
// to the same UNICODE_STRING.
//
KEXAPI NTSTATUS NTAPI KexRtlApplyStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN OUT	PUNICODE_STRING					KeyToValue)
{
	return KexRtlLookupEntryStringMapper(StringMapper, KeyToValue, KeyToValue);
}

//
// This is a convenience function which inserts many entries into the
// mapper with a single call. It is intended to be used with static arrays.
//
KEXAPI NTSTATUS NTAPI KexRtlInsertMultipleEntriesStringMapper(
	IN		PKEX_RTL_STRING_MAPPER				StringMapper,
	IN		CONST KEX_RTL_STRING_MAPPER_ENTRY	Entries[],
	IN		ULONG								EntryCount)
{
	NTSTATUS FailureStatus;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!Entries) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!EntryCount) {
		return STATUS_INVALID_PARAMETER_3;
	}

	FailureStatus = STATUS_SUCCESS;

	do {
		NTSTATUS Status;

		Status = KexRtlInsertEntryStringMapper(
			StringMapper,
			&Entries[EntryCount-1].Key,
			&Entries[EntryCount-1].Value);

		if (!NT_SUCCESS(Status)) {
			FailureStatus = Status;
		}
	} while (--EntryCount);

	return FailureStatus;
}

KEXAPI NTSTATUS NTAPI KexRtlLookupMultipleEntriesStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN OUT	KEX_RTL_STRING_MAPPER_ENTRY		Entries[],
	IN		ULONG							EntryCount)
{
	NTSTATUS FailureStatus;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!Entries) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!EntryCount) {
		return STATUS_INVALID_PARAMETER_3;
	}

	FailureStatus = STATUS_SUCCESS;

	do {
		NTSTATUS Status;

		Status = KexRtlLookupEntryStringMapper(
			StringMapper,
			&Entries[EntryCount-1].Key,
			&Entries[EntryCount-1].Value);

		if (!NT_SUCCESS(Status)) {
			FailureStatus = Status;
		}
	} while (--EntryCount);

	return FailureStatus;
}

KEXAPI NTSTATUS NTAPI KexRtlBatchApplyStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN OUT	UNICODE_STRING					KeyToValue[],
	IN		ULONG							KeyToValueCount)
{
	NTSTATUS FailureStatus;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!KeyToValue) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!KeyToValueCount) {
		return STATUS_INVALID_PARAMETER_3;
	}

	FailureStatus = STATUS_SUCCESS;

	do {
		NTSTATUS Status;

		Status = KexRtlApplyStringMapper(
			StringMapper,
			&KeyToValue[KeyToValueCount-1]);

		if (!NT_SUCCESS(Status)) {
			FailureStatus = Status;
		}
	} while (--KeyToValueCount);

	return FailureStatus;
}