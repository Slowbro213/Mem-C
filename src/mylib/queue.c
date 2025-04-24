#include "../../lib/queue.h"
#include <threads.h>

void queue_init(Queue *q) {
    for (int i = 0; i < MAX_PIPES; i++) {
        q->pipes[i].head = 0;
        q->pipes[i].tail = 0;
        atomic_store(&q->pipes[i].count, 0);
    }
    atomic_store(&q->active_mask, 0);
    atomic_store(&q->valid_mask, 0);
    atomic_store(&q->global_count, 0);
    atomic_store(&q->next_pipe_id, 0);
    q->last_dequeued_pipe = 0;
}

int subscribe(Queue *q) {
    int id = atomic_fetch_add(&q->next_pipe_id, 1) % MAX_PIPES;
    uint64_t mask = 1ULL << id;
    atomic_fetch_or(&q->active_mask, mask);
    return id;
}

bool enqueue(Pipe *p, int pipe_id, int item, Queue *q) {
    size_t current_count = atomic_load(&p->count);
    if (current_count >= QUEUE_SIZE) return false;

    p->numbers[p->tail] = item;
    p->tail = (p->tail + 1) % QUEUE_SIZE;
    
    if (current_count == 0) {
        atomic_fetch_or_explicit(&q->valid_mask, 1ULL << pipe_id, memory_order_release);
    }
    
    atomic_fetch_add(&p->count, 1);
    atomic_fetch_add(&q->global_count, 1);
    return true;
}

bool dequeue(Queue *q, int *item) {
    if (atomic_load(&q->global_count) == 0) return false;

    int start = (q->last_dequeued_pipe + 1) % MAX_PIPES;
    for (int i = 0; i < MAX_PIPES; i++) {
        int id = (start + i) % MAX_PIPES;
        if (atomic_load_explicit(&q->valid_mask, memory_order_acquire) & (1ULL << id)) {
            Pipe *p = &q->pipes[id];
            size_t count = atomic_load(&p->count);
            if (count > 0) {
                *item = p->numbers[p->head];
                p->head = (p->head + 1) % QUEUE_SIZE;
                
                if (count == 1 && !(atomic_load(&q->active_mask) & (1ULL << id))) {
                    atomic_fetch_and_explicit(&q->valid_mask, ~(1ULL << id), memory_order_release);
                }
                
                atomic_fetch_sub(&p->count, 1);
                atomic_fetch_sub(&q->global_count, 1);
                q->last_dequeued_pipe = id;
                return true;
            }
        }
    }
    return false;
}

int dequeue_batch(Queue *q, int *items, size_t max_items) {
  if (atomic_load(&q->global_count) == 0) return false;

  int start = (q->last_dequeued_pipe + 1) % MAX_PIPES;
  for (int i = 0; i < MAX_PIPES; i++) {
    int id = (start + i) % MAX_PIPES;
    if (atomic_load_explicit(&q->valid_mask, memory_order_acquire) & (1ULL << id)) {
      Pipe *p = &q->pipes[id];
      size_t count = atomic_load(&p->count);
      //Dequeue items in batches 
      size_t items_to_dequeue = count < max_items ? count : max_items;
      for (size_t j = 0; j < items_to_dequeue; j++) {
        items[j] = p->numbers[p->head];
        p->head = (p->head + 1) % QUEUE_SIZE;
      }
      if (count == items_to_dequeue && !(atomic_load(&q->active_mask) & (1ULL << id))) {
        atomic_fetch_and_explicit(&q->valid_mask, ~(1ULL << id), memory_order_release);
      }
      atomic_fetch_sub(&p->count, items_to_dequeue);
      atomic_fetch_sub(&q->global_count, items_to_dequeue);
      q->last_dequeued_pipe = id;
      return items_to_dequeue;


    }
  }
  return false;

}

void unsubscribe(Queue *q, int pipe_id) {
    atomic_fetch_and(&q->active_mask, ~(1ULL << pipe_id));
    
    if (atomic_load(&q->pipes[pipe_id].count) == 0) {
        atomic_fetch_and(&q->valid_mask, ~(1ULL << pipe_id));
    }
}

