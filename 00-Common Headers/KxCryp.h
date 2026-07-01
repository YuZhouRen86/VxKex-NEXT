///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KxCryp.h
//
// Abstract:
//
//     Public header file for KxCryp.
//
// Author:
//
//     vxiiduu (29-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               29-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef KXCRYPAPI
#  define KXCRYPAPI DECLSPEC_IMPORT
#endif

#include <bcrypt.h>
GEN_STD_TYPEDEFS(BCRYPT_ALG_HANDLE);
GEN_STD_TYPEDEFS(BCRYPT_KEY_HANDLE);
GEN_STD_TYPEDEFS(BCRYPT_HASH_HANDLE);
GEN_STD_TYPEDEFS(BCRYPT_RSAKEY_BLOB);
GEN_STD_TYPEDEFS(NCRYPT_KEY_HANDLE);

#define BCRYPT_SP800108_CTR_HMAC_ALGORITHM      L"SP800_108_CTR_HMAC"
#define BCRYPT_SP80056A_CONCAT_ALGORITHM        L"SP800_56A_CONCAT"
#define BCRYPT_PBKDF2_ALGORITHM                 L"PBKDF2"
#define BCRYPT_CAPI_KDF_ALGORITHM               L"CAPI_KDF"
#define BCRYPT_AES_CMAC_ALGORITHM				L"AES-CMAC"
#define BCRYPT_ECDH_ALGORITHM					L"ECDH"

#define BCRYPT_KDF_RAW_SECRET					L"TRUNCATE"
#define BCRYPT_KDF_HKDF							L"HKDF"

// Pseudo-BCRYPT_ALG_HANDLE values introduced in Win10.
// https://learn.microsoft.com/en-us/windows/win32/seccng/cng-algorithm-pseudo-handles

#define BCRYPT_MD2_ALG_HANDLE               ((BCRYPT_ALG_HANDLE)0x00000001)
#define BCRYPT_MD4_ALG_HANDLE               ((BCRYPT_ALG_HANDLE)0x00000011)
#define BCRYPT_MD5_ALG_HANDLE               ((BCRYPT_ALG_HANDLE)0x00000021)
#define BCRYPT_SHA1_ALG_HANDLE              ((BCRYPT_ALG_HANDLE)0x00000031)
#define BCRYPT_SHA256_ALG_HANDLE            ((BCRYPT_ALG_HANDLE)0x00000041)
#define BCRYPT_SHA384_ALG_HANDLE            ((BCRYPT_ALG_HANDLE)0x00000051)
#define BCRYPT_SHA512_ALG_HANDLE            ((BCRYPT_ALG_HANDLE)0x00000061)
#define BCRYPT_RC4_ALG_HANDLE               ((BCRYPT_ALG_HANDLE)0x00000071)
#define BCRYPT_RNG_ALG_HANDLE               ((BCRYPT_ALG_HANDLE)0x00000081)
#define BCRYPT_HMAC_MD5_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000091)
#define BCRYPT_HMAC_SHA1_ALG_HANDLE         ((BCRYPT_ALG_HANDLE)0x000000a1)
#define BCRYPT_HMAC_SHA256_ALG_HANDLE       ((BCRYPT_ALG_HANDLE)0x000000b1)
#define BCRYPT_HMAC_SHA384_ALG_HANDLE       ((BCRYPT_ALG_HANDLE)0x000000c1)
#define BCRYPT_HMAC_SHA512_ALG_HANDLE       ((BCRYPT_ALG_HANDLE)0x000000d1)
#define BCRYPT_RSA_ALG_HANDLE               ((BCRYPT_ALG_HANDLE)0x000000e1)
#define BCRYPT_ECDSA_ALG_HANDLE             ((BCRYPT_ALG_HANDLE)0x000000f1)
#define BCRYPT_AES_CMAC_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000101)
#define BCRYPT_AES_GMAC_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000111)
#define BCRYPT_HMAC_MD2_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000121)
#define BCRYPT_HMAC_MD4_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000131)
#define BCRYPT_3DES_CBC_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000141)
#define BCRYPT_3DES_ECB_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000151)
#define BCRYPT_3DES_CFB_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000161)
#define BCRYPT_3DES_112_CBC_ALG_HANDLE      ((BCRYPT_ALG_HANDLE)0x00000171)
#define BCRYPT_3DES_112_ECB_ALG_HANDLE      ((BCRYPT_ALG_HANDLE)0x00000181)
#define BCRYPT_3DES_112_CFB_ALG_HANDLE      ((BCRYPT_ALG_HANDLE)0x00000191)
#define BCRYPT_AES_CBC_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x000001a1)
#define BCRYPT_AES_ECB_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x000001b1)
#define BCRYPT_AES_CFB_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x000001c1)
#define BCRYPT_AES_CCM_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x000001d1)
#define BCRYPT_AES_GCM_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x000001e1)
#define BCRYPT_DES_CBC_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x000001f1)
#define BCRYPT_DES_ECB_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x00000201)
#define BCRYPT_DES_CFB_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x00000211)
#define BCRYPT_DESX_CBC_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000221)
#define BCRYPT_DESX_ECB_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000231)
#define BCRYPT_DESX_CFB_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000241)
#define BCRYPT_RC2_CBC_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x00000251)
#define BCRYPT_RC2_ECB_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x00000261)
#define BCRYPT_RC2_CFB_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x00000271)
#define BCRYPT_DH_ALG_HANDLE                ((BCRYPT_ALG_HANDLE)0x00000281)
#define BCRYPT_ECDH_ALG_HANDLE              ((BCRYPT_ALG_HANDLE)0x00000291)
#define BCRYPT_ECDH_P256_ALG_HANDLE         ((BCRYPT_ALG_HANDLE)0x000002a1)
#define BCRYPT_ECDH_P384_ALG_HANDLE         ((BCRYPT_ALG_HANDLE)0x000002b1)
#define BCRYPT_ECDH_P521_ALG_HANDLE         ((BCRYPT_ALG_HANDLE)0x000002c1)
#define BCRYPT_DSA_ALG_HANDLE               ((BCRYPT_ALG_HANDLE)0x000002d1)
#define BCRYPT_ECDSA_P256_ALG_HANDLE        ((BCRYPT_ALG_HANDLE)0x000002e1)
#define BCRYPT_ECDSA_P384_ALG_HANDLE        ((BCRYPT_ALG_HANDLE)0x000002f1)
#define BCRYPT_ECDSA_P521_ALG_HANDLE        ((BCRYPT_ALG_HANDLE)0x00000301)
#define BCRYPT_RSA_SIGN_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000311)
#define BCRYPT_CAPI_KDF_ALG_HANDLE          ((BCRYPT_ALG_HANDLE)0x00000321)
#define BCRYPT_PBKDF2_ALG_HANDLE            ((BCRYPT_ALG_HANDLE)0x00000331)
#define BCRYPT_SP800108_CTR_HMAC_ALG_HANDLE ((BCRYPT_ALG_HANDLE)0x00000341)
#define BCRYPT_SP80056A_CONCAT_ALG_HANDLE   ((BCRYPT_ALG_HANDLE)0x00000351)
#define BCRYPT_TLS1_1_KDF_ALG_HANDLE        ((BCRYPT_ALG_HANDLE)0x00000361)
#define BCRYPT_TLS1_2_KDF_ALG_HANDLE        ((BCRYPT_ALG_HANDLE)0x00000371)
#define BCRYPT_XTS_AES_ALG_HANDLE           ((BCRYPT_ALG_HANDLE)0x00000381)
#define BCRYPT_HKDF_ALG_HANDLE              ((BCRYPT_ALG_HANDLE)0x00000391)

#define KXCRYP_MAX_PREDEFINED_ALG_HANDLE_INDEX 0x33

#define KXCRYP_IS_PREDEFINED_ALG_HANDLE(AlgHandle) \
	(((ULONG_PTR) AlgHandle & 1) && !(((ULONG_PTR) AlgHandle) & ~0xFFF1))

#define KXCRYP_PREDEFINED_ALG_HANDLE_TO_INDEX(AlgHandle) ((ULONG) ((ULONG_PTR) AlgHandle >> 4))

#define SafeNCryptFreeObject(Object) \
	do { \
		if (Object) { \
			SECURITY_STATUS SafeNCryptFreeObjectSecStatus; \
			SafeNCryptFreeObjectSecStatus = NCryptFreeObject((Object)); \
			ASSERT (SUCCEEDED(SafeNCryptFreeObjectSecStatus)); \
			(Object) = 0; \
		} \
	} while (0)

#define SafeBCryptDestroyHash(HashHandle) \
	do { \
		if (HashHandle) { \
			NTSTATUS SafeBCryptDestroyHashStatus; \
			SafeBCryptDestroyHashStatus = BCryptDestroyHash((HashHandle)); \
			ASSERT (NT_SUCCESS(SafeBCryptDestroyHashStatus)); \
			(HashHandle) = NULL; \
		} \
	} while (0)

#define SafeBCryptDestroySecret(SecretHandle) \
	do { \
		if (SecretHandle) { \
			NTSTATUS SafeBCryptDestroySecretStatus; \
			SafeBCryptDestroySecretStatus = BCryptDestroySecret((SecretHandle)); \
			ASSERT (NT_SUCCESS(SafeBCryptDestroySecretStatus)); \
			(SecretHandle) = NULL; \
		} \
	} while (0)

#define SafeCertFreeCertificateContext(CertContext) \
	do { \
		CertFreeCertificateContext((CertContext)); \
		(CertContext) = NULL; \
	} while (0)

#define SafeCertFreeCertificateChain(ChainContext) \
	do { \
		CertFreeCertificateChain((ChainContext)); \
		(ChainContext) = NULL; \
	} while (0)

#define SafeCertFreeCertificateChainEngine(ChainEngine) \
	do { \
		CertFreeCertificateChainEngine((ChainEngine)); \
		(ChainEngine) = NULL; \
	} while (0)

#define SafeCertCloseStore(hCertStore, dwFlags) \
	do { \
		BOOL SafeCertCloseStoreSuccess; \
		SafeCertCloseStoreSuccess = CertCloseStore( \
			(hCertStore), \
			(dwFlags)); \
		ASSERT (SafeCertCloseStoreSuccess); \
		(hCertStore) = NULL; \
	} while (0)

KXCRYPAPI NTSTATUS WINAPI BCryptHash(
	IN	BCRYPT_ALG_HANDLE	Algorithm,
	IN	PCUCHAR				Secret OPTIONAL,
	IN	ULONG				SecretCb,
	IN	PCUCHAR				Input,
	IN	ULONG				InputCb,
	OUT	PUCHAR				Output,
	IN	ULONG				OutputCb);