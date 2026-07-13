#ifndef OPENSSL_DYN_H
#define OPENSSL_DYN_H

#include <windows.h>
#undef X509_NAME

#include <openssl/ssl.h>
#include <openssl/err.h>

// Loader state
extern BOOL g_OpenSSLLoaded;
BOOL LoadOpenSSLDynamically(void);
void FreeOpenSSLDynamically(void);

// Function pointers
typedef int (*fn_OPENSSL_init_ssl)(unsigned long long opts, const void* settings);
typedef const SSL_METHOD* (*fn_TLS_client_method)(void);
typedef const SSL_METHOD* (*fn_TLS_server_method)(void);
typedef SSL_CTX* (*fn_SSL_CTX_new)(const SSL_METHOD* meth);
typedef void (*fn_SSL_CTX_set_verify)(SSL_CTX* ctx, int mode, int (*callback)(int, X509_STORE_CTX*));
typedef int (*fn_SSL_CTX_use_certificate)(SSL_CTX* ctx, X509* x);
typedef int (*fn_SSL_CTX_use_PrivateKey)(SSL_CTX* ctx, EVP_PKEY* pkey);
typedef void (*fn_SSL_CTX_free)(SSL_CTX* ctx);
typedef SSL* (*fn_SSL_new)(SSL_CTX* ctx);
typedef int (*fn_SSL_set_fd)(SSL* s, int fd);
typedef int (*fn_SSL_connect)(SSL* s);
typedef int (*fn_SSL_accept)(SSL* s);
typedef void (*fn_SSL_free)(SSL* ssl);
typedef int (*fn_SSL_read)(SSL* ssl, void* buf, int num);
typedef int (*fn_SSL_write)(SSL* ssl, const void* buf, int num);
typedef int (*fn_SSL_shutdown)(SSL* s);

typedef EVP_PKEY_CTX* (*fn_EVP_PKEY_CTX_new_id)(int id, ENGINE* e);
typedef int (*fn_EVP_PKEY_keygen_init)(EVP_PKEY_CTX* ctx);
typedef int (*fn_EVP_PKEY_CTX_set_rsa_keygen_bits)(EVP_PKEY_CTX* ctx, int bits);
typedef int (*fn_EVP_PKEY_keygen)(EVP_PKEY_CTX* ctx, EVP_PKEY** ppkey);
typedef void (*fn_EVP_PKEY_CTX_free)(EVP_PKEY_CTX* ctx);
typedef void (*fn_EVP_PKEY_free)(EVP_PKEY* pkey);

typedef X509* (*fn_X509_new)(void);
typedef ASN1_INTEGER* (*fn_X509_get_serialNumber)(X509* x);
typedef int (*fn_ASN1_INTEGER_set)(ASN1_INTEGER* a, long v);
typedef ASN1_TIME* (*fn_X509_getm_notBefore)(const X509* x);
typedef ASN1_TIME* (*fn_X509_getm_notAfter)(const X509* x);
typedef ASN1_TIME* (*fn_X509_gmtime_adj)(ASN1_TIME* s, long adj);
typedef int (*fn_X509_set_pubkey)(X509* x, EVP_PKEY* pkey);
typedef X509_NAME* (*fn_X509_get_subject_name)(const X509* x);
typedef int (*fn_X509_NAME_add_entry_by_txt)(X509_NAME* name, const char* field, int type, const unsigned char* bytes, int len, int loc, int set);
typedef int (*fn_X509_set_issuer_name)(X509* x, const X509_NAME* name);
typedef const EVP_MD* (*fn_EVP_sha256)(void);
typedef int (*fn_X509_sign)(X509* x, EVP_PKEY* pkey, const EVP_MD* md);
typedef void (*fn_X509_free)(X509* x);

// Extern pointers
extern fn_OPENSSL_init_ssl dyn_OPENSSL_init_ssl;
extern fn_TLS_client_method dyn_TLS_client_method;
extern fn_TLS_server_method dyn_TLS_server_method;
extern fn_SSL_CTX_new dyn_SSL_CTX_new;
extern fn_SSL_CTX_set_verify dyn_SSL_CTX_set_verify;
extern fn_SSL_CTX_use_certificate dyn_SSL_CTX_use_certificate;
extern fn_SSL_CTX_use_PrivateKey dyn_SSL_CTX_use_PrivateKey;
extern fn_SSL_CTX_free dyn_SSL_CTX_free;
extern fn_SSL_new dyn_SSL_new;
extern fn_SSL_set_fd dyn_SSL_set_fd;
extern fn_SSL_connect dyn_SSL_connect;
extern fn_SSL_accept dyn_SSL_accept;
extern fn_SSL_free dyn_SSL_free;
extern fn_SSL_read dyn_SSL_read;
extern fn_SSL_write dyn_SSL_write;
extern fn_SSL_shutdown dyn_SSL_shutdown;

extern fn_EVP_PKEY_CTX_new_id dyn_EVP_PKEY_CTX_new_id;
extern fn_EVP_PKEY_keygen_init dyn_EVP_PKEY_keygen_init;
extern fn_EVP_PKEY_CTX_set_rsa_keygen_bits dyn_EVP_PKEY_CTX_set_rsa_keygen_bits;
extern fn_EVP_PKEY_keygen dyn_EVP_PKEY_keygen;
extern fn_EVP_PKEY_CTX_free dyn_EVP_PKEY_CTX_free;
extern fn_EVP_PKEY_free dyn_EVP_PKEY_free;

extern fn_X509_new dyn_X509_new;
extern fn_X509_get_serialNumber dyn_X509_get_serialNumber;
extern fn_ASN1_INTEGER_set dyn_ASN1_INTEGER_set;
extern fn_X509_getm_notBefore dyn_X509_getm_notBefore;
extern fn_X509_getm_notAfter dyn_X509_getm_notAfter;
extern fn_X509_gmtime_adj dyn_X509_gmtime_adj;
extern fn_X509_set_pubkey dyn_X509_set_pubkey;
extern fn_X509_get_subject_name dyn_X509_get_subject_name;
extern fn_X509_NAME_add_entry_by_txt dyn_X509_NAME_add_entry_by_txt;
extern fn_X509_set_issuer_name dyn_X509_set_issuer_name;
extern fn_EVP_sha256 dyn_EVP_sha256;
extern fn_X509_sign dyn_X509_sign;
extern fn_X509_free dyn_X509_free;

#endif
