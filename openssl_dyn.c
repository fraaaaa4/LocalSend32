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

fn_EVP_PKEY_CTX_new_id dyn_EVP_PKEY_CTX_new_id = NULL;
fn_EVP_PKEY_keygen_init dyn_EVP_PKEY_keygen_init = NULL;
fn_EVP_PKEY_CTX_set_rsa_keygen_bits dyn_EVP_PKEY_CTX_set_rsa_keygen_bits = NULL;
fn_EVP_PKEY_keygen dyn_EVP_PKEY_keygen = NULL;
fn_EVP_PKEY_CTX_free dyn_EVP_PKEY_CTX_free = NULL;
fn_EVP_PKEY_free dyn_EVP_PKEY_free = NULL;

fn_X509_new dyn_X509_new = NULL;
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

BOOL LoadOpenSSLDynamically(void) {
    if (g_OpenSSLLoaded) return TRUE;

    g_hCrypto = LoadLibraryA("libcrypto-3.dll");
    g_hSsl = LoadLibraryA("libssl-3.dll");

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

    // Verify all pointers
    if (!dyn_EVP_PKEY_CTX_new_id || !dyn_EVP_PKEY_keygen_init || !dyn_EVP_PKEY_CTX_set_rsa_keygen_bits ||
        !dyn_EVP_PKEY_keygen || !dyn_EVP_PKEY_CTX_free || !dyn_EVP_PKEY_free || !dyn_X509_new ||
        !dyn_X509_get_serialNumber || !dyn_ASN1_INTEGER_set || !dyn_X509_getm_notBefore ||
        !dyn_X509_getm_notAfter || !dyn_X509_gmtime_adj || !dyn_X509_set_pubkey ||
        !dyn_X509_get_subject_name || !dyn_X509_NAME_add_entry_by_txt || !dyn_X509_set_issuer_name ||
        !dyn_EVP_sha256 || !dyn_X509_sign || !dyn_X509_free || !dyn_OPENSSL_init_ssl ||
        !dyn_TLS_client_method || !dyn_TLS_server_method || !dyn_SSL_CTX_new || !dyn_SSL_CTX_set_verify ||
        !dyn_SSL_CTX_use_certificate || !dyn_SSL_CTX_use_PrivateKey || !dyn_SSL_CTX_free ||
        !dyn_SSL_new || !dyn_SSL_set_fd || !dyn_SSL_connect || !dyn_SSL_accept || !dyn_SSL_free ||
        !dyn_SSL_read || !dyn_SSL_write || !dyn_SSL_shutdown) {
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
