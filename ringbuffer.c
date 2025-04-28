#include "ringbuffer.h"
void rb_init(RingBuffer *rb, rb_type type){
    if(type == NO_OVERLAPPED){
        rb->head = rb->tail = rb->count = 0;
        pthread_mutex_init(&rb->mutex, NULL);
        sem_init(&rb->full, 0, 0);
        sem_init(&rb->empty, 0, MAX_BUF_SIZE);
    }else{
        // traditional ring buffer without semaphore
        rb->head = rb->tail = rb->count = 0;
        pthread_mutex_init(&rb->mutex, NULL);
    }
    pthread_mutex_lock(&rb->mutex);
    for(int i=0; i<MAX_BUF_SIZE; i++)
        rb->buffer[i][0] = '\0';
    pthread_mutex_unlock(&rb->mutex);
}
/* For NO_OVERLAPPED ring buffer */
void rb_push(RingBuffer *rb, const char *msg){
    sem_wait(&rb->empty);
    pthread_mutex_lock(&rb->mutex);
    
    // copy msg to buffer
    strncpy(rb->buffer[rb->tail], msg, MAX_MSG_SIZE-1);
    rb->buffer[rb->tail][MAX_MSG_SIZE-1] = '\0';
    rb->tail = (rb->tail+1) % MAX_BUF_SIZE;
    if(rb->count < MAX_BUF_SIZE) rb->count++;

    pthread_mutex_unlock(&rb->mutex);
    sem_post(&rb->full);
}

void rb_front(RingBuffer *rb, char *msg){
    /* read the first element in the buffer */
    pthread_mutex_lock(&rb->mutex);
    strncpy(msg, rb->buffer[rb->head], MAX_MSG_SIZE);
    pthread_mutex_unlock(&rb->mutex);
}
void rb_pop(RingBuffer *rb, char *msg){
    sem_wait(&rb->full);
    pthread_mutex_lock(&rb->mutex);
    
    // get msg from buffer
    strncpy(msg, rb->buffer[rb->head], MAX_MSG_SIZE);
    rb->head = (rb->head+1) % MAX_BUF_SIZE;
    if(rb->count > 0) rb->count--;

    pthread_mutex_unlock(&rb->mutex);
    sem_post(&rb->empty);
}
void rb_clear(RingBuffer *rb){
    while(rb->head != rb->tail)
        rb_pop(rb, NULL);
}

/* For OVERLAPPED ringbuffer */
// for OVERLAPPED ring buffer
void rb_push_overlapped(RingBuffer *rb, const char *msg){
    pthread_mutex_lock(&rb->mutex);
    // push without requiring sepaphore
    strncpy(rb->buffer[rb->tail], msg, MAX_MSG_SIZE-1);
    rb->buffer[rb->tail][MAX_MSG_SIZE-1] = '\0';
    rb->tail = (rb->tail+1) % MAX_BUF_SIZE;
    if(rb->count < MAX_BUF_SIZE) 
        rb->count++;
    else
        rb->head = (rb->head+1) % MAX_BUF_SIZE; // overwrite the oldest message 
    pthread_mutex_unlock(&rb->mutex);
}
int rb_getNth_overlapped(RingBuffer *rb, char *msg, int n){
    /* read the Nth entry from head */
    pthread_mutex_lock(&rb->mutex);
    if(n >= rb->count){
        pthread_mutex_unlock(&rb->mutex);
        return -1; // out of range
    }
    strncpy(msg, rb->buffer[rb->head+n%MAX_BUF_SIZE], MAX_MSG_SIZE);
    pthread_mutex_unlock(&rb->mutex);
}