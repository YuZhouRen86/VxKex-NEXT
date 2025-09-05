#include "buildcfg.h"
#include "kxcrypp.h"

// Function to ensure TLS 1.3 cipher suites are properly configured
STATIC VOID EnsureTls13CipherSuites(VOID)
{
	HKEY CipherSuitesKey = NULL;
	DWORD Value = 1;
	LONG Result;
	
	// Open or create the cipher suites key
	Result = RegCreateKeyExW(
		HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control\\Cryptography\\Configuration\\Local\\SSL\\00010002",
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&CipherSuitesKey,
		NULL);

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to open cipher suites registry key: %d", Result);
		goto Cleanup;
	}

	// Add TLS 1.3 cipher suites
	// TLS_AES_256_GCM_SHA384
	Result = RegSetValueExW(
		CipherSuitesKey,
		L"TLS_AES_256_GCM_SHA384",
		0,
		REG_DWORD,
		(BYTE*)&Value,
		sizeof(DWORD));

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to set TLS_AES_256_GCM_SHA384 cipher suite: %d", Result);
	}

	// TLS_AES_128_GCM_SHA256
	Result = RegSetValueExW(
		CipherSuitesKey,
		L"TLS_AES_128_GCM_SHA256",
		0,
		REG_DWORD,
		(BYTE*)&Value,
		sizeof(DWORD));

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to set TLS_AES_128_GCM_SHA256 cipher suite: %d", Result);
	}

	// TLS_CHACHA20_POLY1305_SHA256
	Result = RegSetValueExW(
		CipherSuitesKey,
		L"TLS_CHACHA20_POLY1305_SHA256",
		0,
		REG_DWORD,
		(BYTE*)&Value,
		sizeof(DWORD));

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to set TLS_CHACHA20_POLY1305_SHA256 cipher suite: %d", Result);
	}

	KexLogDebugEvent(L"TLS 1.3 cipher suites configured successfully");

Cleanup:
	if (CipherSuitesKey) RegCloseKey(CipherSuitesKey);
}

// Function to ensure TLS 1.3 registry settings are properly configured
STATIC VOID EnsureTls13RegistrySettings(VOID)
{
	HKEY SchannelKey = NULL;
	HKEY ProtocolsKey = NULL;
	HKEY Tls13Key = NULL;
	HKEY ClientKey = NULL;
	HKEY ServerKey = NULL;
	DWORD Value = 1;
	DWORD DisabledValue = 0;
	LONG Result;
	
	// Configure TLS 1.3 cipher suites
	EnsureTls13CipherSuites();

	// Open or create the SCHANNEL key
	Result = RegCreateKeyExW(
		HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL",
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&SchannelKey,
		NULL);

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to open SCHANNEL registry key: %d", Result);
		goto Cleanup;
	}

	// Open or create the Protocols key
	Result = RegCreateKeyExW(
		SchannelKey,
		L"Protocols",
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&ProtocolsKey,
		NULL);

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to open Protocols registry key: %d", Result);
		goto Cleanup;
	}

	// Open or create the TLS 1.3 key
	Result = RegCreateKeyExW(
		ProtocolsKey,
		L"TLS 1.3",
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&Tls13Key,
		NULL);

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to create TLS 1.3 registry key: %d", Result);
		goto Cleanup;
	}

	// Create Client key
	Result = RegCreateKeyExW(
		Tls13Key,
		L"Client",
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&ClientKey,
		NULL);

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to create TLS 1.3 Client registry key: %d", Result);
		goto Cleanup;
	}

	// Set Client Enabled value
	Result = RegSetValueExW(
		ClientKey,
		L"Enabled",
		0,
		REG_DWORD,
		(BYTE*)&Value,
		sizeof(DWORD));

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to set TLS 1.3 Client Enabled value: %d", Result);
	}

	// Set Client DisabledByDefault value
	Result = RegSetValueExW(
		ClientKey,
		L"DisabledByDefault",
		0,
		REG_DWORD,
		(BYTE*)&DisabledValue,
		sizeof(DWORD));

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to set TLS 1.3 Client DisabledByDefault value: %d", Result);
	}

	// Create Server key
	Result = RegCreateKeyExW(
		Tls13Key,
		L"Server",
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&ServerKey,
		NULL);

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to create TLS 1.3 Server registry key: %d", Result);
		goto Cleanup;
	}

	// Set Server Enabled value
	Result = RegSetValueExW(
		ServerKey,
		L"Enabled",
		0,
		REG_DWORD,
		(BYTE*)&Value,
		sizeof(DWORD));

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to set TLS 1.3 Server Enabled value: %d", Result);
	}

	// Set Server DisabledByDefault value
	Result = RegSetValueExW(
		ServerKey,
		L"DisabledByDefault",
		0,
		REG_DWORD,
		(BYTE*)&DisabledValue,
		sizeof(DWORD));

	if (Result != ERROR_SUCCESS) {
		KexLogWarningEvent(L"Failed to set TLS 1.3 Server DisabledByDefault value: %d", Result);
	}

	KexLogDebugEvent(L"TLS 1.3 registry settings configured successfully");

Cleanup:
	if (ServerKey) RegCloseKey(ServerKey);
	if (ClientKey) RegCloseKey(ClientKey);
	if (Tls13Key) RegCloseKey(Tls13Key);
	if (ProtocolsKey) RegCloseKey(ProtocolsKey);
	if (SchannelKey) RegCloseKey(SchannelKey);
}

//
// TODO: Implement the ANSI version of this function.
//

KXCRYPAPI SECURITY_STATUS SEC_ENTRY Ext_AcquireCredentialsHandleW(
	IN	PWSTR			Principal OPTIONAL,
	IN	PWSTR			Package,
	IN	ULONG			CredentialUseFlags,					// SECPKG_CRED_*
	IN	PLUID			LogonId OPTIONAL,
	IN	PVOID			AuthData OPTIONAL,
	IN	SEC_GET_KEY_FN	GetKeyFunction OPTIONAL,
	IN	PVOID			GetKeyFunctionArgument OPTIONAL,
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		ExpiryTime OPTIONAL)
{
	SECURITY_STATUS SecurityStatus;
	PSCH_CREDENTIALS Credentials;
	SCHANNEL_CRED ConvertedCredentials;

	if (StringEqual(Package, UNISP_NAME) && AuthData != NULL) {
		Credentials = (PSCH_CREDENTIALS) AuthData;

		//
		// The target application is using the SChannel SSP.
		//

		if (Credentials->Version == SCH_CRED_V5) {
			//
			// Win10+ version of the structure. We need to convert the
			// SCH_CREDENTIALS structure into a SCHANNEL_CRED structure.
			//

			RtlZeroMemory(&ConvertedCredentials, sizeof(ConvertedCredentials));
			ConvertedCredentials.dwVersion			= SCH_CRED_V4;
			ConvertedCredentials.cCreds				= Credentials->NumberOfCertificateContexts;
			ConvertedCredentials.paCred				= Credentials->CertificateContexts;
			ConvertedCredentials.hRootStore			= Credentials->RootStore;
			ConvertedCredentials.cMappers			= Credentials->NumberOfMappers;
			ConvertedCredentials.aphMappers			= Credentials->Mappers;
			ConvertedCredentials.dwSessionLifespan	= Credentials->SessionLifespan;
			ConvertedCredentials.dwFlags			= Credentials->Flags & SCH_WIN7_VALID_FLAGS;

			ASSERT ((Credentials->Flags & ~SCH_WIN7_VALID_FLAGS) == 0);
			ASSERT (Credentials->NumberOfTlsParameters <= 1);

			if (Credentials->NumberOfTlsParameters >= 1 &&
				Credentials->TlsParameters != NULL) {

				PTLS_PARAMETERS TlsParameters;
				
				TlsParameters = Credentials->TlsParameters;

				ASSERT (TlsParameters->NumberOfAlpnIds == 0);
				ASSERT (TlsParameters->AlpnIds == NULL);

				// Calculate enabled protocols from disabled protocols
				ULONG EnabledProtocols = ~(TlsParameters->DisabledProtocols);
				
				// Ensure TLS 1.3 is properly handled for Windows 7
				// Windows 7 doesn't natively support TLS 1.3, so we need to handle it specially
				if (EnabledProtocols & SP_PROT_TLS1_3) {
					// Log that TLS 1.3 is being requested
					KexLogDebugEvent(L"TLS 1.3 protocol requested by application");
					
					// Configure registry settings for TLS 1.3
					EnsureTls13RegistrySettings();
					
					// Make sure TLS 1.2 is also enabled as a fallback
					EnabledProtocols |= SP_PROT_TLS1_2_CLIENT | SP_PROT_TLS1_2_SERVER;
				}
				
				ConvertedCredentials.grbitEnabledProtocols = EnabledProtocols;
			}

			//
			// Make AcquireCredentialsHandleW use the structure we made.
			//

			AuthData = &ConvertedCredentials;
			KexLogDebugEvent(L"Converted v5 SChannel credentials structure to v4.");
		} else if (Credentials->Version > SCH_CRED_V5) {
			//
			// This could be a bug in the target application, or it could be
			// some future version of the structure they will make in newer
			// versions of Windows. Who knows what it will be named. Probably
			// something ridiculous like SCHANL_CREDS.
			//

			KexLogWarningEvent(
				L"SChannel credentials structure version is too high.\r\n\r\n"
				L"The application is requesting version %lu.",
				Credentials->Version);

			KexDebugCheckpoint();
		}
	}

	SecurityStatus = AcquireCredentialsHandle(
		Principal,
		Package,
		CredentialUseFlags,
		LogonId,
		AuthData,
		GetKeyFunction,
		GetKeyFunctionArgument,
		CredentialHandle,
		ExpiryTime);

	if (SecurityStatus != SEC_E_OK) {
		KexDebugCheckpoint();
	}

	return SecurityStatus;
}