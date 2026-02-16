/* Thread-free boundary test for SPSC ring buffer */
#include "ring_buffer.h"
#include <stdio.h>

int main(void) {
    ring_buffer_t rb;
    rb_init(&rb);

    instruction_t out = {0};
    instruction_t item = {0};

    printf("Boundary test started\n");

    /* Test1: consume should fail when buffer is empty */
    if (rb_consume(&rb, &out) != 0) {
        printf("FAIL: consume succeeds on empty buffer\n");
        return 1;
    }

    /* Test2: fill exactly RING_SLOTS - 1 entries */
    for (uint32_t i = 0; i < (uint32_t)RING_SLOTS - 1u; i++) {
        for (uint32_t j = 0; j < 8u; j++) {
            item.bytes[j] = (uint8_t)(i + j);
        }
        if (rb_write(&rb, &item) != 1) {
            printf("FAIL: write failed before reaching capacity at i=%u\n", (unsigned)i);
            return 1;
        }
    }

    /* Test3: one extra write must fail (drop-latest) */
    if (rb_write(&rb, &item) != 0) {
        printf("FAIL: write succeeds when buffer is full\n");
        return 1;
    }

    /* Test4: drain and verify FIFO order */
    for (uint32_t i = 0; i < (uint32_t)RING_SLOTS - 1u; i++) {
        if (rb_consume(&rb, &out) != 1) {
            printf("FAIL: consume failed while draining at i=%u\n", (unsigned)i);
            return 1;
        }
        for (uint32_t j = 0; j < 8u; j++) {
            uint8_t expected = (uint8_t)(i + j);
            if (out.bytes[j] != expected) {
                printf(
                    "FAIL: data mismatch at i=%u j=%u expected=%u got=%u\n",
                    (unsigned)i,
                    (unsigned)j,
                    (unsigned)expected,
                    (unsigned)out.bytes[j]
                );
                return 1;
            }
        }
    }

    /* Test5: consume should fail again after fully drained */
    if (rb_consume(&rb, &out) != 0) {
        printf("FAIL: consume succeeds after drain complete\n");
        return 1;
    }

    printf("PASS: boundary test\n");
    return 0;
}