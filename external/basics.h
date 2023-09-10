#ifndef BASICS_H__
#define BASICS_H__ 1

#if defined(BASICS_STATIC)
#  define BASICS__API static
#else
#  define BASICS__API
#endif

/* compiler tomfoolery {{{ */
#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif
/* }}} */

/* basics.h {{{
 *  ____    _    ____ ___ ____ ____   _   _ 
 * | __ )  / \  / ___|_ _/ ___/ ___| | | | |
 * |  _ \ / _ \ \___ \| | |   \___ \ | |_| |
 * | |_) / ___ \ ___) | | |___ ___) ||  _  |
 * |____/_/   \_\____/___\____|____(_)_| |_|
 */
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define basics_length_of(array) \
    (sizeof(array) / sizeof(array[0]))

#define basics_align(value, boundary) \
    (((value+boundary-1) / boundary) * boundary)

enum basics_memory_endianness {
    BASICS_MEMORY_LITTLE_ENDIAN,
    BASICS_MEMORY_BIG_ENDIAN,
};

BASICS__API int
basics_utf8_get_stride(int byte);

/** @return Literal `1`. */
BASICS__API size_t
basics_memory_write_byte(void *self, size_t const size, size_t const position,
                        int byte);

/** 
 * If `position < size`, writes 
 *     `self[position] = byte`. 
 * Otherwise, writes
 *     `self[size-1] = byte`.
 * The intented use is for safely building a null terminated string in a fixed
 * buffer.
 *
 * @return Literal `1`.
 */
BASICS__API size_t
basics_memory_write_guard(void *self, size_t const size, size_t const position,
                         int byte);

/** @return `length`. */
BASICS__API size_t
basics_memory_write_string(void *self, size_t const size, size_t const position,
                          void const *string, size_t const length);

/** @return Effectively, `strlen(literal)`. */
BASICS__API size_t
basics_memory_write_literal(void *self, size_t const size, size_t const position,
                           char const *literal);

/** @return Number of bytes required to armor `byte`. */
BASICS__API size_t
basics_memory_armor_byte(void *self, size_t const size, size_t const position,
                        int byte);

/** 
 * @return Number of bytes required to armor `string` for `length` bytes plus
 * the final null terminator. 
 */
BASICS__API size_t
basics_memory_armor_string(void *self, size_t const size, size_t const position, 
                          void const *string, size_t length);

/** @return `unit * count`. */
BASICS__API size_t
basics_memory_read(void *self, size_t const size, size_t const position, 
                  void *out, size_t const unit, size_t const count);

/** @return `sizeof(uint16_t) * count`. */
BASICS__API size_t
basics_memory_read_16le(void *self, size_t const size, size_t const position, 
                       void *out, size_t const count);

/** 
 * @return Number of bytes skipped including '\0'. If '\0' is not found
 * by region's end, '0' is returned instead.
 */
BASICS__API size_t
basics_memory_skip_string(void *self, size_t const size, size_t const position);

/** @return If within bounds, a value in [0;255], otherwise -1. */
BASICS__API int
basics_memory_get_byte(void *self, size_t const size, size_t const position);

/** @return `&((char *) self)[position]`. */
BASICS__API void *
basics_memory_get_address(void *self, size_t const position);

/** 
 * @return `true` if `position + length <= size` and 
 * `strncmp(self[position], prefix, length) == 0`. `false` otherwise.
 */
BASICS__API bool
basics_memory_has_prefix(void const *self, size_t const size,
                        size_t const position, void const *prefix, 
                        size_t const length);

/** @return obvious. */
BASICS__API enum basics_memory_endianness
basics_memory_get_endianness(void);
/*
 *
 *
 * }}} basics.h */

#if defined(BASICS_IMPLEMENTATION)

/* basics.c {{{
 *  ____    _    ____ ___ ____ ____   ____ 
 * | __ )  / \  / ___|_ _/ ___/ ___| / ___|
 * |  _ \ / _ \ \___ \| | |   \___ \| |    
 * | |_) / ___ \ ___) | | |___ ___) | |___ 
 * |____/_/   \_\____/___\____|____(_)____|
 */
BASICS__API int
basics_utf8_get_stride(int byte)
{
    assert(0x00 <= byte && byte <= 0xff);
    /* 
     * The rationale for the values is this (where '.' is an arbitrary binary digit):
     * 
     * For "0b 0... ...." the stride is  1; that is obvious.
     * For "0b 10.. ...." the stride is -1 to scan back to the starting byte (this is an intermediate byte.)
     * For "0b 110. ...." the stride is  2; that is obvious.
     * For "0b 1110 ...." the stride is  3; that is obvious.
     * For "0b 1111 0..." the stride is  4; that is obvious.
     * For "0b 1111 1..." the stride is  0 to indicate an invalid UTF-8 byte.
     */
    static int8_t const table[256] = {
          1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
          1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
          1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
          1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
          1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
          1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
          1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
          1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2,
          2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
          2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,
          3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,
          4,  4,  4,  4,  4,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    };
    return table[byte];
}

BASICS__API size_t
basics_memory_write_byte(void *self, size_t const size, size_t const position,
                        int byte)
{
    assert(self != NULL);
    assert(0x00 <= byte && byte <= 0xff);

    if (position < size) {
        ((char *) self)[position] = byte;
    }
    return 1;
}

BASICS__API size_t
basics_memory_write_guard(void *self, size_t const size, size_t const position,
                         int byte)
{
    assert(self != NULL);
    assert(0x00 <= byte && byte <= 0xff);

    char *const alias = (char *) self;
    alias[(position < size) ? (position) : (size-1)] = byte;

    return 1;
}

BASICS__API size_t
basics_memory_write_string(void *self, size_t const size, size_t const position,
                          void const *string, size_t const length)
{
    assert(self != NULL);
    assert(string != NULL);
    
    if (position < size) {
        char *const alias = (char *) self;
        size_t const remain = size - position;
        size_t const limit = length < remain
                           ? length
                           : remain;
        memmove(&alias[position], string, limit);
    }
    return length;
}

BASICS__API size_t
basics_memory_write_literal(void *self, size_t const size, size_t const position,
                            char const *literal)
{
    assert(self != NULL);
    assert(literal != NULL);

    return basics_memory_write_string(self, size, position, literal,
                                      strlen(literal));
}

BASICS__API size_t
basics_memory_armor_byte(void *self, size_t const size, size_t const position,
                        int byte)
{
    assert(self != NULL);
    assert(0x00 <= byte && byte <= 0xff);

    static char const hex[] = "0123456789abcdef";

    void *const m = self;
    size_t const s = size;
    size_t const p = position;
    size_t c = 0;

    if (0x21 <= byte && byte <= 0x7e) { /* printable excluding space */
        c = basics_memory_write_byte(m, s, p, byte);
        goto done;
    }

    switch (byte) {

    case '\a':   c = basics_memory_write_literal(m, s, p, "\\a"); break;
    case '\b':   c = basics_memory_write_literal(m, s, p, "\\b"); break;
    case '\t':   c = basics_memory_write_literal(m, s, p, "\\t"); break;
    case '\n':   c = basics_memory_write_literal(m, s, p, "\\n"); break;
    case '\v':   c = basics_memory_write_literal(m, s, p, "\\v"); break;
    case '\f':   c = basics_memory_write_literal(m, s, p, "\\f"); break;
    case '\r':   c = basics_memory_write_literal(m, s, p, "\\r"); break;
    case '\x1b': c = basics_memory_write_literal(m, s, p, "\\e"); break;

    default: {
        int hi = (byte >> 4) & 0xf;
        int lo = (byte     ) & 0xf;
        c += basics_memory_write_literal(m, s, p+c, "\\x");
        c += basics_memory_write_byte(m, s, p+c, hex[hi]);
        c += basics_memory_write_byte(m, s, p+c, hex[lo]);
    } break;

    }

done:
    return c;
}

BASICS__API size_t
basics_memory_armor_string(void *self, size_t const size, size_t const position, 
                          void const *string, size_t length)
{
    assert(self != NULL);
    assert(string != NULL);

    char const *alias = (char const *) string;

    size_t pos = position;

    for (size_t i = 0; i < length; ++i) {
        pos += basics_memory_armor_byte(self, size, pos, (uint8_t) alias[i]);
    }
    pos += basics_memory_write_guard(self, size, pos, '\0');

    return pos - position;
}

BASICS__API size_t
basics_memory_read(void *self, size_t const size, size_t const position,
                  void *out, size_t const unit, size_t const count)
{
    assert(self != NULL);
    assert(out != NULL);
    assert(unit != 0);

    size_t const bytes = unit * count; /* FIXME no overflow check */
    if (size < position + bytes) { /* FIXME no overflow check */
        goto done;
    }

    char const *alias = (char const *) self;
    memmove(out, &alias[position], bytes);

done:
    return bytes;
}

BASICS__API size_t
basics_memory_read_16le(void *self, size_t const size, size_t const position, 
                       void *out, size_t const count)
{
    assert(self != NULL);
    assert(out != NULL);

    size_t const bytes = 
        basics_memory_read(self, size, position, out, sizeof(uint16_t), count);
    if (size < position + bytes) {
        goto done;
    }
    
    if (basics_memory_get_endianness() == BASICS_MEMORY_LITTLE_ENDIAN) {
        goto done;
    }

    uint16_t *alias16 = (uint16_t *) out;
    for (size_t i = 0; i < count; ++i) {
        uint8_t *alias8 = (uint8_t *) &alias16[i];
        alias16[i] = (alias8[1] << 1)
                   | (alias8[0]     );
    }

done:
    return bytes;
}

BASICS__API size_t
basics_memory_skip_string(void *self, size_t const size, size_t const position)
{
    assert(self != NULL);

    size_t index = position;

    if (size <= position) {
        goto done;
    }

    char const *alias = (char const *) self;
    while (index < size && alias[index] != 0) {
        ++index;
    }
    if (index < size) {
        ++index; /* skip null terminator */
    }
    else {
        index = position; /* no null terminator => no string to skip! */
    }

done:
    return index - position;
}

BASICS__API int
basics_memory_get_byte(void *self, size_t const size, size_t const position)
{
    assert(self != NULL);

    uint8_t const *alias = (uint8_t const *) self;
    return position < size ? alias[position] : -1;
}

BASICS__API void *
basics_memory_get_address(void *self, size_t const position)
{
    assert(self != NULL);

    return &((char *) self)[position];
}

BASICS__API bool
basics_memory_has_prefix(void const *self_, size_t const size, 
                        size_t const position, void const *prefix_, 
                        size_t const length)
{
    assert(self_ != NULL);
    assert(prefix_ != NULL);

    char const *self = (char const *) self_;
    char const *prefix = (char const *) prefix_;

    return (position + length <= size)
        && strncmp(&self[position], prefix, length) == 0;
}

BASICS__API enum basics_memory_endianness
basics_memory_get_endianness(void) 
{
    volatile int cell = 255;
    return *(char *) &cell
        ? BASICS_MEMORY_LITTLE_ENDIAN
        : BASICS_MEMORY_BIG_ENDIAN;
}
/*
 *
 *
 * }}} basics.c */

#endif /* BASICS_IMPLEMENTATION */

/* compiler tomfoolery {{{ */
#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
/* }}} */

#endif /* BASICS_H__ */

/* vi:set sw=4 sts=4 ts=4 et fdm=marker: */
