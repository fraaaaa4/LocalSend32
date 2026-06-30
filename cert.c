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

// Generates a temporary self-signed certificate using Windows CNG
PCCERT_CONTEXT CreateSelfSignedCertificate() {
    NCRYPT_PROV_HANDLE hProvider = 0;
    NCRYPT_KEY_HANDLE hKey = 0;
    PCCERT_CONTEXT pCertContext = NULL;
    SECURITY_STATUS status;

    status = NCryptOpenStorageProvider(&hProvider, MS_SOFTWARE_KEY_STORAGE_PROVIDER, 0);
    if (status != ERROR_SUCCESS) {
        printf("[CNG] Provider error: 0x%08lX\n", (long)status);
        return NULL;
    }

    // Generate ECC key pair
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

    status = NCryptFinalizeKey(hKey, 0);
    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return NULL;
    }

    // Prepare certificate subject name
    CERT_NAME_BLOB nameBlob;
    memset(&nameBlob, 0, sizeof(nameBlob));
    char certName[] = "CN=LocalSend-Surface";

    if (CertStrToNameA(X509_ASN_ENCODING, certName, CERT_X500_NAME_STR, NULL, NULL, &nameBlob.cbData, NULL)) {
        nameBlob.pbData = (BYTE*)malloc(nameBlob.cbData);
        CertStrToNameA(X509_ASN_ENCODING, certName, CERT_X500_NAME_STR, NULL, nameBlob.pbData, &nameBlob.cbData, NULL);
    }

    CRYPT_ALGORITHM_IDENTIFIER sigAlg;
    memset(&sigAlg, 0, sizeof(sigAlg));
    sigAlg.pszObjId = szOID_ECDSA_SHA256;

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

    // Clean up CNG handles and memory
    if (hKey) NCryptFreeObject(hKey);
    if (hProvider) NCryptFreeObject(hProvider);
    if (nameBlob.pbData) free(nameBlob.pbData);

    return pCertContext;
}
