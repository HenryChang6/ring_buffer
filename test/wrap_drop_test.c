#include "ring_buffer.h"
#include <stdio.h>

int main(void) {
    ring_buffer_t rb;
    rb_init(&rb);

    instruction_t item = {0};
    instruction_t out = {0};

    /*
     * Phase A: create head/tail movement so indices wrap.
     * Repeat write+consume for 2 full rounds.
     */
    uint32_t rounds = (uint32_t)RING_SLOTS * 2u;
    for (uint32_t seq = 0; seq < rounds; seq++) {
        item.bytes[0] = (uint8_t)(seq & 0xFFu);
        if (rb_write(&rb, &item) != 1) {
            printf("FAIL: write failed during wrap preparation at seq=%u\n", (unsigned)seq);
            return 1;
        }
        if (rb_consume(&rb, &out) != 1) {
            printf("FAIL: consume failed during wrap preparation at seq=%u\n", (unsigned)seq);
            return 1;
        }
        if (out.bytes[0] != item.bytes[0]) {
            printf(
                "FAIL: wrap data mismatch at seq=%u expected=%u got=%u\n",
                (unsigned)seq,
                (unsigned)item.bytes[0],
                (unsigned)out.bytes[0]
            );
            return 1;
        }
    }

    /*
     * Phase B: fill ring to capacity-1 with a known sequence.
     * Then verify one extra write is dropped and unread data remains intact.
     */
    for (uint32_t i = 0; i < (uint32_t)RING_SLOTS - 1u; i++) {
        item.bytes[0] = (uint8_t)((100u + i) & 0xFFu);
        if (rb_write(&rb, &item) != 1) {
            printf("FAIL: write failed during full-buffer setup at i=%u\n", (unsigned)i);
            return 1;
        }
    }

    item.bytes[0] = 0xEEu;
    if (rb_write(&rb, &item) != 0) {
        printf("FAIL: extra write should be dropped on full buffer\n");
        return 1;
    }

    for (uint32_t i = 0; i < (uint32_t)RING_SLOTS - 1u; i++) {
        uint8_t expected = (uint8_t)((100u + i) & 0xFFu);
        if (rb_consume(&rb, &out) != 1) {
            printf("FAIL: consume failed during post-drop verification at i=%u\n", (unsigned)i);
            return 1;
        }
        if (out.bytes[0] != expected) {
            printf(
                "FAIL: drop policy corrupted unread data at i=%u expected=%u got=%u\n",
                (unsigned)i,
                (unsigned)expected,
                (unsigned)out.bytes[0]
            );
            return 1;
        }
    }

    printf("PASS: wrap and drop policy test\n");
    return 0;
}
