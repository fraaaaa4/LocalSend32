#ifndef CERT_H
#define CERT_H

#include <windows.h>
#include <wincrypt.h>

// Generates an in-memory, self-signed certificate for securing incoming HTTPS connections.
PCCERT_CONTEXT CreateSelfSignedCertificate();

#endif

