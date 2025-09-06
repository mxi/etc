#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#define CHACHA_IMPL
#include "chacha.h"

static uint8_t secret[64] = "this message is a secret!";
static uint8_t cipher_text[64];
static uint8_t cipher_next[64];
static uint8_t plain_text[64];

int main(void)
{
    uint8_t key[32] = "this key is also a secret!";
    uint8_t iv[12] = "can be known";

    struct chacha chacha;
    chacha_xinit(&chacha, key, iv);
    chacha_xor(&chacha, 64, cipher_text, secret);
    chacha_xor(&chacha, 64, cipher_next, secret);
    chacha_xinit(&chacha, key, iv);
    chacha_xor(&chacha, 64, plain_text, cipher_text);

    printf("secret:      ");
    for (int i = 0; i < 64; ++i)
    {
        printf("%02x", secret[i]);
    }
    printf("\n");

    printf("cipher text: ");
    for (int i = 0; i < 64; ++i)
    {
        printf("%02x", cipher_text[i]);
    }
    printf("\n");

    printf("cipher next: ");
    for (int i = 0; i < 64; ++i)
    {
        printf("%02x", cipher_next[i]);
    }
    printf("\n");

    printf("plain text:  ");
    for (int i = 0; i < 64; ++i)
    {
        printf("%02x", plain_text[i]);
    }
    printf("\n");

    return 0;
}