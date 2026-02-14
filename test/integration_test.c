#include "ring_buffer.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
    ring_buffer_t rb;
    _Atomic int stop;
    _Atomic uint64_t produced;
    _Atomic uint64_t consumed;
    _Atomic uint64_t dropped;
    _Atomic uint64_t errors;
} test_ctx_t;

void* producer_worker (void* arg) {
    test_ctx_t* ctx = (test_ctx_t*)arg;
    uint64_t seq = 0;
    while (atomic_load_explicit(&ctx->stop, memory_order_acquire) != 1) {
        instruction_t item = {0};
        // Just a mock up logic to generate a unique instruction
        uint64_t base = seq * 8u;
        for (int i = 0; i < 8; i++) {
            item.bytes[i] = (uint8_t)(base + (uint64_t)i);
        }
        if(rb_write(&ctx->rb, &item)) {
            seq++;
            atomic_fetch_add_explicit(&ctx->produced, 1u, memory_order_relaxed);
        } else {
            atomic_fetch_add_explicit(&ctx->dropped, 1u, memory_order_relaxed);
            usleep(50);
        }
    } 
    return NULL;
}

void* consumer_worker (void* arg) {
    test_ctx_t* ctx = (test_ctx_t*)arg;
    uint64_t seq = 0;
    while(!atomic_load_explicit(&ctx->stop, memory_order_acquire) || atomic_load_explicit(&ctx->rb.head, memory_order_acquire) != atomic_load_explicit(&ctx->rb.tail, memory_order_acquire)) {
        instruction_t out;
        if(rb_consume(&ctx->rb, &out)) {
            uint64_t base = seq * 8u;
            int valid = 1;
            for (int i = 0; i < 8; i++) {
                uint8_t expected = (uint8_t)(base + (uint64_t)i);
                if (out.bytes[i] != expected) {
                    atomic_fetch_add_explicit(&ctx->errors, 1u, memory_order_relaxed);
                    printf("Error: expected %u, got %u\n", (unsigned)expected, (unsigned)out.bytes[i]);
                    valid = 0;
                    break;
                }
            }
            if (valid) {
                seq++;
            }
            atomic_fetch_add_explicit(&ctx->consumed, 1u, memory_order_relaxed);
        } else {
            if (atomic_load_explicit(&ctx->stop, memory_order_acquire) == 1 && atomic_load_explicit(&ctx->rb.head, memory_order_acquire) == atomic_load_explicit(&ctx->rb.tail, memory_order_acquire)) 
                break;
            usleep(50);
        }
    }
    return NULL;
}

int main(void) {
    // initialize test context
    test_ctx_t ctx = {0};
    rb_init(&ctx.rb);
    atomic_init(&ctx.stop, 0);
    atomic_init(&ctx.produced, 0);
    atomic_init(&ctx.consumed, 0);
    atomic_init(&ctx.dropped, 0);
    atomic_init(&ctx.errors, 0);
    int test_time = 2; // seconds
    // initialize threads
    pthread_t producer, consumer;
    pthread_create(&producer, NULL, producer_worker, &ctx);
    pthread_create(&consumer, NULL, consumer_worker, &ctx);
    
    sleep(test_time);

    // stop threads and summarize the test
    atomic_store_explicit(&ctx.stop, 1, memory_order_release);
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    uint64_t produced = atomic_load_explicit(&ctx.produced, memory_order_relaxed);
    uint64_t consumed = atomic_load_explicit(&ctx.consumed, memory_order_relaxed);
    uint64_t dropped = atomic_load_explicit(&ctx.dropped, memory_order_relaxed);
    uint64_t errors = atomic_load_explicit(&ctx.errors, memory_order_relaxed);
    printf("Test summary for duration %d seconds:\n", test_time);
    printf("Produced: %llu instructions\n", (unsigned long long)produced);
    printf("Consumed: %llu instructions\n", (unsigned long long)consumed);
    printf("Dropped: %llu instructions\n", (unsigned long long)dropped);
    printf("Errors: %llu instructions\n", (unsigned long long)errors);
    printf("Test result: %s\n", (errors == 0 && consumed > 0 && produced == consumed) ? "PASSED" : "FAILED");
    return 0;
}