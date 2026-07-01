#include "buildcfg.h"
#include <KexComm.h>
#include <KexDll.h>
#include <KxSChanl.h>

#define SECURITY_WIN32
#include <Security.h>
#include <bcrypt.h>
#include <KxCryp.h>

EXTERN PKEX_PROCESS_DATA KexData;

#define SafeBCryptCloseAlgorithmProvider(Provider) \
	do { \
		if (Provider) { \
			NTSTATUS SafeBCryptCloseAlgorithmProviderStatus; \
			SafeBCryptCloseAlgorithmProviderStatus = Ext_BCryptCloseAlgorithmProvider((Provider), 0); \
			ASSERT (NT_SUCCESS(SafeBCryptCloseAlgorithmProviderStatus)); \
			(Provider) = NULL; \
		} \
	} while (0)

#define SafeBCryptDestroyKey(KeyHandle) \
	do { \
		if (KeyHandle) { \
			NTSTATUS SafeBCryptDestroyKeyStatus; \
			SafeBCryptDestroyKeyStatus = Ext_BCryptDestroyKey((KeyHandle)); \
			ASSERT (NT_SUCCESS(SafeBCryptDestroyKeyStatus)); \
			(KeyHandle) = NULL; \
		} \
	} while(0)

typedef struct DECLSPEC_ALIGN(4) {
	ULONG				cbLength;		// 0x14
	ULONG				dwMagic;		// always 'MSEC'
	ULONG				cbBuffer;
	ULONG				cbSecretAgreement;
	BYTE				rgbBuffer[ANYSIZE_ARRAY];
} TYPEDEF_TYPE_NAME(MSCRYPT_SECRET);

// Cast from BCRYPT_SECRET_HANDLE to obtain pointer to this.
typedef struct {
	ULONG				cbLength;
	ULONG				dwMagic;		// always 'UUUT'
	BCRYPT_ALG_HANDLE	hAlgorithm;
	PMSCRYPT_SECRET		pSecret;
} TYPEDEF_TYPE_NAME(BCRYPT_SECRET_HEADER);

#define KDF_LABEL               0xD
#define KDF_CONTEXT             0xE
#define KDF_SALT                0xF
#define KDF_ITERATION_COUNT     0x10

#define MAGIC_ALG (('A' << 24) | ('L' << 16) | ('G' << 8) | '0')
#define MAGIC_KEY (('K' << 24) | ('E' << 16) | ('Y' << 8) | '0')

typedef enum _ALG_ID {
	ALG_ID_3DES,
	ALG_ID_AES,
	ALG_ID_RC4,
	ALG_ID_SHA256,
	ALG_ID_SHA384,
	ALG_ID_SHA512,
	ALG_ID_SHA1,
	ALG_ID_MD5,
	ALG_ID_MD4,
	ALG_ID_MD2,
	ALG_ID_RSA,
	ALG_ID_DH,
	ALG_ID_ECDH_P256,
	ALG_ID_ECDH_P384,
	ALG_ID_RSA_SIGN,
	ALG_ID_ECDSA_P256,
	ALG_ID_ECDSA_P384,
	ALG_ID_DSA,
	ALG_ID_RNG,
	ALG_ID_SP800108_CTR_HMAC,
	ALG_ID_SP80056A_CONCAT,
	ALG_ID_PBKDF2,
	ALG_ID_CAPI_KDF
} TYPEDEF_TYPE_NAME(ALG_ID);

typedef enum _CHAIN_MODE {
	CHAIN_MODE_CBC,
	CHAIN_MODE_ECB,
	CHAIN_MODE_CFB,
	CHAIN_MODE_CCM,
	CHAIN_MODE_GCM
} TYPEDEF_TYPE_NAME(CHAIN_MODE);

typedef struct _KEY_SYMMETRIC {
	CHAIN_MODE			Mode;
	//ULONG				BlockSize;
	UCHAR*				Vector;
	ULONG				VectorLength;
	UCHAR*				Secret;
	ULONG				SecretLength;
	CRITICAL_SECTION	CriticalSection;
} TYPEDEF_TYPE_NAME(KEY_SYMMETRIC);

typedef struct _KEY_ASYMMETRIC {
	ULONG		BitLength;
	ULONG		Flags;
	DSSSEED		DssSeed;
} TYPEDEF_TYPE_NAME(KEY_ASYMMETRIC);

typedef struct _KEY_OBJECT {
	ULONG				Magic;
	ALG_ID				AlgId;
	UINT64 PrivateData[3];
	union {
		KEY_SYMMETRIC	Symmetric;
		KEY_ASYMMETRIC	Asymmetric;
	} Data;
} TYPEDEF_TYPE_NAME(KEY_OBJECT);

typedef struct _ALGORITHM {
	ULONG		Magic;
	ALG_ID		Id;
	CHAIN_MODE	Mode;
	ULONG		Flags;
} TYPEDEF_TYPE_NAME(ALGORITHM);

//
// bcrypt.c
//

VOID CleanupCachedPredefinedHandles(
	VOID);

//
// credhndl.c
//

KXCRYPAPI SECURITY_STATUS SEC_ENTRY Ext_AcquireCredentialsHandleA(
	IN	PSTR			Principal OPTIONAL,
	IN	PSTR			Package,
	IN	ULONG			CredentialUseFlags,
	IN	PVOID			LogonId OPTIONAL,
	IN	PVOID			AuthData OPTIONAL,
	IN	SEC_GET_KEY_FN	GetKeyFn OPTIONAL,
	IN	PVOID			GetKeyArgument OPTIONAL,
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		Expiry OPTIONAL);

KXCRYPAPI SECURITY_STATUS SEC_ENTRY Ext_AcquireCredentialsHandleW(
	IN	PWSTR			Principal OPTIONAL,
	IN	PWSTR			Package,
	IN	ULONG			CredentialUseFlags,
	IN	PVOID			LogonId OPTIONAL,
	IN	PVOID			AuthData OPTIONAL,
	IN	SEC_GET_KEY_FN	GetKeyFn OPTIONAL,
	IN	PVOID			GetKeyArgument OPTIONAL,
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		Expiry OPTIONAL);