#pragma once

// added tasks to this because this is where the queue code resides
// tasks hold all the info and we are gonna malloc them at the start because i think im having issues with memory / not having the right shit in the calls
// idk im getting invalid reads and writes cant even seem  to do any requests (altho it did a little of the bible which was interesting)

// Changed the BQueue struct to hold task_t
// changed functions to take task_ts and return task_ts
// changed dequeue to return a pointer to the task in question instead of setting a value

#define BUFF_SIZE 4096
// this should be what ever the other one is set to

typedef struct {
    int connfd;
    char *buffer;
} task_t;

task_t *createTask(int cfd) // make a task
{
    task_t *nt = (task_t *) malloc(sizeof(task_t));
    nt->connfd = cfd;
    nt->buffer = (char *) malloc(sizeof(char) * BUFF_SIZE);
    return nt;
}

void freeTask(task_t *t) // clean it up
{
    free(t->buffer);
    free(t);
}

// Thanks Eugene
typedef struct {
    uint size;
    int head;
    int tail;
    task_t *buffer;
    uint capacity;
} BQueue;

BQueue *queue_new(uint cap);
void enqueue(BQueue *B, task_t *item);
task_t *dequeue(BQueue *B);
int queue_full(BQueue *B);
int queue_empty(BQueue *B);

BQueue *queue_new(uint cap) // make a new one
{
    BQueue *B = (BQueue *) malloc(sizeof(BQueue));
    B->size = 0;
    B->head = 0;
    B->tail = 0;
    B->capacity = cap;
    //BQueue B = { .size = 0, .head = 0, .tail = 0, .capacity = cap };

    B->buffer = (task_t *) malloc(cap * sizeof(task_t));
    if (B->buffer == NULL) // no space for desired queue
    {
        return NULL;
    }

    return B;
}

int queue_full(BQueue *B) // return 1 for full queue and 0 for not full
{
    if (B->size == B->capacity) {
        return 1;
    }
    return 0;
}
int queue_empty(BQueue *B) // return 1 for empty queue and 0 for not empty
{
    if (B->size == 0) {
        return 1;
    }
    return 0;
}
// 0 1 2 3 4 5
//
// ^
//

void queue_free(BQueue *b) {
    free(b->buffer); // deallocate the buffer
    free(b); // free ya mind
}

// going to make ensuring we enqueue and dequeue correctly up to
// the producer and consumer with queue_full / queue_empty because i think
// if these are void i could potentially lose requests ona full or empty
void enqueue(BQueue *B, task_t *item) {
    B->buffer[B->tail] = *item; // insert item
    B->tail = (B->tail + 1) % B->capacity; // increment
    B->size = B->size + 1;
}

task_t *dequeue(BQueue *B) {
    task_t *ret_item = &(B->buffer[B->head]); // get the item at the front
    B->head = (B->head + 1) % B->capacity; // increment
    B->size = B->size - 1;
    return ret_item;
}
