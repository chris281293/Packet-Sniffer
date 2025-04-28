#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_MSG_SIZE 1024
// #define MAX_BUF_SIZE 1048576
#define MAX_BUF_SIZE 1024
/*
    實作一個thread safe的RingBuffer, 分成OVERLAPPED和NO_OVERLAPPED兩種模式:
        OVERLAPPED: buffer滿了的換會從最早的元素開始覆蓋.
        - function: rb_push_overlapped: push元素進入buffer, 如果buffer滿了會覆蓋最早的元素.
        - function: rb_getNth_overlapped: 取得第n個元素, 如果n大於buffer的大小會回傳-1.
        - function: rb_clear_overlapped: 清空buffer
        NO_OVERLAPPED: buffer滿了的話, 會利用semaphore block住等到有位子才能進來, 可以用來解決producer-consumer問題.
        - function: rb_push: push元素進入buffer, 如果buffer滿了會block住等到有位子才能進來.
        - function: rb_pop: pop元素出來, 如果buffer空了會block住等到有元素才能進來.
        - function: rb_front: 取得buffer最前面的元素, 如果buffer空了會block住等到有元素才能進來.
        - function: rb_clear: 清空buffer
    -function rb_init: 初始化buffer, 需給定buffer大小以及類型.
*/
typedef enum{
    OVERLAPPED = 0,
    NO_OVERLAPPED
}rb_type;

typedef struct{
    char buffer[MAX_BUF_SIZE][MAX_MSG_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    sem_t full;
    sem_t empty;
} RingBuffer;

void rb_init(RingBuffer *rb, rb_type type);
// for NON_OVERLAPPED ring buffer
void rb_push(RingBuffer *rb, const char *msg);
void rb_front(RingBuffer *rb, char *msg);
void rb_pop(RingBuffer *rb, char *msg);
void rb_clear(RingBuffer *rb);

// for OVERLAPPED ring buffer
void rb_push_overlapped(RingBuffer *rb, const char *msg);
int rb_getNth_overlapped(RingBuffer *rb, char *msg, int n);

#endif