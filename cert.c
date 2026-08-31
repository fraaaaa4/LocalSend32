#include "cert.h"
#include <stdio.h>
#include <stdlib.h>
#include <wincrypt.h>

#pragma comment(lib, "crypt32.lib")

// Generates a temporary self-signed certificate with Client and Server Authentication EKUs
PCCERT_CONTEXT CreateSelfSignedCertificate() {
    PCCERT_CONTEXT pCertContext = NULL;

    // Prepare certificate subject name
    CERT_NAME_BLOB nameBlob;
    memset(&nameBlob, 0, sizeof(nameBlob));
    char certName[] = "CN=LocalSend-Surface";

    if (CertStrToNameA(X509_ASN_ENCODING, certName, CERT_X500_NAME_STR, NULL, NULL, &nameBlob.cbData, NULL)) {
        nameBlob.pbData = (BYTE*)malloc(nameBlob.cbData);
        if (nameBlob.pbData) {
            CertStrToNameA(X509_ASN_ENCODING, certName, CERT_X500_NAME_STR, NULL, nameBlob.pbData, &nameBlob.cbData, NULL);
        }
    }

    // Set up Extended Key Usage for both Server and Client authentication
    LPSTR ekus[] = {
        (LPSTR)szOID_PKIX_KP_SERVER_AUTH,
        (LPSTR)szOID_PKIX_KP_CLIENT_AUTH
    };
    CERT_ENHKEY_USAGE enhKeyUsage;
    enhKeyUsage.cUsageIdentifier = 2;
    enhKeyUsage.rgpszUsageIdentifier = ekus;

    CERT_EXTENSION ext;
    memset(&ext, 0, sizeof(ext));
    ext.pszObjId = (LPSTR)szOID_ENHANCED_KEY_USAGE;
    ext.fCritical = FALSE;
    ext.Value.cbData = 0;
    ext.Value.pbData = NULL;

    // Encode EKU structure to ASN.1
    if (CryptEncodeObject(X509_ASN_ENCODING, szOID_ENHANCED_KEY_USAGE, &enhKeyUsage, NULL, &ext.Value.cbData)) {
        ext.Value.pbData = (BYTE*)malloc(ext.Value.cbData);
        if (ext.Value.pbData) {
            CryptEncodeObject(X509_ASN_ENCODING, szOID_ENHANCED_KEY_USAGE, &enhKeyUsage, ext.Value.pbData, &ext.Value.cbData);
        }
    }

    CERT_EXTENSIONS exts;
    exts.cExtension = 1;
    exts.rgExtension = &ext;

    // Try machine keyset first so LSASS can access private keys during SSL handshake when running as Admin.
    pCertContext = CertCreateSelfSignCertificate(
        0,
        &nameBlob,
        CRYPT_MACHINE_KEYSET,
        NULL,
        NULL,
        NULL,
        NULL,
        &exts
    );
    if (!pCertContext) {
        // Fall back to user keyset if we lack admin privileges for machine keyset.
        pCertContext = CertCreateSelfSignCertificate(
            0,
            &nameBlob,
            0,
            NULL,
            NULL,
            NULL,
            NULL,
            &exts
        );
    }

    if (!pCertContext) {
        printf("[Crypto] Automatic CertCreateSelfSignCertificate failed: 0x%08lX\n", (long)GetLastError());
    } else {
        printf("[Crypto] Successfully generated self-signed certificate with Client/Server EKU.\n");
    }

    if (ext.Value.pbData) free(ext.Value.pbData);
    if (nameBlob.pbData) free(nameBlob.pbData);

    return pCertContext;
}
