#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

typedef struct {
    int data;
    atomic_int flag;
} thread_arg_t;

void* producer(void* arg) {
    thread_arg_t* targ = (thread_arg_t*)arg;
    targ->data = 777;
    /*
     atomic write to flag which adds on ordering constraint that all normal
     write (like data = 777) must become visible before another thread 
     successfully loads the flag acquires this flag
    */
    atomic_store_explicit(&targ->flag, 1, memory_order_release);
    return NULL;
}

void* consumer(void* arg) {
    thread_arg_t* targ = (thread_arg_t*)arg;
    /*
     After that acquire load, subsequent reads in consumer (like targ->data) are
     guaranteed to see writes that happened-before the release store.
    */
    while(atomic_load_explicit(&targ->flag, memory_order_acquire) != 1) {
        usleep(1000);
        printf("waiting for flag...\n");
    }
    printf("successfully received flag\n");
    printf("data: %d\n", targ->data);
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    thread_arg_t arg = {.data = 666, .flag = 0};
    printf("Initial data: %d\n", arg.data);
    pthread_create(&t1, NULL, producer, &arg);
    pthread_create(&t2, NULL, consumer, &arg);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Final data: %d\n", arg.data);
    return 0;
}