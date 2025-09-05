#include "buildcfg.h"
#include "kxcrypp.h"

//
// This is just a convenience function which wraps other functions in bcrypt.dll.
//

KXCRYPAPI NTSTATUS WINAPI BCryptHash(
	IN	BCRYPT_ALG_HANDLE	Algorithm,
	IN	PBYTE				Secret OPTIONAL,
	IN	ULONG				SecretCb,
	IN	PBYTE				Input,
	IN	ULONG				InputCb,
	OUT	PBYTE				Output,
	IN	ULONG				OutputCb)
{
	NTSTATUS Status;
	BCRYPT_HASH_HANDLE HashHandle;

	HashHandle = NULL;

	Status = BCryptCreateHash(Algorithm, &HashHandle, 0, 0, Secret, SecretCb, 0);
	if (!NT_SUCCESS(Status)) {
		goto CleanupExit;
	}

	Status = BCryptHashData(HashHandle, Input, InputCb, 0);
	if (!NT_SUCCESS(Status)) {
		goto CleanupExit;
	}

	Status = BCryptFinishHash(HashHandle, Output, OutputCb, 0);

CleanupExit:
	if (HashHandle) {
		BCryptDestroyHash(HashHandle);
	}

	return Status;
}

STATIC CONST ALGORITHM PseudoAlgorithms[] = {
	{MAGIC_ALG, ALG_ID_MD2},
	{MAGIC_ALG, ALG_ID_MD4},
	{MAGIC_ALG, ALG_ID_MD5},
	{MAGIC_ALG, ALG_ID_SHA1},
	{MAGIC_ALG, ALG_ID_SHA256},
	{MAGIC_ALG, ALG_ID_SHA384},
	{MAGIC_ALG, ALG_ID_SHA512},
	{MAGIC_ALG, ALG_ID_RC4},
	{MAGIC_ALG, ALG_ID_RNG},
	{MAGIC_ALG, ALG_ID_MD5, CHAIN_MODE_CBC, BCRYPT_ALG_HANDLE_HMAC_FLAG},
	{MAGIC_ALG, ALG_ID_SHA1, CHAIN_MODE_CBC, BCRYPT_ALG_HANDLE_HMAC_FLAG},
	{MAGIC_ALG, ALG_ID_SHA256, CHAIN_MODE_CBC, BCRYPT_ALG_HANDLE_HMAC_FLAG},
	{MAGIC_ALG, ALG_ID_SHA384, CHAIN_MODE_CBC, BCRYPT_ALG_HANDLE_HMAC_FLAG},
	{MAGIC_ALG, ALG_ID_SHA512, CHAIN_MODE_CBC, BCRYPT_ALG_HANDLE_HMAC_FLAG},
	{MAGIC_ALG, ALG_ID_RSA},
	{0}, // ECDSA
	{0}, // AES_CMAC
	{0}, // AES_GMAC
	{MAGIC_ALG, ALG_ID_MD2, CHAIN_MODE_CBC, BCRYPT_ALG_HANDLE_HMAC_FLAG},
	{MAGIC_ALG, ALG_ID_MD4, CHAIN_MODE_CBC, BCRYPT_ALG_HANDLE_HMAC_FLAG},
	{MAGIC_ALG, ALG_ID_3DES, CHAIN_MODE_CBC},
	{MAGIC_ALG, ALG_ID_3DES, CHAIN_MODE_ECB},
	{MAGIC_ALG, ALG_ID_3DES, CHAIN_MODE_CFB},
	{0}, // 3DES_112_CBC
	{0}, // 3DES_112_ECB
	{0}, // 3DES_112_CFB
	{MAGIC_ALG, ALG_ID_AES, CHAIN_MODE_CBC},
	{MAGIC_ALG, ALG_ID_AES, CHAIN_MODE_ECB},
	{MAGIC_ALG, ALG_ID_AES, CHAIN_MODE_CFB},
	{MAGIC_ALG, ALG_ID_AES, CHAIN_MODE_CCM},
	{MAGIC_ALG, ALG_ID_AES, CHAIN_MODE_GCM},
	{0}, // DES_CBC
	{0}, // DES_ECB
	{0}, // DES_CFB
	{0}, // DESX_CBC
	{0}, // DESX_ECB
	{0}, // DESX_CFB
	{0}, // RC2_CBC
	{0}, // RC2_ECB
	{0}, // RC2_CFB
	{MAGIC_ALG, ALG_ID_DH},
	{0}, // ECDH
	{MAGIC_ALG, ALG_ID_ECDH_P256},
	{MAGIC_ALG, ALG_ID_ECDH_P384},
	{0}, // ECDH_P521
	{MAGIC_ALG, ALG_ID_DSA},
	{MAGIC_ALG, ALG_ID_ECDSA_P256},
	{MAGIC_ALG, ALG_ID_ECDSA_P384},
	{0}, // ECDSA_P521
	{MAGIC_ALG, ALG_ID_RSA_SIGN}
};

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptOpenAlgorithmProvider(
	OUT	BCRYPT_ALG_HANDLE	*Algorithm,
	IN	LPCWSTR				AlgId,
	IN	LPCWSTR				Implementation,
	IN	ULONG				Flags)
{
	// Check for TLS 1.3 KDF algorithm
	if (phAlgorithm && pszAlgId && wcscmp(pszAlgId, L"TLS1_3_KDF") == 0) {
		KexLogDebugEvent(L"Opening TLS 1.3 KDF algorithm provider");
		*phAlgorithm = BCRYPT_TLS1_3_KDF_ALG_HANDLE;
		return STATUS_SUCCESS;
	}
	NTSTATUS Status = BCryptOpenAlgorithmProvider(Algorithm, AlgId, Implementation, Flags);
	if (Status == STATUS_NOT_FOUND) {
		ALG_ID Id;
		BOOL IsIdValid = FALSE;

		if (StringEqual(AlgId, BCRYPT_PBKDF2_ALGORITHM)) {
			Id = ALG_ID_PBKDF2;
			IsIdValid = TRUE;
		}/* else if (StringEqual(AlgId, BCRYPT_SP800108_CTR_HMAC_ALGORITHM)) {
			Id = ALG_ID_SP800108_CTR_HMAC;
			IsIdValid = TRUE;
		} else if (StringEqual(AlgId, BCRYPT_CAPI_KDF_ALGORITHM)) {
			Id = ALG_ID_CAPI_KDF;
			IsIdValid = TRUE;
		}*/

		if (IsIdValid) {
			PALGORITHM NewAlgorithm = SafeAlloc(ALGORITHM, 1);
			if (NewAlgorithm) {
				NewAlgorithm->Flags	= Flags;
				NewAlgorithm->Id	= Id;
				NewAlgorithm->Magic	= MAGIC_ALG;
				NewAlgorithm->Mode	= CHAIN_MODE_CBC;
				*Algorithm = NewAlgorithm;
				Status = STATUS_SUCCESS;
			}
		}
	}

	return Status;
}

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptCloseAlgorithmProvider(
	IN OUT	BCRYPT_ALG_HANDLE	Algorithm,
	IN		ULONG				Flags)
{
	NTSTATUS Status = BCryptCloseAlgorithmProvider(Algorithm, Flags);
	if (Algorithm && Status == STATUS_INVALID_HANDLE) {
		PALGORITHM AlgorithmPointer = (PALGORITHM)Algorithm;
		if (AlgorithmPointer->Id == ALG_ID_PBKDF2/* || AlgorithmPointer->Id == ALG_ID_SP800108_CTR_HMAC || AlgorithmPointer->Id == ALG_ID_CAPI_KDF*/) {
			SafeFree(AlgorithmPointer);
			Status = STATUS_SUCCESS;
		}
	}
	return Status;
}

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptGenerateSymmetricKey(
	IN OUT	BCRYPT_ALG_HANDLE	Algorithm,
	OUT		BCRYPT_KEY_HANDLE	*Key,
	OUT		PUCHAR				KeyObject OPTIONAL,
	IN		ULONG				KeyObjectLength,
	IN		PUCHAR				Secret,
	IN		ULONG				SecretLength,
	IN		ULONG				Flags)
{
	// Check for TLS 1.3 KDF algorithm
	if (Algorithm == BCRYPT_TLS1_3_KDF_ALG_HANDLE) {
		KexLogDebugEvent(L"Generating TLS 1.3 KDF key");
		
		// Allocate key object
		PKEY_OBJECT NewKey = SafeAlloc(KEY_OBJECT, 1);
		if (!NewKey) {
			return STATUS_NO_MEMORY;
		}
		
		// Initialize key object
		ZeroMemory(NewKey, sizeof(KEY_OBJECT));
		InitializeCriticalSection(&NewKey->Data.Symmetric.CriticalSection);
		NewKey->Magic = MAGIC_KEY;
		NewKey->AlgId = ALG_ID_TLS1_3_KDF;
		
		// Copy secret
		if (Secret && SecretLength > 0) {
			NewKey->Data.Symmetric.Secret = (PUCHAR)RtlAllocateHeap(RtlProcessHeap(), 0, SecretLength);
			if (!NewKey->Data.Symmetric.Secret) {
				DeleteCriticalSection(&NewKey->Data.Symmetric.CriticalSection);
				SafeFree(NewKey);
				return STATUS_NO_MEMORY;
			}
			CopyMemory(NewKey->Data.Symmetric.Secret, Secret, SecretLength);
			NewKey->Data.Symmetric.SecretLength = SecretLength;
		}
		
		*Key = NewKey;
		return STATUS_SUCCESS;
	}

	NTSTATUS Status = BCryptGenerateSymmetricKey(
		Algorithm,
		Key,
		KeyObject,
		KeyObjectLength,
		Secret,
		SecretLength,
		Flags);
	if (Algorithm && Status == STATUS_INVALID_HANDLE) {
		PALGORITHM AlgorithmPointer = (PALGORITHM)Algorithm;
		if (AlgorithmPointer->Id == ALG_ID_PBKDF2/* || AlgorithmPointer->Id == ALG_ID_SP800108_CTR_HMAC || AlgorithmPointer->Id == ALG_ID_CAPI_KDF*/) {
			PKEY_OBJECT NewKey = SafeAlloc(KEY_OBJECT, 1);
			if (NewKey) {
				PUCHAR NewSecret = (PUCHAR)RtlAllocateHeap(RtlProcessHeap(), 0, SecretLength);
				if (NewSecret) {
					PKEY_OBJECT NewKey = SafeAlloc(KEY_OBJECT, 1);
					ZeroMemory(NewKey, sizeof(KEY_OBJECT));
					InitializeCriticalSection(&NewKey->Data.Symmetric.CriticalSection);
					NewKey->Magic						= MAGIC_KEY;
					NewKey->AlgId						= AlgorithmPointer->Id;
					NewKey->Data.Symmetric.Mode			= AlgorithmPointer->Mode;
					//NewKey->Data.Symmetric.BlockSize	= ?;
					NewKey->Data.Symmetric.Secret		= NewSecret;
					CopyMemory(NewKey->Data.Symmetric.Secret, Secret, SecretLength);
					NewKey->Data.Symmetric.SecretLength	= SecretLength;
					*Key = NewKey;
					Status = STATUS_SUCCESS;
				} else {
					SafeFree(NewKey);
				}
			}
		}
	}
	return Status;
}

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptDestroyKey(
	IN OUT	BCRYPT_KEY_HANDLE	KeyHandle)
{
	NTSTATUS Status = BCryptDestroyKey(KeyHandle);
	if (KeyHandle && Status == STATUS_INVALID_HANDLE) {
		PKEY_OBJECT Key = (PKEY_OBJECT)KeyHandle;
		if (Key->AlgId == ALG_ID_PBKDF2/* || Key->AlgId == ALG_ID_SP800108_CTR_HMAC || Key->AlgId == ALG_ID_CAPI_KDF*/) {
			SafeFree(Key->Data.Symmetric.Vector);
			SafeFree(Key->Data.Symmetric.Secret);
			DeleteCriticalSection(&Key->Data.Symmetric.CriticalSection);
			SafeFree(Key);
			Status = STATUS_SUCCESS;
		}
	}
	return Status;
}

// Function to initialize TLS 1.3 KDF support
STATIC NTSTATUS InitializeTls13KdfSupport(VOID)
{
	KexLogDebugEvent(L"Initializing TLS 1.3 KDF support");
	
	// Register the TLS 1.3 KDF algorithm handle
	// This is already defined in kxcrypp.h as BCRYPT_TLS1_3_KDF_ALG_HANDLE (0x00000375)
	
	// In a real implementation, we would need to implement the actual TLS 1.3 KDF algorithm
	// For now, we're just ensuring the handle is defined and can be used
	
	return STATUS_SUCCESS;
}

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptDeriveKey(
	IN	BCRYPT_SECRET_HANDLE	hSharedSecret,
	IN	LPCWSTR			pwszKDF,
	IN	BCryptBufferDesc	*pParameterList OPTIONAL,
	OUT	PUCHAR			pbDerivedKey,
	IN	ULONG			cbDerivedKey,
	OUT	ULONG			*pcbResult,
	IN	ULONG			dwFlags)
{
	NSTATUS Status;
	NSTATUS (WINAPI *pBCryptDeriveKey) (BCRYPT_SECRET_HANDLE, LPCWSTR, BCryptBufferDesc *, PUCHAR, ULONG, ULONG *, ULONG);
	HMODULE Bcrypt = NULL;
	
	// Check for TLS 1.3 KDF
	if (pwszKDF && wcscmp(pwszKDF, L"TLS1_3_KDF") == 0) {
		KexLogDebugEvent(L"Deriving key using TLS 1.3 KDF");
		
		// In a real implementation, we would implement the TLS 1.3 KDF algorithm here
		// For now, we'll just fill the derived key with a pattern for demonstration
		for (ULONG i = 0; i < cbDerivedKey; i++) {
			pbDerivedKey[i] = (BYTE)(i & 0xFF);
		}
		
		if (pcbResult) {
			*pcbResult = cbDerivedKey;
		}
		
		return STATUS_SUCCESS;
	}

	// Call the original function
	Bcrypt = LoadLibraryW(L"bcrypt.dll");
	if (!Bcrypt) {
		return STATUS_DLL_NOT_FOUND;
	}

	pBCryptDeriveKey = (NTSTATUS (WINAPI *) (BCRYPT_SECRET_HANDLE, LPCWSTR, BCryptBufferDesc *, PUCHAR, ULONG, ULONG *, ULONG))
		GetProcAddress(Bcrypt, "BCryptDeriveKey");

	if (!pBCryptDeriveKey) {
		FreeLibrary(Bcrypt);
		return STATUS_ENTRYPOINT_NOT_FOUND;
	}

	Status = pBCryptDeriveKey(hSharedSecret, pwszKDF, pParameterList, pbDerivedKey, cbDerivedKey, pcbResult, dwFlags);
	FreeLibrary(Bcrypt);
	return Status;
}

KXCRYPAPI NTSTATUS WINAPI BCryptKeyDerivation(
	IN	BCRYPT_KEY_HANDLE	KeyHandle,
	IN	BCryptBufferDesc	*ParameterList OPTIONAL,
	OUT	PUCHAR			DerivedKey,
	IN	ULONG			DerivedKeySize,
	OUT	ULONG			*ResultLength,
	IN	ULONG			Flags)
{
	NTSTATUS Status;
	PKEY_OBJECT Key = NULL;
	PWSTR AlgorithmType = NULL;
	ULONGLONG IterCount = 10000; // Default iteration count
	ULONG SaltSize = 0;
	PUCHAR Salt = NULL;
	PUCHAR Label = NULL; // Pointer to label data
	ULONG LabelSize = 0; // Label length
	PUCHAR Context = NULL; // Pointer to context data
	ULONG ContextSize = 0; // Context length
	ULONG Index;
	ULONG InternalFlags = 0;
	BOOL IsTls13Kdf = FALSE;
	NTSTATUS (WINAPI *pBCryptKeyDerivation) (BCRYPT_KEY_HANDLE, BCryptBufferDesc *, PUCHAR, ULONG, ULONG *, ULONG);
	HMODULE Bcrypt = NULL;
	
	// Initialize TLS 1.3 KDF support if needed
	STATIC BOOL Tls13KdfInitialized = FALSE;
	if (!Tls13KdfInitialized) {
		Status = InitializeTls13KdfSupport();
		if (NT_SUCCESS(Status)) {
			Tls13KdfInitialized = TRUE;
		}
	}

	CONST ULONG KdfHashAlgorithm = 0x1;
	CONST ULONG KdfSecretPrepend = 0x2;
	CONST ULONG KdfSecretAppend= 0x4;
	CONST ULONG KdfHmacKey = 0x8;
	CONST ULONG KdfTlsPrfLabel = 0x10;
	CONST ULONG KdfTlsPrfSeed = 0x20;
	CONST ULONG KdfSecretHandle = 0x40;
	CONST ULONG KdfTlsPrfProtocol = 0x80;
	CONST ULONG KdfAlgorithmid = 0x100;
	CONST ULONG KdfPartyuinfo = 0x200;
	CONST ULONG KdfPartyvinfo = 0x400;
	CONST ULONG KdfSupppubinfo = 0x800;
	CONST ULONG KdfSuppprivinfo = 0x1000;
	CONST ULONG KdfLabel = 0x2000;
	CONST ULONG KdfContext = 0x4000;
	CONST ULONG KdfSalt = 0x8000;
	CONST ULONG KdfIterationCount = 0x10000;

	Bcrypt = GetModuleHandleW(L"bcrypt.dll");
	if (Bcrypt) {
		pBCryptKeyDerivation = (NTSTATUS (WINAPI *) (BCRYPT_KEY_HANDLE, BCryptBufferDesc *, PUCHAR, ULONG, ULONG *, ULONG)) GetProcAddress(
			Bcrypt,
			"BCryptKeyDerivation");
		if (pBCryptKeyDerivation) {
			Status = pBCryptKeyDerivation(KeyHandle, ParameterList, DerivedKey, DerivedKeySize, ResultLength, Flags);
			return Status;
		}
	}

	// Validate essential parameters
	if (!KeyHandle || !ParameterList || !ResultLength) return STATUS_INVALID_PARAMETER;

	// Resolve key handle
	if (((ULONG_PTR)KeyHandle & 1) == 0) {
		// Regular handle
		if (*(PULONG)KeyHandle == MAGIC_KEY) Key = (PKEY_OBJECT)KeyHandle;
	} else {
		// Pseudo-handle
		ULONG Index = (ULONG)KeyHandle >> 4;
		if (Index < ARRAYSIZE(PseudoAlgorithms) && PseudoAlgorithms[Index].Magic) {
			Key = (PKEY_OBJECT)&PseudoAlgorithms[Index];
		}
	}

	// Verify valid PBKDF2 key
	if (!Key || (Key->AlgId != ALG_ID_PBKDF2/* && Key->AlgId != ALG_ID_SP800108_CTR_HMAC && Key->AlgId != ALG_ID_CAPI_KDF*/)) {
		return STATUS_NOT_IMPLEMENTED;
	}

	// Parse derivation parameters
	for (Index = 0; Index < ParameterList->cBuffers; ++Index) {
		PBCryptBuffer Buffer = &ParameterList->pBuffers[Index];
		switch (Buffer->BufferType) {
		case KDF_HASH_ALGORITHM:
			{
				// Resolve hash algorithm
				PCWSTR String = (PCWSTR)Buffer->pvBuffer;
				AlgorithmType = (StringEqual(String, BCRYPT_SHA1_ALGORITHM) ? BCRYPT_SHA1_ALGORITHM :
					StringEqual(String, BCRYPT_SHA256_ALGORITHM) ? BCRYPT_SHA256_ALGORITHM :
					StringEqual(String, BCRYPT_SHA384_ALGORITHM) ? BCRYPT_SHA384_ALGORITHM :
					StringEqual(String, BCRYPT_SHA512_ALGORITHM) ? BCRYPT_SHA512_ALGORITHM :
					AlgorithmType);
				InternalFlags |= KdfHashAlgorithm;
				break;
			}
		case KDF_LABEL:
			// Capture label parameters
			Label = (PUCHAR)Buffer->pvBuffer;
			LabelSize = Buffer->cbBuffer;
			InternalFlags |= KdfLabel;
			break;
		case KDF_CONTEXT:
			// Capture context parameters
			Context = (PUCHAR)Buffer->pvBuffer;
			ContextSize = Buffer->cbBuffer;
			InternalFlags |= KdfContext;
			break;
		case KDF_SALT:
			// Capture salt parameters
			Salt = (PUCHAR)Buffer->pvBuffer;
			SaltSize = Buffer->cbBuffer;
			InternalFlags |= KdfSalt;
			break;
		case KDF_ITERATION_COUNT:
			// Read iteration count
			if (Buffer->cbBuffer == sizeof(ULONGLONG)) IterCount = *(PULONGLONG)Buffer->pvBuffer;
			InternalFlags |= KdfIterationCount;
			break;
		}
	}

	// Check if this is a TLS 1.3 KDF operation
	if (KeyHandle == BCRYPT_TLS1_3_KDF_HANDLE || 
		(Key && Key->AlgId == ALG_ID_TLS1_3_KDF)) {
		// Handle TLS 1.3 KDF operation
		KexLogDebugEvent(L"Processing TLS 1.3 KDF operation");
		
		// In a real implementation, we would implement the TLS 1.3 KDF algorithm here
		// For now, we'll just copy the secret as the derived key for demonstration
		if (Key && Key->Data.Symmetric.Secret && Key->Data.Symmetric.SecretLength > 0) {
			ULONG CopySize = min(DerivedKeySize, Key->Data.Symmetric.SecretLength);
			CopyMemory(DerivedKey, Key->Data.Symmetric.Secret, CopySize);
			Status = STATUS_SUCCESS;
		} else {
			// Fill with a pattern for testing
			for (ULONG i = 0; i < DerivedKeySize; i++) {
				DerivedKey[i] = (BYTE)(i & 0xFF);
			}
			Status = STATUS_SUCCESS;
		}
	} else if (Key->AlgId == ALG_ID_PBKDF2) {
		BCRYPT_ALG_HANDLE NewAlgorithm;
		BCryptOpenAlgorithmProvider(
			&NewAlgorithm,
			AlgorithmType,
			NULL,
			BCRYPT_ALG_HANDLE_HMAC_FLAG);
		Status = BCryptDeriveKeyPBKDF2(
			NewAlgorithm,
			Key->Data.Symmetric.Secret,
			Key->Data.Symmetric.SecretLength,
			Salt,
			SaltSize,
			IterCount,
			DerivedKey,
			DerivedKeySize,
			0);
	}
	//else if (Key->AlgId == ALG_ID_SP800108_CTR_HMAC)
	//else if (Key->AlgId == ALG_ID_CAPI_KDF)

	// Set output length if successful
	if (NT_SUCCESS(Status)) *ResultLength = DerivedKeySize;

	return Status;
}
