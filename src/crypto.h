#ifndef _CRYPTO_H
#define _CRYPTO_H

#include <Crypto.h>
#include <AES.h>
#include <GCM.h>
#include "secrets.h"

/*
 * AES-256 GCM
 */
class CryptUtilClass
{
private:
    const unsigned char key[32] = AES265_GCM_KEY;
    unsigned char iv[12] = {0};

    unsigned char tag[16];
    GCM<AES256> *gcmaes256 = 0;

public:
    CryptUtilClass();

    int encryptionOverhead = 16 + 12;

    bool encrypt(
        unsigned char plaintext[],
        const size_t plaintextLen,
        unsigned char *dstBuff,
        const size_t dstBuffLen);

    int decrypt(
        unsigned char ciphertextAndTagAndTime[],
        const size_t ciphertextAndTagAndTimeLength,
        unsigned char dstBuff[]);
};

extern CryptUtilClass CryptUtil;

#endif