#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "logging.h"
#include "proxy.h"

#define THREAD_POOL_SIZE 8
#define QUEUE_INITIAL_CAPACITY 16

typedef struct thread_pool {
    pthread_t threads[THREAD_POOL_SIZE]; // Массив идендификаторов потоков
    pthread_mutex_t lock; // Мьютекс для синхронизации доступа к очереди
    pthread_cond_t cond; // Условная переменная для уведомления потоков
    int* queue; // Очередь из клиенских сокетов
    size_t size; // Текущий размер очереди
    int front, rear; // Указатели на начало и конец очереди
    int stop; // Флаг для завершения работы пула
    size_t capacity; // Вместимость очереди
} thread_pool;

void init_thread_pool();
void enqueue(int);
void resize_queue();
int dequeue();
void *thread_function(void *);
void start_thread_pool();
void stop_thread_pool();

#endif
