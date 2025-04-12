#ifndef QUEUE_H
#define QUEUE_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define QUEUE_SIZE (1 << 12)
#define MAX_PIPES 8 

typedef struct {
    int numbers[QUEUE_SIZE];
    size_t head;           
    size_t tail;          
    _Atomic size_t count;  
} Pipe;

typedef struct {
    Pipe pipes[MAX_PIPES];
    _Atomic uint64_t active_mask;   
    _Atomic uint64_t valid_mask;    
    _Atomic int next_pipe_id;
    _Atomic size_t global_count;
    int last_dequeued_pipe;         
} Queue;

void queue_init(Queue *q);
int subscribe(Queue *q);
void unsubscribe(Queue *q, int pipe_id);
bool enqueue(Pipe *p, int pipe_id, int item, Queue *parent_q);
bool dequeue(Queue *q, int *item);
int dequeue_batch(Queue *q, int *items, int max_items);  
void print_queue(Queue *q);

#endif
