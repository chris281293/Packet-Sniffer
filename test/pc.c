#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#define MAX_BUF_SIZE 8


int buf[MAX_BUF_SIZE], front=0, rear=0;
sem_t full, empty;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void print_buf(){
    printf("buf: ");
    for(int i=front; i!=rear; i=(i+1)%MAX_BUF_SIZE){
        printf("%d ", buf[i]);
    }
    printf("\n");
}

void* consumer(void *arg){
    do{
        sem_wait(&full);
        pthread_mutex_lock(&mutex);
        // read data
        int res = buf[front];
        front = (front+1)%MAX_BUF_SIZE;
        print_buf();

        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
        usleep(rand()%1000+500);
    }while(1);
}

void* producer(void *arg){
    do{    int data = rand();
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);
        // write data
        buf[rear] = data;
        rear = (rear+1)%MAX_BUF_SIZE;
        print_buf();

        pthread_mutex_unlock(&mutex);
        sem_post(&full);
        usleep(rand()%1000+500);
    }while(1);
}

int main(){
    srand(time(NULL));

    sem_init(&full, 0, 0);
    sem_init(&empty, 0, MAX_BUF_SIZE);

    pthread_t producer_thread, consumer_thread;
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);


    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    
    sem_destroy(&full);
    sem_destroy(&empty);
}