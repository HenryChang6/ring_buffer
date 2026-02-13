#include "ring_buffer.h"

void rb_init(ring_buffer_t *rb) {
    atomic_store_explicit(&rb->head, 0u, memory_order_relaxed);
    atomic_store_explicit(&rb->tail, 0u, memory_order_relaxed);
}

int rb_write(ring_buffer_t *rb, const instruction_t *item) {
    uint32_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    uint32_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    uint32_t next_head = (head + 1u) & RING_MASK;
    // Note: head == tail --> buffer is empty
    if (next_head == tail) { 
        printf("Ring buffer is full. The latest instruction is dropped.\n");
        return 0;
    }
    rb->slots[head] = *item;
    atomic_store_explicit(&rb->head, next_head, memory_order_release);
    return 1;
}

int rb_consume(ring_buffer_t *rb, instruction_t *out) {

}
