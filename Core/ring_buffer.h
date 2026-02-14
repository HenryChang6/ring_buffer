#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*
 * Project configuration:
 * 16 KB total ring size, 64-byte instruction unit -> 256 slots.
 */
#define RING_BUFFER_BYTES 16384u
#define INSTRUCTION_BYTES 64u
#define RING_SLOTS (RING_BUFFER_BYTES / INSTRUCTION_BYTES)
#define RING_MASK (RING_SLOTS - 1u)

#if (RING_SLOTS == 0u)
#error "RING_SLOTS must be greater than zero."
#endif

#if (RING_SLOTS & RING_MASK)
#error "RING_SLOTS must be a power of two."
#endif

/*
 * One instruction payload (64 bytes).
 * Replace this with real instruction fields later.
 */
typedef struct {
    uint8_t bytes[INSTRUCTION_BYTES];
} instruction_t;

/*
 * SPSC ring buffer:
 * - producer writes head
 * - consumer writes tail
 */
typedef struct {
    _Atomic uint32_t head;
    _Atomic uint32_t tail;
    instruction_t slots[RING_SLOTS];
} ring_buffer_t;

/*
 * API:
 * - rb_init() initializes indices.
 * - rb_write() returns 1 on success, 0 when full (drop-latest policy).
 * - rb_consume() returns 1 on success, 0 when empty.
 */
void rb_init(ring_buffer_t *rb);
int rb_write(ring_buffer_t *rb, const instruction_t *item);
int rb_consume(ring_buffer_t *rb, instruction_t *out);

#endif /* RING_BUFFER_H */
