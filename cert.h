#ifndef CERT_H
#define CERT_H

#include <windows.h>
#include <wincrypt.h>

PCCERT_CONTEXT CreateSelfSignedCertificate();

#endif
