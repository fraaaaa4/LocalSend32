#include "openssl_dyn.h"

BOOL g_OpenSSLLoaded = FALSE;
static HMODULE g_hCrypto = NULL;
static HMODULE g_hSsl = NULL;

fn_OPENSSL_init_ssl dyn_OPENSSL_init_ssl = NULL;
fn_TLS_client_method dyn_TLS_client_method = NULL;
fn_TLS_server_method dyn_TLS_server_method = NULL;
fn_SSL_CTX_new dyn_SSL_CTX_new = NULL;
fn_SSL_CTX_set_verify dyn_SSL_CTX_set_verify = NULL;
fn_SSL_CTX_use_certificate dyn_SSL_CTX_use_certificate = NULL;
fn_SSL_CTX_use_PrivateKey dyn_SSL_CTX_use_PrivateKey = NULL;
fn_SSL_CTX_free dyn_SSL_CTX_free = NULL;
fn_SSL_new dyn_SSL_new = NULL;
fn_SSL_set_fd dyn_SSL_set_fd = NULL;
fn_SSL_connect dyn_SSL_connect = NULL;
fn_SSL_accept dyn_SSL_accept = NULL;
fn_SSL_free dyn_SSL_free = NULL;
fn_SSL_read dyn_SSL_read = NULL;
fn_SSL_write dyn_SSL_write = NULL;
fn_SSL_shutdown dyn_SSL_shutdown = NULL;
fn_SSL_get_error dyn_SSL_get_error = NULL;

fn_EVP_PKEY_CTX_new_id dyn_EVP_PKEY_CTX_new_id = NULL;
fn_EVP_PKEY_keygen_init dyn_EVP_PKEY_keygen_init = NULL;
fn_EVP_PKEY_CTX_set_rsa_keygen_bits dyn_EVP_PKEY_CTX_set_rsa_keygen_bits = NULL;
fn_EVP_PKEY_keygen dyn_EVP_PKEY_keygen = NULL;
fn_EVP_PKEY_CTX_free dyn_EVP_PKEY_CTX_free = NULL;
fn_EVP_PKEY_free dyn_EVP_PKEY_free = NULL;

fn_X509_new dyn_X509_new = NULL;
fn_X509_set_version dyn_X509_set_version = NULL;
fn_X509_get_serialNumber dyn_X509_get_serialNumber = NULL;
fn_ASN1_INTEGER_set dyn_ASN1_INTEGER_set = NULL;
fn_X509_getm_notBefore dyn_X509_getm_notBefore = NULL;
fn_X509_getm_notAfter dyn_X509_getm_notAfter = NULL;
fn_X509_gmtime_adj dyn_X509_gmtime_adj = NULL;
fn_X509_set_pubkey dyn_X509_set_pubkey = NULL;
fn_X509_get_subject_name dyn_X509_get_subject_name = NULL;
fn_X509_NAME_add_entry_by_txt dyn_X509_NAME_add_entry_by_txt = NULL;
fn_X509_set_issuer_name dyn_X509_set_issuer_name = NULL;
fn_EVP_sha256 dyn_EVP_sha256 = NULL;
fn_X509_sign dyn_X509_sign = NULL;
fn_X509_free dyn_X509_free = NULL;
fn_X509_digest dyn_X509_digest = NULL;

typedef struct {
    const char* cryptoName;
    const char* sslName;
} OpenSSLDllPair;

static const OpenSSLDllPair g_DllPairs[] = {
    // Version 3.x
    { "libcrypto-3.dll", "libssl-3.dll" },
    { "libcrypto-3-x64.dll", "libssl-3-x64.dll" },
    { "libcrypto-3-arm64.dll", "libssl-3-arm64.dll" },
    { "libcrypto-3-arm.dll", "libssl-3-arm.dll" },
    // Version 4.x
    { "libcrypto-4.dll", "libssl-4.dll" },
    { "libcrypto-4-x64.dll", "libssl-4-x64.dll" },
    { "libcrypto-4-arm64.dll", "libssl-4-arm64.dll" },
    { "libcrypto-4-arm.dll", "libssl-4-arm.dll" }
};

BOOL LoadOpenSSLDynamically(void) {
    if (g_OpenSSLLoaded) return TRUE;

    // Load matching pairs of crypto and ssl DLLs to avoid version/ABI mismatches
    int numPairs = sizeof(g_DllPairs) / sizeof(g_DllPairs[0]);
    for (int i = 0; i < numPairs; i++) {
        g_hCrypto = LoadLibraryA(g_DllPairs[i].cryptoName);
        if (g_hCrypto) {
            g_hSsl = LoadLibraryA(g_DllPairs[i].sslName);
            if (g_hSsl) {
                printf("[OpenSSL] Loaded %s and %s successfully!\n", g_DllPairs[i].cryptoName, g_DllPairs[i].sslName);
                fflush(stdout);
                break;
            } else {
                DWORD sslErr = GetLastError();
                printf("[OpenSSL] Loaded %s, but %s failed (error %lu)\n", g_DllPairs[i].cryptoName, g_DllPairs[i].sslName, sslErr);
                fflush(stdout);
                FreeLibrary(g_hCrypto);
                g_hCrypto = NULL;
            }
        } else {
            DWORD cryptoErr = GetLastError();
            if (cryptoErr != ERROR_MOD_NOT_FOUND && cryptoErr != ERROR_FILE_NOT_FOUND) {
                printf("[OpenSSL] %s failed to load (error %lu)\n", g_DllPairs[i].cryptoName, cryptoErr);
                fflush(stdout);
            }
        }
    }

    if (!g_hCrypto || !g_hSsl) {
        FreeOpenSSLDynamically();
        return FALSE;
    }

    // Resolve crypto functions
    dyn_EVP_PKEY_CTX_new_id = (fn_EVP_PKEY_CTX_new_id)GetProcAddress(g_hCrypto, "EVP_PKEY_CTX_new_id");
    dyn_EVP_PKEY_keygen_init = (fn_EVP_PKEY_keygen_init)GetProcAddress(g_hCrypto, "EVP_PKEY_keygen_init");
    dyn_EVP_PKEY_CTX_set_rsa_keygen_bits = (fn_EVP_PKEY_CTX_set_rsa_keygen_bits)GetProcAddress(g_hCrypto, "EVP_PKEY_CTX_set_rsa_keygen_bits");
    dyn_EVP_PKEY_keygen = (fn_EVP_PKEY_keygen)GetProcAddress(g_hCrypto, "EVP_PKEY_keygen");
    dyn_EVP_PKEY_CTX_free = (fn_EVP_PKEY_CTX_free)GetProcAddress(g_hCrypto, "EVP_PKEY_CTX_free");
    dyn_EVP_PKEY_free = (fn_EVP_PKEY_free)GetProcAddress(g_hCrypto, "EVP_PKEY_free");

    dyn_X509_new = (fn_X509_new)GetProcAddress(g_hCrypto, "X509_new");
    dyn_X509_set_version = (fn_X509_set_version)GetProcAddress(g_hCrypto, "X509_set_version");
    dyn_X509_get_serialNumber = (fn_X509_get_serialNumber)GetProcAddress(g_hCrypto, "X509_get_serialNumber");
    dyn_ASN1_INTEGER_set = (fn_ASN1_INTEGER_set)GetProcAddress(g_hCrypto, "ASN1_INTEGER_set");
    dyn_X509_getm_notBefore = (fn_X509_getm_notBefore)GetProcAddress(g_hCrypto, "X509_getm_notBefore");
    dyn_X509_getm_notAfter = (fn_X509_getm_notAfter)GetProcAddress(g_hCrypto, "X509_getm_notAfter");
    dyn_X509_gmtime_adj = (fn_X509_gmtime_adj)GetProcAddress(g_hCrypto, "X509_gmtime_adj");
    dyn_X509_set_pubkey = (fn_X509_set_pubkey)GetProcAddress(g_hCrypto, "X509_set_pubkey");
    dyn_X509_get_subject_name = (fn_X509_get_subject_name)GetProcAddress(g_hCrypto, "X509_get_subject_name");
    dyn_X509_NAME_add_entry_by_txt = (fn_X509_NAME_add_entry_by_txt)GetProcAddress(g_hCrypto, "X509_NAME_add_entry_by_txt");
    dyn_X509_set_issuer_name = (fn_X509_set_issuer_name)GetProcAddress(g_hCrypto, "X509_set_issuer_name");
    dyn_EVP_sha256 = (fn_EVP_sha256)GetProcAddress(g_hCrypto, "EVP_sha256");
    dyn_X509_sign = (fn_X509_sign)GetProcAddress(g_hCrypto, "X509_sign");
    dyn_X509_free = (fn_X509_free)GetProcAddress(g_hCrypto, "X509_free");
    dyn_X509_digest = (fn_X509_digest)GetProcAddress(g_hCrypto, "X509_digest");

    // Resolve SSL functions
    dyn_OPENSSL_init_ssl = (fn_OPENSSL_init_ssl)GetProcAddress(g_hSsl, "OPENSSL_init_ssl");
    dyn_TLS_client_method = (fn_TLS_client_method)GetProcAddress(g_hSsl, "TLS_client_method");
    dyn_TLS_server_method = (fn_TLS_server_method)GetProcAddress(g_hSsl, "TLS_server_method");
    dyn_SSL_CTX_new = (fn_SSL_CTX_new)GetProcAddress(g_hSsl, "SSL_CTX_new");
    dyn_SSL_CTX_set_verify = (fn_SSL_CTX_set_verify)GetProcAddress(g_hSsl, "SSL_CTX_set_verify");
    dyn_SSL_CTX_use_certificate = (fn_SSL_CTX_use_certificate)GetProcAddress(g_hSsl, "SSL_CTX_use_certificate");
    dyn_SSL_CTX_use_PrivateKey = (fn_SSL_CTX_use_PrivateKey)GetProcAddress(g_hSsl, "SSL_CTX_use_PrivateKey");
    dyn_SSL_CTX_free = (fn_SSL_CTX_free)GetProcAddress(g_hSsl, "SSL_CTX_free");
    dyn_SSL_new = (fn_SSL_new)GetProcAddress(g_hSsl, "SSL_new");
    dyn_SSL_set_fd = (fn_SSL_set_fd)GetProcAddress(g_hSsl, "SSL_set_fd");
    dyn_SSL_connect = (fn_SSL_connect)GetProcAddress(g_hSsl, "SSL_connect");
    dyn_SSL_accept = (fn_SSL_accept)GetProcAddress(g_hSsl, "SSL_accept");
    dyn_SSL_free = (fn_SSL_free)GetProcAddress(g_hSsl, "SSL_free");
    dyn_SSL_read = (fn_SSL_read)GetProcAddress(g_hSsl, "SSL_read");
    dyn_SSL_write = (fn_SSL_write)GetProcAddress(g_hSsl, "SSL_write");
    dyn_SSL_shutdown = (fn_SSL_shutdown)GetProcAddress(g_hSsl, "SSL_shutdown");
    dyn_SSL_get_error = (fn_SSL_get_error)GetProcAddress(g_hSsl, "SSL_get_error");

    // Verify all pointers
    if (!dyn_EVP_PKEY_CTX_new_id || !dyn_EVP_PKEY_keygen_init || !dyn_EVP_PKEY_CTX_set_rsa_keygen_bits ||
        !dyn_EVP_PKEY_keygen || !dyn_EVP_PKEY_CTX_free || !dyn_EVP_PKEY_free || !dyn_X509_new ||
        !dyn_X509_set_version || !dyn_X509_get_serialNumber || !dyn_ASN1_INTEGER_set ||
        !dyn_X509_getm_notBefore || !dyn_X509_getm_notAfter || !dyn_X509_gmtime_adj ||
        !dyn_X509_set_pubkey || !dyn_X509_get_subject_name || !dyn_X509_NAME_add_entry_by_txt ||
        !dyn_X509_set_issuer_name || !dyn_EVP_sha256 || !dyn_X509_sign || !dyn_X509_free ||
        !dyn_X509_digest || !dyn_OPENSSL_init_ssl || !dyn_TLS_client_method || !dyn_TLS_server_method ||
        !dyn_SSL_CTX_new || !dyn_SSL_CTX_set_verify || !dyn_SSL_CTX_use_certificate ||
        !dyn_SSL_CTX_use_PrivateKey || !dyn_SSL_CTX_free || !dyn_SSL_new || !dyn_SSL_set_fd ||
        !dyn_SSL_connect || !dyn_SSL_accept || !dyn_SSL_free || !dyn_SSL_read ||
        !dyn_SSL_write || !dyn_SSL_shutdown || !dyn_SSL_get_error) {
        
        printf("--- OpenSSL Dynamic Load Failure Details ---\n");
        if (!dyn_EVP_PKEY_CTX_new_id) printf("NULL: EVP_PKEY_CTX_new_id\n");
        if (!dyn_EVP_PKEY_keygen_init) printf("NULL: EVP_PKEY_keygen_init\n");
        if (!dyn_EVP_PKEY_CTX_set_rsa_keygen_bits) printf("NULL: EVP_PKEY_CTX_set_rsa_keygen_bits\n");
        if (!dyn_EVP_PKEY_keygen) printf("NULL: EVP_PKEY_keygen\n");
        if (!dyn_EVP_PKEY_CTX_free) printf("NULL: EVP_PKEY_CTX_free\n");
        if (!dyn_EVP_PKEY_free) printf("NULL: EVP_PKEY_free\n");
        if (!dyn_X509_new) printf("NULL: X509_new\n");
        if (!dyn_X509_set_version) printf("NULL: X509_set_version\n");
        if (!dyn_X509_get_serialNumber) printf("NULL: X509_get_serialNumber\n");
        if (!dyn_ASN1_INTEGER_set) printf("NULL: ASN1_INTEGER_set\n");
        if (!dyn_X509_getm_notBefore) printf("NULL: X509_getm_notBefore\n");
        if (!dyn_X509_getm_notAfter) printf("NULL: X509_getm_notAfter\n");
        if (!dyn_X509_gmtime_adj) printf("NULL: X509_gmtime_adj\n");
        if (!dyn_X509_set_pubkey) printf("NULL: X509_set_pubkey\n");
        if (!dyn_X509_get_subject_name) printf("NULL: X509_get_subject_name\n");
        if (!dyn_X509_NAME_add_entry_by_txt) printf("NULL: X509_NAME_add_entry_by_txt\n");
        if (!dyn_X509_set_issuer_name) printf("NULL: X509_set_issuer_name\n");
        if (!dyn_EVP_sha256) printf("NULL: EVP_sha256\n");
        if (!dyn_X509_sign) printf("NULL: X509_sign\n");
        if (!dyn_X509_free) printf("NULL: X509_free\n");
        if (!dyn_X509_digest) printf("NULL: X509_digest\n");
        if (!dyn_OPENSSL_init_ssl) printf("NULL: OPENSSL_init_ssl\n");
        if (!dyn_TLS_client_method) printf("NULL: TLS_client_method\n");
        if (!dyn_TLS_server_method) printf("NULL: TLS_server_method\n");
        if (!dyn_SSL_CTX_new) printf("NULL: SSL_CTX_new\n");
        if (!dyn_SSL_CTX_set_verify) printf("NULL: SSL_CTX_set_verify\n");
        if (!dyn_SSL_CTX_use_certificate) printf("NULL: SSL_CTX_use_certificate\n");
        if (!dyn_SSL_CTX_use_PrivateKey) printf("NULL: SSL_CTX_use_PrivateKey\n");
        if (!dyn_SSL_CTX_free) printf("NULL: SSL_CTX_free\n");
        if (!dyn_SSL_new) printf("NULL: SSL_new\n");
        if (!dyn_SSL_set_fd) printf("NULL: SSL_set_fd\n");
        if (!dyn_SSL_connect) printf("NULL: SSL_connect\n");
        if (!dyn_SSL_accept) printf("NULL: SSL_accept\n");
        if (!dyn_SSL_free) printf("NULL: SSL_free\n");
        if (!dyn_SSL_read) printf("NULL: SSL_read\n");
        if (!dyn_SSL_write) printf("NULL: SSL_write\n");
        if (!dyn_SSL_shutdown) printf("NULL: SSL_shutdown\n");
        if (!dyn_SSL_get_error) printf("NULL: SSL_get_error\n");

        FreeOpenSSLDynamically();
        return FALSE;
    }

    g_OpenSSLLoaded = TRUE;
    return TRUE;
}

void FreeOpenSSLDynamically(void) {
    if (g_hSsl) {
        FreeLibrary(g_hSsl);
        g_hSsl = NULL;
    }
    if (g_hCrypto) {
        FreeLibrary(g_hCrypto);
        g_hCrypto = NULL;
    }
    g_OpenSSLLoaded = FALSE;
}
