#include "tls_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include "openssl_dyn.h"

#define SSL_CTX_new dyn_SSL_CTX_new
#define SSL_CTX_free dyn_SSL_CTX_free
#define SSL_CTX_set_verify dyn_SSL_CTX_set_verify
#define SSL_CTX_use_certificate dyn_SSL_CTX_use_certificate
#define SSL_CTX_use_PrivateKey dyn_SSL_CTX_use_PrivateKey
#define TLS_client_method dyn_TLS_client_method
#define TLS_server_method dyn_TLS_server_method
#define SSL_new dyn_SSL_new
#define SSL_set_fd dyn_SSL_set_fd
#define SSL_connect dyn_SSL_connect
#define SSL_accept dyn_SSL_accept
#define SSL_free dyn_SSL_free
#define SSL_read dyn_SSL_read
#define SSL_write dyn_SSL_write
#define SSL_shutdown dyn_SSL_shutdown

#define EVP_PKEY_CTX_new_id dyn_EVP_PKEY_CTX_new_id
#define EVP_PKEY_keygen_init dyn_EVP_PKEY_keygen_init
#define EVP_PKEY_CTX_set_rsa_keygen_bits dyn_EVP_PKEY_CTX_set_rsa_keygen_bits
#define EVP_PKEY_keygen dyn_EVP_PKEY_keygen
#define EVP_PKEY_CTX_free dyn_EVP_PKEY_CTX_free
#define EVP_PKEY_free dyn_EVP_PKEY_free

#define X509_new dyn_X509_new
#define X509_get_serialNumber dyn_X509_get_serialNumber
#define ASN1_INTEGER_set dyn_ASN1_INTEGER_set
#define X509_getm_notBefore dyn_X509_getm_notBefore
#define X509_getm_notAfter dyn_X509_getm_notAfter
#define X509_gmtime_adj dyn_X509_gmtime_adj
#define X509_set_pubkey dyn_X509_set_pubkey
#define X509_get_subject_name dyn_X509_get_subject_name
#define X509_NAME_add_entry_by_txt dyn_X509_NAME_add_entry_by_txt
#define X509_set_issuer_name dyn_X509_set_issuer_name
#define EVP_sha256 dyn_EVP_sha256
#define X509_sign dyn_X509_sign
#define X509_free dyn_X509_free


static SSL_CTX* g_sslCtxClient = NULL;
static SSL_CTX* g_sslCtxServer = NULL;

struct TlsSocket {
    SOCKET sock;
    SSL* ssl;
};

static EVP_PKEY* generatePrivateKey() {
    EVP_PKEY* pkey = NULL;
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (pctx) {
        if (EVP_PKEY_keygen_init(pctx) > 0) {
            EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048);
            EVP_PKEY_keygen(pctx, &pkey);
        }
        EVP_PKEY_CTX_free(pctx);
    }
    return pkey;
}

static X509* generateSelfSignedCertificate(EVP_PKEY* pkey) {
    X509* x509 = X509_new();
    if (!x509) return NULL;

    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 31536000L); // 1 year

    X509_set_pubkey(x509, pkey);

    X509_NAME* name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"LocalSendRT", -1, -1, 0);
    X509_set_issuer_name(x509, name);

    if (!X509_sign(x509, pkey, EVP_sha256())) {
        X509_free(x509);
        return NULL;
    }
    return x509;
}

bool TlsInitGlobal() {
    if (!LoadOpenSSLDynamically()) return false;
    dyn_OPENSSL_init_ssl(0, NULL);

    g_sslCtxClient = SSL_CTX_new(TLS_client_method());
    if (!g_sslCtxClient) return false;
    SSL_CTX_set_verify(g_sslCtxClient, SSL_VERIFY_NONE, NULL);

    g_sslCtxServer = SSL_CTX_new(TLS_server_method());
    if (!g_sslCtxServer) return false;

    EVP_PKEY* pkey = generatePrivateKey();
    if (pkey) {
        X509* cert = generateSelfSignedCertificate(pkey);
        if (cert) {
            SSL_CTX_use_certificate(g_sslCtxServer, cert);
            SSL_CTX_use_PrivateKey(g_sslCtxServer, pkey);
            X509_free(cert);
        }
        EVP_PKEY_free(pkey);
    }
    return true;
}

void TlsCleanupGlobal() {
    if (g_sslCtxClient) { SSL_CTX_free(g_sslCtxClient); g_sslCtxClient = NULL; }
    if (g_sslCtxServer) { SSL_CTX_free(g_sslCtxServer); g_sslCtxServer = NULL; }
    FreeOpenSSLDynamically();
}

TlsSocket* TlsConnect(SOCKET sock, const char* targetIP) {
    if (!g_sslCtxClient) return NULL;
    TlsSocket* tls = (TlsSocket*)malloc(sizeof(TlsSocket));
    if (!tls) return NULL;

    tls->sock = sock;
    tls->ssl = SSL_new(g_sslCtxClient);
    if (!tls->ssl) {
        free(tls);
        return NULL;
    }

    SSL_set_fd(tls->ssl, (int)sock);
    if (SSL_connect(tls->ssl) <= 0) {
        SSL_free(tls->ssl);
        free(tls);
        return NULL;
    }
    return tls;
}

TlsSocket* TlsAccept(SOCKET clientSock) {
    if (!g_sslCtxServer) return NULL;
    TlsSocket* tls = (TlsSocket*)malloc(sizeof(TlsSocket));
    if (!tls) return NULL;

    tls->sock = clientSock;
    tls->ssl = SSL_new(g_sslCtxServer);
    if (!tls->ssl) {
        free(tls);
        return NULL;
    }

    SSL_set_fd(tls->ssl, (int)clientSock);
    if (SSL_accept(tls->ssl) <= 0) {
        SSL_free(tls->ssl);
        free(tls);
        return NULL;
    }
    return tls;
}

int TlsRead(TlsSocket* tls, char* outBuffer, int maxLen) {
    if (!tls || !tls->ssl) return -1;
    return SSL_read(tls->ssl, outBuffer, maxLen);
}

int TlsWrite(TlsSocket* tls, const char* message, int len) {
    if (!tls || !tls->ssl) return -1;
    return SSL_write(tls->ssl, message, len);
}

void TlsFreeSocket(TlsSocket* tls) {
    if (tls) {
        if (tls->ssl) {
            SSL_shutdown(tls->ssl);
            SSL_free(tls->ssl);
        }
        free(tls);
    }
}
