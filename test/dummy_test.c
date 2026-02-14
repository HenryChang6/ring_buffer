#include "ring_buffer.h"

int main(void) {
    ring_buffer_t rb;
    rb_init(&rb);
    instruction_t item = {.bytes = {1, 2, 3, 4, 5, 6, 7, 8}};
    rb_write(&rb, &item);
    instruction_t out;
    rb_consume(&rb, &out);
    printf("Consumed item: %d\n", out.bytes[0]);
    printf("Consumed item: %d\n", out.bytes[1]);
    printf("Consumed item: %d\n", out.bytes[2]);
    printf("Consumed item: %d\n", out.bytes[3]);
    printf("Consumed item: %d\n", out.bytes[4]);
    printf("Consumed item: %d\n", out.bytes[5]);
    printf("Consumed item: %d\n", out.bytes[6]);
    printf("Consumed item: %d\n", out.bytes[7]);
    return 0;
}