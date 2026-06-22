#include "cert.h"
#include <stdio.h>
#include <stdlib.h>
#include <ncrypt.h>

#ifndef NCRYPT_VOLATILE_KEY_FLAG
#define NCRYPT_VOLATILE_KEY_FLAG 0x00000001
#endif

#ifndef PROV_RSA_AES
#define PROV_RSA_AES 24
#endif

// We define SECURITY_WIN32 before including security.h to target the correct SSPI namespace
#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>
#include <schnlsp.h>
#include <wincrypt.h>
#include <ncrypt.h>

#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")

#ifndef MS_SOFTWARE_KEY_STORAGE_PROVIDER
#define MS_SOFTWARE_KEY_STORAGE_PROVIDER L"Microsoft Software Key Storage Provider"
#endif

#ifndef NCRYPT_ECDSA_P256_ALGORITHM
#define NCRYPT_ECDSA_P256_ALGORITHM L"ECDSA_P256"
#endif

// Generates an ephemeral self-signed ECDSA certificate using the Windows CNG (Cryptography Next Generation) API.
// This certificate is registered in-memory and used by Schannel to authenticate SSL/TLS sessions.
PCCERT_CONTEXT CreateSelfSignedCertificate() {
    NCRYPT_PROV_HANDLE hProvider = 0;
    NCRYPT_KEY_HANDLE hKey = 0;
    PCCERT_CONTEXT pCertContext = NULL;
    SECURITY_STATUS status;

    // Open the default software key storage provider to host our certificate keys
    status = NCryptOpenStorageProvider(&hProvider, MS_SOFTWARE_KEY_STORAGE_PROVIDER, 0);

    if (status != ERROR_SUCCESS) {
        printf("[CNG] Provider error: 0x%08lX\n", (long)status);
        return NULL;
    }

    // Create a new key container using the ECDSA P-256 algorithm.
    // The key is marked silent and overwrites any existing key container with the same name.
    status = NCryptCreatePersistedKey(
        hProvider,
        &hKey,
        NCRYPT_ECDSA_P256_ALGORITHM,
        L"LocalSendSurfaceRT_ECC",
        0,
        NCRYPT_OVERWRITE_KEY_FLAG | NCRYPT_SILENT_FLAG
    );

    if (status != ERROR_SUCCESS) {
        printf("[CNG] ECC key error: 0x%08lX\n", (long)status);
        NCryptFreeObject(hProvider);
        return NULL;
    }

    // Generate the actual key pair within the initialized container
    status = NCryptFinalizeKey(hKey, 0);
    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return NULL;
    }

    // Set up the distinguished name (Subject DN) for the certificate
    CERT_NAME_BLOB nameBlob;
    memset(&nameBlob, 0, sizeof(nameBlob));
    char certName[] = "CN=LocalSend-Surface";

    if (CertStrToNameA(X509_ASN_ENCODING, certName, CERT_X500_NAME_STR, NULL, NULL, &nameBlob.cbData, NULL)) {
        nameBlob.pbData = (BYTE*)malloc(nameBlob.cbData);
        CertStrToNameA(X509_ASN_ENCODING, certName, CERT_X500_NAME_STR, NULL, nameBlob.pbData, &nameBlob.cbData, NULL);
    }

    // Use ECDSA with SHA-256 for the certificate signature algorithm
    CRYPT_ALGORITHM_IDENTIFIER sigAlg;
    memset(&sigAlg, 0, sizeof(sigAlg));
    sigAlg.pszObjId = szOID_ECDSA_SHA256;

    // The certificate is valid immediately and expires in 1 year
    SYSTEMTIME startTime, endTime;
    GetSystemTime(&startTime);
    GetSystemTime(&endTime);
    endTime.wYear += 1;

    pCertContext = CertCreateSelfSignCertificate(
        (HCRYPTPROV_OR_NCRYPT_KEY_HANDLE)hKey,
        &nameBlob,
        0,
        NULL,
        &sigAlg,
        &startTime,
        &endTime,
        NULL
    );

    if (!pCertContext) {
        printf("[CryptoAPI] CertCreateSelfSignCertificate failed: 0x%08lX\n", (long)GetLastError());
    }

    // Release allocated CNG handles and DN name buffers
    if (hKey) NCryptFreeObject(hKey);
    if (hProvider) NCryptFreeObject(hProvider);
    if (nameBlob.pbData) free(nameBlob.pbData);

    return pCertContext;
}

