#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define BASICS_STATIC
#define BASICS_IMPLEMENTATION
#include "external/basics.h"

enum key {
    KEY_TAB       =   9, /* ascii tab */
    KEY_ENTER     =  10, /* ascii newline */
    KEY_ESCAPE    =  27, /* ascii escape */
    KEY_BACKSPACE = 127,
    KEY_UTF8      = 128,
    KEY_F1        = 129,
    KEY_F2        = 130,
    KEY_F3        = 131,
    KEY_F4        = 132,
    KEY_F5        = 133,
    KEY_F6        = 134,
    KEY_F7        = 135,
    KEY_F8        = 136,
    KEY_F9        = 137,
    KEY_F10       = 138,
    KEY_F11       = 139,
    KEY_F12       = 140,
    KEY_UP        = 142,
    KEY_DOWN      = 143,
    KEY_RIGHT     = 144,
    KEY_LEFT      = 145,
    KEY_HOME      = 146,
    KEY_INSERT    = 147,
    KEY_DELETE    = 148,
    KEY_END       = 149,
    KEY_PAGE_UP   = 150,
    KEY_PAGE_DOWN = 151,
    KEY_CENTER    = 152,
    KEY_UNKNOWN   = 255,
};

enum mod {
    MOD_SHIFT     = 0x01,
    MOD_ALT       = 0x02,
    MOD_CONTROL   = 0x04,
    MOD_META      = 0x08,
};

/* info {{{ */
enum info_status {
    INFO_STATUS_OK,
    INFO_STATUS_BAD,
    INFO_STATUS_KEY_COLLISION,
    INFO_STATUS_STRING_TOO_LONG,
    INFO_STATUS_NO_MEMORY,
};

struct info_entry {
    uint8_t size;
    uint8_t key;
    uint8_t mod;
    char string[];
};

struct info_table {
    void *base;
    size_t base_size;
    size_t base_used;
};

struct info_table_iterator {
    struct info_table *ref;
    size_t base_offset;
};

struct info_base {
    struct info_table string_table;
    uint8_t key_table[128];
};

static size_t
info_entry_get_string_length(struct info_entry *self)
{
    assert(self != NULL);
    return self->size - sizeof(struct info_entry);
}

static void
info_table_initialize(struct info_table *self)
{
    assert(self != NULL);
    memset(self, 0, sizeof(*self));
}

static void
info_table_decommission(struct info_table *self)
{
    if (self != NULL) {
        free(self->base);
        memset(self, 0, sizeof(*self));
    }
}

static enum info_status
info_table__append(struct info_table *self, struct info_entry **out, 
                   size_t string_length)
{
    assert(self != NULL);
    assert(self->base_used <= self->base_size);
    assert(out != NULL);
    enum info_status status = INFO_STATUS_OK;
    struct info_entry *entry = NULL;

    size_t const entry_size = sizeof(struct info_entry) + string_length;
    if (255 < entry_size) {
        status = INFO_STATUS_STRING_TOO_LONG;
        goto done;
    }
    size_t const size_required = self->base_used + entry_size;
    size_t const size_desired = basics_align(size_required, 256); /* MAGIC */

    if (self->base_size < size_desired) {
        void *new_base = realloc(self->base, size_desired);
        if (new_base == NULL) {
            status = INFO_STATUS_NO_MEMORY;
            goto done;
        }
        self->base = new_base;
        self->base_size = size_desired;
    }
    assert(self->base_used + entry_size <= self->base_size);

    uint8_t *alias = (uint8_t *) self->base;
    entry = (struct info_entry *) &alias[self->base_used];
    entry->size = (uint8_t) entry_size;
    self->base_used += entry_size;

done:
    *out = entry;
    return status;
}

static enum info_status
info_table_add(struct info_table *self, char const *string, size_t length,
               uint8_t key, uint8_t mod)
{
    assert(self != NULL);
    assert(string != NULL);
    enum info_status status = INFO_STATUS_OK;

    struct info_entry *e = NULL;
    status = info_table__append(self, &e, length);
    if (status != INFO_STATUS_OK) {
        goto done;
    }
    assert(e != NULL);
    e->key = key;
    e->mod = mod;
    strncpy(e->string, string, length);

done:
    return status;
}

static void
info_table_iterator_initialize(struct info_table_iterator *self, 
                               struct info_table *table)
{
    assert(self != NULL);
    assert(table != NULL);
    self->ref = table;
    self->base_offset = 0;
}

static struct info_entry *
info_table_iterator_next(struct info_table_iterator *self)
{
    assert(self != NULL);
    assert(self->ref != NULL);
    struct info_entry *result = NULL;

    if (self->ref->base_used <= self->base_offset) {
        goto done;
    }
    
    uint8_t *alias = (uint8_t *) self->ref->base;
    result = (struct info_entry *) &alias[self->base_offset];
    self->base_offset += result->size;

done:
    return result;
}

static void
info_base_initialize(struct info_base *self)
{
    assert(self != NULL);
    memset(self, 0, sizeof(*self));
    info_table_initialize(&self->string_table);
    memset(self->key_table, -1, sizeof(self->key_table));
}

static void
info_base_decommission(struct info_base *self)
{
    if (self != NULL) {
        info_table_decommission(&self->string_table);
        memset(self, 0, sizeof(*self));
    }
}

static size_t 
info__scanf(char const *string, size_t string_length, char const *format, ...) /* FIXME temp */
{
    assert(string != NULL);
    assert(format != NULL);

    enum {
        STAT_OK,
        STAT_BAD,
        STAT_BAD_FORMAT,
        STAT_BAD_MATCH,
        STAT_BAD_PARAMETER,
        STAT_PARAMETER_OVERFLOW,
        STAT_STRING_TOO_SHORT,
        STAT_WTF,
    } status = STAT_OK;

    size_t const format_length = strlen(format);
    size_t s = 0, f = 0;
    va_list ap;
    va_start(ap, format);

    while (f < format_length) {
        enum {
            MATCH_TYPE_VERBATIM,
            MATCH_TYPE_CHARACTER,
            MATCH_TYPE_PARAMETER,
        } type = MATCH_TYPE_VERBATIM;

        int const predicate = format[f];
        if (predicate == '%') {
            /* FIXME need custom strncmp for bounds correctness */
            if (strncmp(&format[f], "%c", 2) == 0) { 
                type = MATCH_TYPE_CHARACTER;
                f += 2;
            }
            else if (strncmp(&format[f], "%p", 2) == 0) {
                type = MATCH_TYPE_PARAMETER;
                f += 2;
            }
            else if (strncmp(&format[f], "%%", 2) == 0) {
                type = MATCH_TYPE_VERBATIM;
                f += 2;
            }
            else {
                assert(!STAT_BAD_FORMAT);
                status = STAT_BAD_FORMAT;
                goto done;
            }
        }
        else {
            ++f;
        }

        if (string_length <= s) {
            status = STAT_STRING_TOO_SHORT;
            goto done;
        }
        int const poked = string[s];

        switch (type) {

        case MATCH_TYPE_VERBATIM: {
            if (poked != predicate) {
                status = STAT_BAD_MATCH;
                goto done;
            }
            ++s;
        } break;

        case MATCH_TYPE_CHARACTER: {
            char *out = va_arg(ap, char *);
            assert(out != NULL);
            *out = string[s++];
        } break;

        case MATCH_TYPE_PARAMETER: {
            if (poked < '0' || '9' < poked) {
                status = STAT_BAD_PARAMETER;
                goto done;
            }
            uint8_t *out = va_arg(ap, uint8_t *);
            assert(out != NULL);
            uintptr_t result = 0;
            while (s < string_length && '0' <= string[s] && string[s] <= '9') {
                result *= 10;
                result += (string[s] - '0');
                if (255 < result) {
                    status = STAT_PARAMETER_OVERFLOW;
                    goto done;
                }
                ++s;
            }
            *out = result;
        } break;

        default: {
            assert(0);
            status = STAT_WTF;
            goto done;
        } break;

        } /* switch (type) */
    }

done:
    va_end(ap);
    return status != STAT_OK ? 0 : s;
}

static enum info_status
info_base_add_input(struct info_base *self, char const *string, size_t length,
                    uint8_t key, uint8_t mod)
{
    assert(self != NULL);
    assert(string != NULL);
    assert(length != 0);

    enum info_status status = INFO_STATUS_OK;

    /* Optimize for keycode/modcode encoding of escape sequences. */
    uint8_t keycode, modcode;

    /* 
     * NOTE the following abuses comma expressions a little to ensure that
     * a failed scanf doesn't clobber a keycode/modcode while simultaneously
     * not increasing the number of lines more than 2x.
     */

    if ((keycode = -1, modcode = -1, 
         info__scanf(string, length, "\033[%p;%p~", &keycode, &modcode)) == length ||
        (keycode = -1, modcode = -1,
         info__scanf(string, length, "\033[%p~", &keycode)) == length)
    {
        /* 
         * ANSI requires incrementing the first two parameters in CSI. Since
         * the keycode in these two cases is specified as one such parameter,
         * we decrement it to keep consistent with the keycodes obtained from
         * characters.
         */
        keycode--;
        goto key_table;
    }

    if ((keycode = -1, modcode = -1,
         info__scanf(string, length, "\033[1;%p%c", &modcode, &keycode)) == length ||
        /* gnome-specific: */
        (keycode = -1, modcode = -1,
         info__scanf(string, length, "\033O1;%p%c", &modcode, &keycode)) == length ||
        /* konsole-specific: */
        (keycode = -1, modcode = -1,
         info__scanf(string, length, "\033O%p%c", &modcode, &keycode)) == length ||
        (keycode = -1, modcode = -1,
         info__scanf(string, length, "\033[%c", &keycode)) == length ||
        (keycode = -1, modcode = -1,
         info__scanf(string, length, "\033O%c", &keycode)) == length)
    {
        /* keycode is a character, no decrement necessary. */
        goto key_table;
    }

    goto string_table;

key_table:
    assert(keycode != (uint8_t) -1);

    if (basics_length_of(self->key_table) <= keycode) {
        goto string_table;
    }

    if (modcode != (uint8_t) -1 && (modcode-1) != mod) {
        /*
         * Why modcode-1? For the same reason keycode was decremented above if
         * specified as a CSI parameter. 
         *
         * In theory, modcode-1 should encode the same bitfield as we have 
         * defined with `enum mod`. Hence, if modcode-1 != mod, we cannot be
         * sure that the terminal will properly encode the key using CSI, so
         * we defer this sequence to the string table.
         */
        goto string_table;
    }

    uint8_t const existing_key = self->key_table[keycode];
    if (existing_key != (uint8_t) -1 && existing_key != key) 
    {
        /*
         * There is a collision between two different keys, so I opt to fail
         * immediately because (a) I doubt this will happen with many terminals,
         * if any, and (b) if this does happen, terminal-specific code can
         * be inserted to fix it for that terminal.
         */
        status = INFO_STATUS_KEY_COLLISION;
        goto done;
    }

    self->key_table[keycode] = key;
    goto done;

    /* 
     * No suitable encoding of the sequence, so we're forced to chuck it into
     * a string lookup table.
     */
string_table:
    status = info_table_add(&self->string_table, string, length, key, mod);

done:
    return status;
}
/* }}} info */

static void
dump_table(struct info_table *table)
{
    assert(table != NULL);

    int index = 0;
    struct info_entry *e = NULL;
    struct info_table_iterator it;
    info_table_iterator_initialize(&it, table);
    while ((e = info_table_iterator_next(&it)) != NULL) {
        char armored[128] = {0};
        basics_memory_armor_string(armored, sizeof(armored), 0,
                                   e->string, info_entry_get_string_length(e));
        printf("%d: (key %u, mod %01x) %s\n", index+1, e->key, e->mod, armored);
        ++index;
    }
}

static bool
load_file(char const *path, void **out_buffer, size_t *out_size)
{
    assert(path != NULL);
    assert(out_buffer != NULL);
    assert(out_size != NULL);

    bool succeeded = true;

    int file = -1;
    void *buffer = NULL;
    size_t size = 0;

    file = open(path, O_RDONLY);
    if (file < 0) {
        fprintf(stderr, "Failed to open file!\n");
        succeeded = false;
        goto done;
    }

    struct stat stat;
    if (fstat(file, &stat) < 0) {
        fprintf(stderr, "Failed to fstat file!\n");
        succeeded = false;
        goto done;
    }
    size = stat.st_size;

    buffer = malloc(size);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate buffer for the file contents!\n");
        succeeded = false;
        goto done;
    }

    if (read(file, buffer, size) < (ssize_t) size) {
        fprintf(stderr, "Failed to read file into contents buffer!\n");
        succeeded = false;
        goto done;
    }

done:
    if (!succeeded) {
        free(buffer);
        buffer = NULL;
        size = 0;
    }
    if (0 <= file) {
        close(file);
    }
    *out_buffer = buffer;
    *out_size = size;
    return succeeded;
}

int
main(int argc, char const *argv[])
{
    enum exit_code {
        EXIT_OK,
        EXIT_BAD,
    };
    struct buffer {
        void *base;
        size_t size;
    };
    enum exit_code exit_code = EXIT_OK;
    struct buffer content;
    struct info_base base;

    info_base_initialize(&base);

    /* what terminal */
    char const *term = getenv("TERM");
    if (2 <= argc) {
        term = argv[1];
    }
    if (term == NULL) {
        fprintf(stderr, "TERM not set!\n");
        exit_code = EXIT_BAD;
        goto done;
    }
    if (strlen(term) == 0) {
        fprintf(stderr, "TERM is empty!\n");
        exit_code = EXIT_BAD;
        goto done;
    }

    /* load in the terminfo compiled file */
    char path[128] = {0};
    snprintf(path, sizeof(path)-1, "/usr/share/terminfo/%c/%s", term[0], term);

    printf("PATH %s\n", path);
    if (!load_file(path, &content.base, &content.size)) {
        exit_code = EXIT_BAD;
        goto done;
    }

    /*
     * LOAD LEGACY PORTION
     */
    size_t number_data_type = 0;

    struct header {
        int16_t magic;
        int16_t names_size;
        int16_t booleans_count;
        int16_t numbers_count;
        int16_t string_table_entries;
        int16_t string_table_size;
    } header;

    {
        size_t const bytes = 
            basics_memory_read_16le(content.base, content.size, 0, &header, 6);
        if (content.size < bytes) {
            fprintf(stderr, "corrupted header!\n");
            exit_code = EXIT_BAD;
            goto done;
        }
    }
    printf("HEADER\n");
    printf("  magic: %06o\n",              header.magic);
    printf("  names_size: %d\n",           header.names_size);
    printf("  booleans_count: %d\n",       header.booleans_count);
    printf("  numbers_count: %d\n",        header.numbers_count);
    printf("  string_table_entries: %d\n", header.string_table_entries);
    printf("  string_table_size: %d\n",    header.string_table_size);

    if (header.magic == 0432) {
        printf("NOTE legacy format!\n");
        number_data_type = sizeof(int16_t);
    }
    else if (header.magic == 01036) {
        printf("NOTE extended format!\n");
        number_data_type = sizeof(int32_t);
    }
    else {
        printf("Unknown magic; aborting!\n");
        exit_code = EXIT_BAD;
        goto done;
    }
    assert(number_data_type != 0);
    
    if (header.magic < 0
        || header.names_size < 0
        || header.booleans_count < 0
        || header.numbers_count < 0
        || header.string_table_entries < 0
        || header.string_table_size < 0)
    {
        printf("Corrupted header: negative values found!\n");
    }

    size_t legacy_end_position = 0;
    {
        static struct index_keymod_map {
            int16_t index; /* aka. index into the string index table in terminfo */
            uint8_t key;
            uint8_t mod;
        } index_keymod_map[] = {
            {  55, .key=KEY_BACKSPACE, .mod=0                       }, /* BACKSPACE */
            {  59, .key=KEY_DELETE,    .mod=0                       }, /* DC */
            {  61, .key=KEY_DOWN,      .mod=0                       }, /* DOWN */
            {  66, .key=KEY_F1,        .mod=0                       }, /* F1 */
            {  67, .key=KEY_F10,       .mod=0                       }, /* F10 */
            {  68, .key=KEY_F2,        .mod=0                       }, /* F2 */
            {  69, .key=KEY_F3,        .mod=0                       }, /* F3 */
            {  70, .key=KEY_F4,        .mod=0                       }, /* F4 */
            {  71, .key=KEY_F5,        .mod=0                       }, /* F5 */
            {  72, .key=KEY_F6,        .mod=0                       }, /* F6 */
            {  73, .key=KEY_F7,        .mod=0                       }, /* F7 */
            {  74, .key=KEY_F8,        .mod=0                       }, /* F8 */
            {  75, .key=KEY_F9,        .mod=0                       }, /* F9 */
            {  76, .key=KEY_HOME,      .mod=0                       }, /* HOME */
            {  77, .key=KEY_INSERT,    .mod=0                       }, /* IC */
            {  79, .key=KEY_LEFT,      .mod=0                       }, /* LEFT */
            {  81, .key=KEY_PAGE_DOWN, .mod=0                       }, /* NPAGE */
            {  82, .key=KEY_PAGE_UP,   .mod=0                       }, /* PPAGE */
            {  83, .key=KEY_RIGHT,     .mod=0                       }, /* RIGHT */
            {  87, .key=KEY_UP,        .mod=0                       }, /* UP */
            { 141, .key=KEY_CENTER,    .mod=0                       }, /* B2 */
            { 148, .key=KEY_TAB,       .mod=MOD_SHIFT               }, /* BTAB */
            { 164, .key=KEY_END,       .mod=0                       }, /* END */
            { 165, .key=KEY_ENTER,     .mod=0                       }, /* ENTER */
            { 216, .key=KEY_F11,       .mod=0                       }, /* F11 */
            { 217, .key=KEY_F12,       .mod=0                       }, /* F12 */

            { 218, .key=KEY_F1,        .mod=MOD_SHIFT               }, /* S-F1 */
            { 219, .key=KEY_F2,        .mod=MOD_SHIFT               }, /* S-F2 */
            { 220, .key=KEY_F3,        .mod=MOD_SHIFT               }, /* S-F3 */
            { 221, .key=KEY_F4,        .mod=MOD_SHIFT               }, /* S-F4 */
            { 222, .key=KEY_F5,        .mod=MOD_SHIFT               }, /* S-F5 */
            { 223, .key=KEY_F6,        .mod=MOD_SHIFT               }, /* S-F6 */
            { 224, .key=KEY_F7,        .mod=MOD_SHIFT               }, /* S-F7 */
            { 225, .key=KEY_F8,        .mod=MOD_SHIFT               }, /* S-F8 */
            { 226, .key=KEY_F9,        .mod=MOD_SHIFT               }, /* S-F9 */
            { 227, .key=KEY_F10,       .mod=MOD_SHIFT               }, /* S-F10 */
            { 228, .key=KEY_F11,       .mod=MOD_SHIFT               }, /* S-F11 */
            { 229, .key=KEY_F12,       .mod=MOD_SHIFT               }, /* S-F12 */

            { 230, .key=KEY_F1,        .mod=MOD_CONTROL             }, /* C-F1 */
            { 231, .key=KEY_F2,        .mod=MOD_CONTROL             }, /* C-F2 */
            { 232, .key=KEY_F3,        .mod=MOD_CONTROL             }, /* C-F3 */
            { 233, .key=KEY_F4,        .mod=MOD_CONTROL             }, /* C-F4 */
            { 234, .key=KEY_F5,        .mod=MOD_CONTROL             }, /* C-F5 */
            { 235, .key=KEY_F6,        .mod=MOD_CONTROL             }, /* C-F6 */
            { 236, .key=KEY_F7,        .mod=MOD_CONTROL             }, /* C-F7 */
            { 237, .key=KEY_F8,        .mod=MOD_CONTROL             }, /* C-F8 */
            { 238, .key=KEY_F9,        .mod=MOD_CONTROL             }, /* C-F9 */
            { 239, .key=KEY_F10,       .mod=MOD_CONTROL             }, /* C-F10 */
            { 240, .key=KEY_F11,       .mod=MOD_CONTROL             }, /* C-F11 */
            { 241, .key=KEY_F12,       .mod=MOD_CONTROL             }, /* C-F12 */

            { 242, .key=KEY_F1,        .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F1 */
            { 243, .key=KEY_F2,        .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F2 */
            { 244, .key=KEY_F3,        .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F3 */
            { 245, .key=KEY_F4,        .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F4 */
            { 246, .key=KEY_F5,        .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F5 */
            { 247, .key=KEY_F6,        .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F6 */
            { 248, .key=KEY_F7,        .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F7 */
            { 249, .key=KEY_F8,        .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F8 */
            { 250, .key=KEY_F9,        .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F9 */
            { 251, .key=KEY_F10,       .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F10 */
            { 252, .key=KEY_F11,       .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F11 */
            { 253, .key=KEY_F12,       .mod=MOD_CONTROL | MOD_SHIFT }, /* C-S-F12 */

            { 254, .key=KEY_F1,        .mod=MOD_ALT                 }, /* A-F1 */
            { 255, .key=KEY_F2,        .mod=MOD_ALT                 }, /* A-F2 */
            { 256, .key=KEY_F3,        .mod=MOD_ALT                 }, /* A-F3 */
            { 257, .key=KEY_F4,        .mod=MOD_ALT                 }, /* A-F4 */
            { 258, .key=KEY_F5,        .mod=MOD_ALT                 }, /* A-F5 */
            { 259, .key=KEY_F6,        .mod=MOD_ALT                 }, /* A-F6 */
            { 260, .key=KEY_F7,        .mod=MOD_ALT                 }, /* A-F7 */
            { 261, .key=KEY_F8,        .mod=MOD_ALT                 }, /* A-F8 */
            { 262, .key=KEY_F9,        .mod=MOD_ALT                 }, /* A-F9 */
            { 263, .key=KEY_F10,       .mod=MOD_ALT                 }, /* A-F10 */
            { 264, .key=KEY_F11,       .mod=MOD_ALT                 }, /* A-F11 */
            { 265, .key=KEY_F12,       .mod=MOD_ALT                 }, /* A-F12 */

            { 266, .key=KEY_F1,        .mod=MOD_ALT | MOD_SHIFT     }, /* A-S-F1 */
            { 267, .key=KEY_F2,        .mod=MOD_ALT | MOD_SHIFT     }, /* A-S-F2 */
            { 268, .key=KEY_F3,        .mod=MOD_ALT | MOD_SHIFT     }, /* A-S-F3 */
        };

        size_t index_table_offset = 0;
        {
            size_t i = 0;
            i += sizeof(int16_t) * 6;
            i += sizeof(uint8_t) * header.names_size;
            i += sizeof(uint8_t) * header.booleans_count;
            i  = basics_align(i, sizeof(int16_t));
            i += number_data_type * header.numbers_count;
            index_table_offset = i;
        }
        assert(index_table_offset % 2 == 0);
        printf("index_table_offset: %lu\n", index_table_offset);

        size_t string_table_offset = 0;
        {
            size_t i = index_table_offset;
            i += sizeof(int16_t) * header.string_table_entries;
            string_table_offset = i;
        }
        printf("string_table_offset: %lu\n", string_table_offset);

        printf("LEGACY STRINGS\n");
        for (int i = 0; i < (int) basics_length_of(index_keymod_map); ++i) {
            struct index_keymod_map const *map = &index_keymod_map[i];
            if (header.string_table_entries <= map->index) {
                fprintf(stderr, "ref_index %d out of bounds!\n", map->index);
                continue;
            }

            int16_t string_index = -1;
            {
                size_t const position = 
                    index_table_offset + sizeof(int16_t) * map->index;
                size_t const bytes = 
                    basics_memory_read_16le(content.base, content.size, 
                                            position, &string_index, 1);
                if (content.size < position + bytes) {
                    fprintf(stderr, "ref_index %d out of file bounds!\n", 
                            map->index);
                    continue;
                }
            }
            if (string_index < 0) {
                continue;
            }
            if (header.string_table_size <= string_index) {
                fprintf(stderr, "string_index %d out of bounds!\n", string_index);
                continue;
            }

            char const *string = NULL;
            size_t length = 0;
            {
                size_t const position = string_table_offset + string_index;
                string = 
                    basics_memory_get_address(content.base, position);
                size_t const n = 
                    basics_memory_skip_string(content.base, content.size, 
                                              position);
                if (n == 0) {
                    fprintf(stderr, "El failed to read legacy string!\n");
                    goto done;
                }
                length = n-1;
            }

            char armored[128] = {0};
            basics_memory_armor_string(armored, sizeof(armored), 0,
                                       string, length);
            printf("%d: %d %s (length %lu)\n", i+1, map->index, armored, length);
            
            enum info_status status =
                info_base_add_input(&base, string, length, map->key, map->mod);
            if (status != INFO_STATUS_OK) {
                fprintf(stderr, "Failed to add string to info base!\n");
            }
        }

        legacy_end_position = string_table_offset + header.string_table_size;
    }
    assert(legacy_end_position != 0);
    printf("Legacy ends on %04lx!\n", legacy_end_position);

    size_t const ext_offset = basics_align(legacy_end_position, sizeof(int16_t));
    if (content.size <= ext_offset) {
        printf("End of terminfo file!\n");
        goto epilogue;
    }

    /*
     * LOAD EXTENDED PORTION
     */
    printf("%lu bytes of extended info awaits!\n", content.size - ext_offset);

    struct ext_header {
        int16_t booleans_count;
        int16_t numbers_count;
        int16_t strings_count;
        int16_t string_table_entries;
        int16_t string_table_size;
    } ext_header;

    {
        size_t const bytes =
            basics_memory_read_16le(content.base, content.size, ext_offset, 
                                    &ext_header, 5);
        if (content.size < ext_offset + bytes) {
            fprintf(stderr, "corrupted extended header!\n");
            exit_code = EXIT_BAD;
            goto done;
        }
    }
    printf("EXTENDED HEADER\n");
    printf("  booleans_count: %d\n",       ext_header.booleans_count);
    printf("  numbers_count: %d\n",        ext_header.numbers_count);
    printf("  strings_count: %d\n",        ext_header.strings_count);
    printf("  string_table_entries: %d\n", ext_header.string_table_entries);
    printf("  string_table_size: %d\n",    ext_header.string_table_size);

    {
        /*
         * names suffixed with
         *   * := MOD_SHIFT
         *   3 := MOD_ALT
         *   4 := MOD_ALT | MOD_SHIFT
         *   5 := MOD_CTRL
         *   6 := MOD_CTRL | MOD_SHIFT
         *   7 := MOD_CTRL | MOD_ALT
         *   8 := MOD_CTRL | MOD_ALT | MOD_SHIFT
         * For brevity, the .mod field does not explicitly OR the enum mod field.
         */
        static struct name_keymod_map {
            char const *name;
            uint8_t key;
            uint8_t mod;
        } name_keymod_map[] = {
            /* delete key */
            { "kDC",   .key=KEY_DELETE,    .mod=1 },
            { "kDC3",  .key=KEY_DELETE,    .mod=2 },
            { "kDC4",  .key=KEY_DELETE,    .mod=3 },
            { "kDC5",  .key=KEY_DELETE,    .mod=4 },
            { "kDC6",  .key=KEY_DELETE,    .mod=5 },
            { "kDC7",  .key=KEY_DELETE,    .mod=6 },
            /* arrow down key */
            { "kDN",   .key=KEY_DOWN,      .mod=1 },
            { "kDN3",  .key=KEY_DOWN,      .mod=2 },
            { "kDN4",  .key=KEY_DOWN,      .mod=3 },
            { "kDN5",  .key=KEY_DOWN,      .mod=4 },
            { "kDN6",  .key=KEY_DOWN,      .mod=5 },
            { "kDN7",  .key=KEY_DOWN,      .mod=6 },
            /* end (1 on keypad) */
            { "kEND",  .key=KEY_END,       .mod=1 },
            { "kEND3", .key=KEY_END,       .mod=2 },
            { "kEND4", .key=KEY_END,       .mod=3 },
            { "kEND5", .key=KEY_END,       .mod=4 },
            { "kEND6", .key=KEY_END,       .mod=5 },
            { "kEND7", .key=KEY_END,       .mod=6 },
            /* home */
            { "kHOM",  .key=KEY_HOME,      .mod=1 },
            { "kHOM3", .key=KEY_HOME,      .mod=2 },
            { "kHOM4", .key=KEY_HOME,      .mod=3 },
            { "kHOM5", .key=KEY_HOME,      .mod=4 },
            { "kHOM6", .key=KEY_HOME,      .mod=5 },
            { "kHOM7", .key=KEY_HOME,      .mod=6 },
            /* insert */
            { "kIC",   .key=KEY_INSERT,    .mod=1 },
            { "kIC3",  .key=KEY_INSERT,    .mod=2 },
            { "kIC4",  .key=KEY_INSERT,    .mod=3 },
            { "kIC5",  .key=KEY_INSERT,    .mod=4 },
            { "kIC6",  .key=KEY_INSERT,    .mod=5 },
            { "kIC7",  .key=KEY_INSERT,    .mod=6 },
            /* arrow-left */
            { "kLFT",  .key=KEY_LEFT,      .mod=1 },
            { "kLFT3", .key=KEY_LEFT,      .mod=2 },
            { "kLFT4", .key=KEY_LEFT,      .mod=3 },
            { "kLFT5", .key=KEY_LEFT,      .mod=4 },
            { "kLFT6", .key=KEY_LEFT,      .mod=5 },
            { "kLFT7", .key=KEY_LEFT,      .mod=6 },
            /* page down */
            { "kNXT",  .key=KEY_PAGE_DOWN, .mod=1 },
            { "kNXT3", .key=KEY_PAGE_DOWN, .mod=2 },
            { "kNXT4", .key=KEY_PAGE_DOWN, .mod=3 },
            { "kNXT5", .key=KEY_PAGE_DOWN, .mod=4 },
            { "kNXT6", .key=KEY_PAGE_DOWN, .mod=5 },
            { "kNXT7", .key=KEY_PAGE_DOWN, .mod=6 },
            /* page up */
            { "kPRV",  .key=KEY_PAGE_UP,   .mod=1 },
            { "kPRV3", .key=KEY_PAGE_UP,   .mod=2 },
            { "kPRV4", .key=KEY_PAGE_UP,   .mod=3 },
            { "kPRV5", .key=KEY_PAGE_UP,   .mod=4 },
            { "kPRV6", .key=KEY_PAGE_UP,   .mod=5 },
            { "kPRV7", .key=KEY_PAGE_UP,   .mod=6 },
            /* arrow-right */
            { "kRIT",  .key=KEY_RIGHT,     .mod=1 },
            { "kRIT3", .key=KEY_RIGHT,     .mod=2 },
            { "kRIT4", .key=KEY_RIGHT,     .mod=3 },
            { "kRIT5", .key=KEY_RIGHT,     .mod=4 },
            { "kRIT6", .key=KEY_RIGHT,     .mod=5 },
            { "kRIT7", .key=KEY_RIGHT,     .mod=6 },
            /* arrow-up */
            { "kUP",   .key=KEY_UP,        .mod=1 },
            { "kUP3",  .key=KEY_UP,        .mod=2 },
            { "kUP4",  .key=KEY_UP,        .mod=3 },
            { "kUP5",  .key=KEY_UP,        .mod=4 },
            { "kUP6",  .key=KEY_UP,        .mod=5 },
            { "kUP7",  .key=KEY_UP,        .mod=6 },
        };

        size_t string_table_offset = -1;
        {
            size_t i = ext_offset;
            i += sizeof(int16_t) * 5;
            i += sizeof(uint8_t) * ext_header.booleans_count;
            i  = basics_align(i, sizeof(int16_t));
            i += number_data_type * ext_header.numbers_count;
            i += sizeof(int16_t) * ext_header.string_table_entries;
            string_table_offset = i;
        }
        assert(string_table_offset != (size_t) -1);

        /*
         * Unfortunately--while far from being the worst terminfo has to
         * offer--the extended index table cuts off before indexing capability 
         * names, so we either have to
         *
         * (1) scan backwards from the end of the file, reading in the names, and
         *     scan backwards in the index table to index into the strings table. OR
         *
         * (2) skip ext_header.strings_count worth of null terminated strings to
         *     start at the names table.
         *
         * Neither of those options should be required, but (2) is the most 
         * simple, hence what is implemented here.
         */
        size_t name_table_offset = -1;
        {
            size_t i = string_table_offset;
            size_t const strings_to_skip = ext_header.strings_count
                                         + ext_header.booleans_count 
                                         + ext_header.numbers_count;
            for (size_t j = 0; j < strings_to_skip; ++j) {
                i += basics_memory_skip_string(content.base, content.size, i);
            }
            name_table_offset = i;
        }
        assert(name_table_offset != (size_t) -1);

        printf("EXTENDED STRINGS\n");
        size_t string_iterator = string_table_offset;
        size_t names_iterator = name_table_offset;
        for (int i = 0; i < ext_header.strings_count; ++i) {
            char const *string = NULL;
            size_t string_length = 0;
            {
                string = 
                    basics_memory_get_address(content.base, string_iterator);
                size_t const n = 
                    basics_memory_skip_string(content.base, content.size, 
                                              string_iterator);
                if (n == 0) {
                    fprintf(stderr, "Extended string table ends before exhaustion!");
                    exit_code = EXIT_BAD;
                    goto done;
                }
                string_length = n-1;
                string_iterator += n;
            }

            char const *name = NULL;
            // size_t name_length = 0;
            {
                name = 
                    basics_memory_get_address(content.base, names_iterator);
                size_t const n = 
                    basics_memory_skip_string(content.base, content.size, 
                                              names_iterator);
                if (n == 0) {
                    break;
                }
                // name_length = n-1;
                names_iterator += n;
            }

            char armored[128];
            basics_memory_armor_string(armored, sizeof(armored), 0,
                                       string, string_length);
            printf("%d: %s %s\n", i+1, name, armored);

            for (int j = 0; j < (int) basics_length_of(name_keymod_map); ++j) {
                struct name_keymod_map const *map = &name_keymod_map[j];
                /* 
                 * NOTE map->name is guaranteed null terminated and name is
                 * verified to be null terminated.
                 */
                if (strcmp(map->name, name) != 0) {
                    continue;
                }
                enum info_status status = 
                    info_base_add_input(&base, string, string_length, 
                                        map->key, map->mod);
                if (status != INFO_STATUS_OK) {
                    fprintf(stderr, "Failed to add extended string to info base!\n");
                }
            }
        }
    }

epilogue:
    printf("INPUT STRING TABLE\n");
    dump_table(&base.string_table);

    printf("INPUT KEY TABLE\n");
    for (int i = 0; i < (int) basics_length_of(base.key_table); ++i) {
        if (base.key_table[i] != (uint8_t) -1) {
            printf("%d %u\n", i, base.key_table[i]);
        }
    }

done:
    info_base_decommission(&base);
    if (content.base) {
        free(content.base);
    }
    return exit_code;
}

/* vi:set sw=4 sts=4 ts=4 et fdm=marker: */
