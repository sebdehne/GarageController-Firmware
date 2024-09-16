#include "crypto.h"
#include "utils.h"
#include "logger.h"

CryptUtilClass::CryptUtilClass()
{
    gcmaes256 = new GCM<AES256>();
}

bool CryptUtilClass::encrypt(
    unsigned char *plaintext,
    const size_t plaintextLen,
    unsigned char *dstBuff,
    const size_t dstBuffLen)
{

    size_t totalLength = plaintextLen + sizeof(tag) + sizeof(iv);
    if (dstBuffLen < totalLength)
    {
        Log.log("cannot encrypt() - dstBuffLen too small");
        return false;
    }

    // setup
    gcmaes256->clear();
    gcmaes256->setKey(key, gcmaes256->keySize());

    // generate random IV
    for (unsigned int i = 0; i < sizeof(iv); i++)
    {
        iv[i] = random(0, 255);
    }
    gcmaes256->setIV(iv, sizeof(iv));

    // perform the encryption
    gcmaes256->encrypt(dstBuff, plaintext, plaintextLen);
    // compute the authentication tag
    gcmaes256->computeTag(tag, sizeof(tag));

    // append the authentication tag to dstBuff
    dstBuff += plaintextLen;           // move the pointer forward to the end of the cipher text
    memcpy(dstBuff, tag, sizeof(tag)); // and append computed tag

    // finally, append the IV/Nonce in clear text
    dstBuff += sizeof(tag);          // move the pointer to after the tag
    memcpy(dstBuff, iv, sizeof(iv)); // and append IV/Nonce

    return true;
}

int CryptUtilClass::decrypt(
    unsigned char cipherTextWithIv[],
    const size_t cipherTextWithIvLength,
    unsigned char dstBuff[])
{
    gcmaes256->clear();

    size_t ciphertextSize = cipherTextWithIvLength - sizeof(tag) - sizeof(iv);

    unsigned char *ptr = cipherTextWithIv;

    // extract the tag
    ptr += ciphertextSize;         // move pointer to after the ciphertext / start og tag
    memcpy(tag, ptr, sizeof(tag)); // copy out tag

    // extract the IV/Nonce
    ptr += sizeof(tag);          // move pointer to after the tag
    memcpy(iv, ptr, sizeof(iv)); // copy out IV

    // setup cipher
    gcmaes256->setKey(key, gcmaes256->keySize());

    // set IV/Nonce
    gcmaes256->setIV(iv, sizeof(iv));

    // decrypt the data (doesn't validate the tag)
    gcmaes256->decrypt(dstBuff, cipherTextWithIv, ciphertextSize);

    // check the tag
    if (!gcmaes256->checkTag(tag, sizeof(tag)))
    {
        Serial.println("Tag-validation failed");
        return -1;
    }

    return ciphertextSize;
}

CryptUtilClass CryptUtil;
