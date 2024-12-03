#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "logging.h"

#define CACHE_SIZE 100
#define URL_SIZE 256

typedef struct cache_entry {
    char url[URL_SIZE]; // URL ключ связанный с данными
    char *data; // Указатель на данные
    size_t size; // Размер данных
    time_t expiry; // Тайм-аут для записи "время жизни"
    struct cache_entry *prev; // Ссылка на предыдущий элемент
    struct cache_entry *next; // Ссылка на следующий элемент
} cache_entry;

typedef struct {
    cache_entry *head; // Голова списка LRU
    cache_entry *tail; // Хвост списка LRU
    cache_entry *entries; // Массив записей
    pthread_mutex_t lock; // Мьютекс для синхронизации доступа
} cache;

cache *cache_init();
void cache_destroy(cache *cache);
cache_entry *cache_find(cache *cache, const char *url);
void cache_add(cache *cache, const char *url, const char *data, size_t size, time_t expiry);
void cache_remove_expired(cache *cache);
void cache_print(cache *cache);

#endif
