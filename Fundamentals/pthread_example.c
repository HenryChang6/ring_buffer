#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
    int id;
} thread_arg_t;

void* worker(void* arg) {
    thread_arg_t* targ = (thread_arg_t*)arg;
    for(int i = 0; i < 5; i++) {
        printf("Thread %d: loop %d\n", targ->id, i);
        usleep(1000);
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    thread_arg_t arg1 = {.id = 1};
    thread_arg_t arg2 = {.id = 2};
    pthread_create(&t1, NULL, worker, &arg1);
    pthread_create(&t2, NULL, worker, &arg2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}