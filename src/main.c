#include <threads.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "../lib/queue.h"

#define NUM_PRODUCERS (1 << 3)  
#define ITEMS_PER_PRODUCER (1 << 20) // items per producer
#define TOTAL_ITEMS (NUM_PRODUCERS * ITEMS_PER_PRODUCER)

_Atomic int consumed_count = 0;

typedef struct {
    Queue *q;
    int pipe_id;
} ProducerArgs;

int producer(void *arg) {
    ProducerArgs *args = (ProducerArgs *)arg;
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        int item = i + (args->pipe_id * ITEMS_PER_PRODUCER);
        while(!enqueue(&args->q->pipes[args->pipe_id], args->pipe_id, item, args->q)) {
            thrd_yield();
        }
    }
    return 0;
}

int consumer_single(void *arg) {
    Queue *q = (Queue *)arg;
    while (atomic_load(&consumed_count) < TOTAL_ITEMS) {
        int item;
        if (dequeue(q, &item)) {
            atomic_fetch_add(&consumed_count, 1);
        } else {
            thrd_yield();
        }
    }
    return 0;
}

int consumer_batch(void *arg) {
    Queue *q = (Queue *)arg;
    const size_t BATCH_SIZE = (1 << 10); 
    int batch[BATCH_SIZE];
    
    while (atomic_load(&consumed_count) < TOTAL_ITEMS) {
        int count = dequeue_batch(q, batch, BATCH_SIZE);
        if (count > 0) {
            atomic_fetch_add(&consumed_count, count);
        } else {
            thrd_yield();
        }
    }
    return 0;
}

void run_test(int mode) {
    atomic_store(&consumed_count, 0);
    
    Queue q;
    queue_init(&q);
    thrd_t producers[NUM_PRODUCERS], consumer_thread;
    ProducerArgs args[NUM_PRODUCERS];
    
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        args[i].pipe_id = subscribe(&q);
        args[i].q = &q;
    }
    
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        thrd_create(&producers[i], producer, &args[i]);
    }
    
    if (mode == 0) {
        thrd_create(&consumer_thread, consumer_single, &q);
        printf("Running single-item dequeue test...\n");
    } else {
        thrd_create(&consumer_thread, consumer_batch, &q);
        printf("Running batch dequeue test...\n");
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        thrd_join(producers[i], NULL);
        unsubscribe(&q, args[i].pipe_id);
    }
    thrd_join(consumer_thread, NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long duration_ms = (end.tv_sec - start.tv_sec) * 1000 +
                         (end.tv_nsec - start.tv_nsec) / 1000000;
    
    double throughput = (TOTAL_ITEMS / (duration_ms / 1000.0)) / 1000000.0;
    double avg_latency_ns = ((double)duration_ms * 1e6) / TOTAL_ITEMS;
    
    printf("========================\n");
    if (mode == 0) {
        printf("Test: Single-item Dequeue\n");
    } else {
        printf("Test: Batch Dequeue (batch size = %d)\n", 64);
    }
    printf("Producers: %d\n", NUM_PRODUCERS);
    printf("Items per producer: %d\n", ITEMS_PER_PRODUCER);
    printf("Total items: %d\n", TOTAL_ITEMS);
    printf("Total time: %ld ms\n", duration_ms);
    printf("Throughput: %.2f M ops/sec\n", throughput);
    printf("Average latency: %.2f ns/op\n", avg_latency_ns);
    printf("========================\n\n");
}

int main(void) {
    run_test(0);
    run_test(1);
    
    return 0;
}
