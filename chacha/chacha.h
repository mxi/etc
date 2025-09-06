#ifndef CHACHA_H
#define CHACHA_H 1

#include <assert.h>
#include <string.h>
#include <stdint.h>

struct chacha
{
    uint32_t state[16];
};

void chacha_xinit(struct chacha *self, uint8_t const key[32], uint8_t const iv[12]);
void chacha_xor(struct chacha *self, size_t n, uint8_t *out, uint8_t const *in);
void chacha_erase(struct chacha *self);

#if defined(CHACHA_IMPL)
static uint32_t rotate_left_32(uint32_t v, uint32_t c)
{
    uint32_t m = 31;
    uint32_t d = c & m;
    return (v << d) | (v >> ((-d) & m));
}

static void u32_to_u8_le(uint8_t *out, uint32_t v)
{
    out[0] = (v      ) & 0xff;
    out[1] = (v >>  8) & 0xff;
    out[2] = (v >> 16) & 0xff;
    out[3] = (v >> 24) & 0xff;
}

static uint32_t u8_to_u32_le(uint8_t const *bytes)
{
    return (bytes[0]      )
         | (bytes[1] <<  8)
         | (bytes[2] << 16)
         | (bytes[3] << 24);
}

static void chacha_quarter_round(uint32_t x[16], uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    x[a] = x[a] + x[b];
    x[d] = rotate_left_32(x[d] ^ x[a], 16);
    x[c] = x[c] + x[d];
    x[b] = rotate_left_32(x[b] ^ x[c], 12);
    x[a] = x[a] + x[b];
    x[d] = rotate_left_32(x[d] ^ x[a],  8);
    x[c] = x[c] + x[d];
    x[b] = rotate_left_32(x[b] ^ x[c],  7);
}

static void chacha_n(int n, uint8_t out_stream[64], uint32_t state[16])
{
    assert((8 <= n && n <= 20) && (n % 2 == 0));

    uint32_t x[16];

    memcpy(x, state, 64);

    for (int i = n; i > 0; i -= 2)
    {
        chacha_quarter_round(x, 0, 4,  8, 12);
        chacha_quarter_round(x, 1, 5,  9, 13);
        chacha_quarter_round(x, 2, 6, 10, 14);
        chacha_quarter_round(x, 3, 7, 11, 15);

        chacha_quarter_round(x, 0, 5, 10, 15);
        chacha_quarter_round(x, 1, 6, 11, 12);
        chacha_quarter_round(x, 2, 7,  8, 13);
        chacha_quarter_round(x, 3, 4,  9, 14);
    }

    for (int i = 0; i < 16; ++i)
    {
        x[i] += state[i];
    }

    for (int i = 0; i < 16; ++i)
    {
        u32_to_u8_le(out_stream + 4 * i, x[i]);
    }
}

static const uint8_t sigma[16] = "expand 32-byte k";

void chacha_xinit(struct chacha *self, uint8_t const key[32], uint8_t const iv[12])
{
    self->state[0] = u8_to_u32_le(sigma + 0);
    self->state[1] = u8_to_u32_le(sigma + 4);
    self->state[2] = u8_to_u32_le(sigma + 8);
    self->state[3] = u8_to_u32_le(sigma + 12);

    self->state[4] = u8_to_u32_le(key + 0);
    self->state[5] = u8_to_u32_le(key + 4);
    self->state[6] = u8_to_u32_le(key + 8);
    self->state[7] = u8_to_u32_le(key + 12);
    self->state[8] = u8_to_u32_le(key + 16);
    self->state[9] = u8_to_u32_le(key + 20);
    self->state[10] = u8_to_u32_le(key + 24);
    self->state[11] = u8_to_u32_le(key + 28);

    self->state[12] = 0;
    self->state[13] = u8_to_u32_le(iv + 0);
    self->state[14] = u8_to_u32_le(iv + 4);
    self->state[15] = u8_to_u32_le(iv + 8);
}

void chacha_xor(struct chacha *self, size_t n, uint8_t *out, uint8_t const *in)
{
    size_t m = n;

    while (0 < m)
    {
        uint8_t stream[64];
        chacha_n(12, stream, self->state); /* https://eprint.iacr.org/2019/1492.pdf */
        self->state[12] += 1; /* overflow is the responsibility of the user */
        size_t len = 64 < m ? 64 : m;
        for (size_t i = 0; i < len; ++i)
        {
            out[i] = stream[i] ^ in[i];
        }
        m -= 64;
        out += 64;
        in += 64;
    }
}

void chacha_erase(struct chacha *self)
{
    assert(self);

    /* https://github.com/jedisct1/libsodium/blob/be58b2e6664389d9c7993b55291402934b43b3ca/src/libsodium/sodium/utils.c#L78:L101 */
    uint8_t volatile *volatile m = (uint8_t volatile *volatile) self->state;

    for (size_t i = 0u; i < sizeof(self->state); ++i)
    {
        m[i] = 0u;
    }
}
#endif /* CHACHA_IMPL */

#endif /* CHACHA_H */