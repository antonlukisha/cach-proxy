#include "cache.h"

cache *cache_init() {
    cache *cache_ptr = (cache *)malloc(sizeof(cache)); // Выделяем память
    if (!cache_ptr ) {
        logger(ERROR, "Failed to initialize cache");
        return NULL;
    }
    // Выделение памяти под массив записей
    cache_ptr->entries = (cache_entry *)calloc(CACHE_SIZE, sizeof(cache_entry));
    if (!cache_ptr->entries) {
        logger(ERROR, "Failed to allocate cache entries");
        free(cache_ptr);
        return NULL;
    }

    cache_ptr->head = cache_ptr->tail = NULL; // Инициализируем указатели
    pthread_mutex_init(&cache_ptr->lock, NULL); // Инициализация мьютекса
    logger(INFO, "Cache initialized");
    return cache_ptr;
}

void cache_destroy(cache *cache_ptr) {
    pthread_mutex_lock(&cache_ptr->lock); // Залочить мьютекс
    for (int i = 0; i < CACHE_SIZE; i++) {
        free(cache_ptr->entries[i].data); // Освобождение памяти, выделенной под данные
    }
    free(cache_ptr->entries); // Освобождение массива записей
    pthread_mutex_unlock(&cache_ptr->lock); // Разлочить мьютекс
    pthread_mutex_destroy(&cache_ptr->lock); // Дестрой мютекса
    free(cache_ptr); // Очистка кэша
    logger(INFO, "Cache was destroyed");
}

cache_entry *cache_find(cache *cache_ptr, const char *url) {
    pthread_mutex_lock(&cache_ptr->lock); // Залочить мьютекс
    for (cache_entry *entry = cache_ptr->head; entry; entry = entry->next) {
        if (strcmp(entry->url, url) == 0 && entry->expiry > time(NULL)) {
            logger(DEBUG, "Cache hit: URL=%s found", url);

            // Перемещаем найденную запись в начало списка для LRU обновления
            if (entry != cache_ptr->head) {
                // Перемещаем запись в начало списка
                if (entry->prev) entry->prev->next = entry->next;
                if (entry->next) entry->next->prev = entry->prev;
                if (entry == cache_ptr->tail) cache_ptr->tail = entry->prev;

                entry->prev = NULL;
                entry->next = cache_ptr->head;
                if (cache_ptr->head) cache_ptr->head->prev = entry;
                cache_ptr->head = entry;
            }

            pthread_mutex_unlock(&cache_ptr->lock); // Разлочить мьютекс
            return entry; // Возвращаем найденную запись
        }
    }
    logger(DEBUG, "Cache miss: URL=%s not found", url);
    pthread_mutex_unlock(&cache_ptr->lock); // Разлочить мьютекс
    return NULL; // Не найдено
}

void cache_add(cache *cache_ptr, const char *url, const char *data, size_t size, time_t expiry) {
    pthread_mutex_lock(&cache_ptr->lock); // Залочить мьютекс
    logger(INFO, "Adding to cache: URL=%s, SIZE=%zu", url, size);
    cache_remove_expired(cache_ptr); // Удаляем устаревшие записи
    cache_entry *entry = NULL;
    // Ищем свободное место
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (!cache_ptr->entries[i].data) { // Если найдена пустая запись
            entry = &cache_ptr->entries[i];
            break;
        }
    }

    // Если свободного места нет, используем LRU хвост
    if (!entry) {
        entry = cache_ptr->tail;
        if (entry->data) free(entry->data); // Если данные в элементе есть, освобождаем память
    }
    // Заполняем новую запись
    strncpy(entry->url, url, URL_SIZE - 1); // Копируем URL
    entry->url[URL_SIZE - 1] = '\0'; // Вставляем нулевой сивол для предотвращения попадания мусора
    // Копируем данные
    entry->data = malloc(size); // Выделяем память под данные
    if (!entry->data) {
        logger(ERROR, "Failed to allocate memory for cache data");
        pthread_mutex_unlock(&cache_ptr->lock); // Разлочиваем мьютекс
        return;
    }
    memcpy(entry->data, data, size); // Копируем данные в запись
    entry->size = size; // Устанавливаем размер данных
    entry->expiry = expiry; // Устанавливаем время истечения записи

    // Перемещаем запись в начало списка
    if (entry != cache_ptr->head) {
        if (entry->prev) entry->prev->next = entry->next;
        if (entry->next) entry->next->prev = entry->prev;
        if (entry == cache_ptr->tail) cache_ptr->tail = entry->prev;

        entry->prev = NULL;
        entry->next = cache_ptr->head;
        if (cache_ptr->head) cache_ptr->head->prev = entry;
        cache_ptr->head = entry;
    }

    pthread_mutex_unlock(&cache_ptr->lock); // Разлочить мьютекс
    logger(INFO, "Cache entry added: URL=%s", url);
}

void cache_remove_expired(cache *cache_ptr) {
    time_t now = time(NULL); // Получение текущего времени
    cache_entry *entry = cache_ptr->head;
    logger(INFO, "Removing expired cache entries...");
    while (entry) {
        cache_entry *next = entry->next;
        if (entry->expiry <= now) { // Если запись устарела
            logger(DEBUG, "Removing expired entry: URL=%s, Expiry=%ld", entry->url, entry->expiry);
            // Удаляем устаревший элемент
            if (entry->prev) entry->prev->next = entry->next;
            if (entry->next) entry->next->prev = entry->prev;
            if (entry == cache_ptr->head) cache_ptr->head = entry->next;
            if (entry == cache_ptr->tail) cache_ptr->tail = entry->prev;

            free(entry->data); // Освобождаем память
            entry->data = NULL;
        }
        entry = next; // Переход к следующей записи
    }
    logger(INFO, "Expired entries removed");
}

void cache_print(cache *cache_ptr) {
    pthread_mutex_lock(&cache_ptr->lock); // Залочить мьютекс
    logger(RESET, "#########CACHE CONTENT#########\n");
    // Вывод данных
    for (cache_entry *entry = cache_ptr->head; entry; entry = entry->next) {
        logger(DEBUG, "URL=%s, SIZE=%zu, EXPIRY=%ld\n", entry->url, entry->size, entry->expiry);
    }
    logger(RESET, "###############################\n");
    pthread_mutex_unlock(&cache_ptr->lock); // Разлочить мьютекс
}
