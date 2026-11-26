#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "cbin.h"

typedef struct cbin_state {
    const char* name;
    FILE *fp;
    int errored;
    int finished;
    int idx;
    int sector;
    uint8_t checksum_sum;
    uint8_t buffer[SECTOR_SIZE];
    struct cbin_record record;
} cbin_state;

unsigned cbin_finished(cbin_state* cbin) {
    return !cbin->errored && cbin->finished;
}

unsigned cbin_errored(cbin_state* cbin) {
    return cbin->errored;
}

// Print common error message and mark cbin as errored
void cbin_error(cbin_state* cbin) {
    size_t file_offset = (size_t)cbin->sector * SECTOR_SIZE + cbin->idx;
    fprintf(stderr, " at sector %i + %x (file offset: 0x%04zx)\n",
        cbin->sector, cbin->idx, file_offset);
    fprintf(stderr, "\n%s: centurion binary format load failed\n",
        cbin->name);

    cbin->errored = 1;
}

static uint8_t cbin_read8(cbin_state* cbin) {
    if (cbin->idx+1 > SECTOR_SIZE) {
        fprintf(stderr, "sector overrun");
        cbin_error(cbin);
        return 0;
    }
    uint8_t byte = cbin->buffer[cbin->idx++];
    cbin->checksum_sum += byte;

    return byte;
}

static uint16_t cbin_read16(cbin_state* cbin) {
    uint16_t high = cbin_read8(cbin);
    return (high << 8) | cbin_read8(cbin);
}

void cbin_next_sector(cbin_state* cbin) {
    if (cbin->fp == NULL)
        return;

    cbin->sector++;
    cbin->idx = 0;
    size_t read_len = fread(cbin->buffer, 1, SECTOR_SIZE, cbin->fp);
    if (read_len != SECTOR_SIZE) {
        if (ferror(cbin->fp)) {
            perror("fread");
        } else if (read_len == 0) {
            fprintf(stderr, "unexpected end of file");
        } else {
            fprintf(stderr, "sector too small, only got %zu bytes", read_len);
        }
        cbin_error(cbin);
    }
}

struct cbin_state* cbin_open(const char *name) {
    // create state object
    cbin_state* state = calloc(1, sizeof(cbin_state));
    if (!state) {
        perror("calloc");
        exit(1);
    }

    state->name = name;
    // Open file
    state->fp = fopen(name, "rb");
	if (state->fp == NULL) {
		perror("fopen");
		cbin_error(state);
	}

    // Setup initial state
    state->sector = -1; // nextsector will increment
    cbin_next_sector(state);

    return state;
}

void cbin_free(cbin_state* cbin) {
    assert(cbin != NULL);
    if (cbin->fp) {
        fclose(cbin->fp);
    }
    free(cbin);
}

struct cbin_record* cbin_next_record(cbin_state* cbin) {
    while (!cbin->errored && !cbin->finished) {
        cbin->checksum_sum = 0;

        cbin->record.type = cbin_read8(cbin);

        if (cbin->record.type == CBIN_END_FILE) {
            cbin->finished = 1;
            return NULL;
        }

        if (cbin->record.type == CBIN_END_SECTOR) {
            cbin_next_sector(cbin);
            continue;
        }

        uint8_t len = cbin_read8(cbin);
        cbin->record.len = len;
        cbin->record.addr = cbin_read16(cbin);

        if((cbin->idx + len + 1) > SECTOR_SIZE) {
            fprintf(stderr, "record too big, %i bytes", len);
            cbin_error(cbin);
            return NULL;
        }

        // copy data;
        for (int i = 0; i < len; i++) {
            // reading calculates checksum
            cbin->record.data[i] = cbin_read8(cbin);
        }

        uint8_t checksum = -cbin->checksum_sum; // Negated
        uint8_t expected = cbin_read8(cbin);

        if (expected != checksum) {
            fprintf(stderr, "checksum error. Got %02x, Expected %02x",
                expected, checksum);
            cbin_error(cbin);
        }

        if (cbin->errored)
            return NULL;

        return &cbin->record;
    }
    return NULL;
}
