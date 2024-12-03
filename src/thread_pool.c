#include "thread_pool.h"

thread_pool pool;
pthread_t threads[THREAD_POOL_SIZE];

void init_thread_pool() {
    pool.capacity = QUEUE_INITIAL_CAPACITY; // Задаём размер очереди
    pool.queue = (int *)malloc(pool.capacity * sizeof(int)); // Выделяем под очередь память
    if (!pool.queue) {
        logger(ERROR, "Failed to allocate memory for the queue");
        exit(EXIT_FAILURE);
    }
    pool.front = pool.rear = 0; // Очередь пуста
    pool.stop = 0; // Пул активен
    pthread_mutex_init(&pool.lock, NULL); // Инициализация мьютекса
    pthread_cond_init(&pool.cond, NULL); // Инициализация условной переменной
    logger(INFO, "Thread pool initialized");
}

void resize_queue() {
    size_t new_capacity = pool.capacity * 2; // Удваиваем размер очереди
    int *new_queue = (int *)realloc(pool.queue, new_capacity * sizeof(int)); // Выделяем память для нового размера
    if (!new_queue) {
        logger(ERROR, "Failed to resize queue");
        exit(EXIT_FAILURE);
    }
    if (pool.front > pool.rear) {
        for (size_t i = 0; i < pool.rear; i++) {
            new_queue[pool.capacity + i] = pool.queue[i];
        }
        pool.rear += pool.capacity;
    }
    pool.capacity = new_capacity; // Обновляем объем
    pool.queue = new_queue; // Обновляем указатель на очередь
    logger(DEBUG, "Queue resized to %lu", pool.capacity);
}

void enqueue(int client_socket) {
    pthread_mutex_lock(&pool.lock); // Залочить мьютекс
    // В случае достижения лимита увеличиваем размер очереди в два раза
    if (pool.size == pool.capacity) resize_queue();
    pool.queue[pool.rear] = client_socket; // Добавляет сокет в очередь
    pool.rear = (pool.rear + 1) % pool.capacity;
    pool.size++; // Увеличение размера
    pthread_cond_signal(&pool.cond); // Отправка сигнала появления нового сокета в очереди
    pthread_mutex_unlock(&pool.lock); // Разлочить мьютекс
    logger(DEBUG, "Client socket %d added to queue", client_socket);
}

int dequeue() {
    pthread_mutex_lock(&pool.lock); // Залочить мьютекс
    while (!pool.size && !pool.stop) {
        pthread_cond_wait(&pool.cond, &pool.lock); // Если очередь пуста засыпаем в ожидании пока задача появится
    }
    if (pool.stop) { // Если активирован стоп флаг
        pthread_mutex_unlock(&pool.lock); // Разлочить мьютекс
        logger(DEBUG, "Thread pool caught stop flag");
        return -1;
    }
    int client_socket = pool.queue[pool.front]; // Достаём соккет из очереди
    pool.front = (pool.front + 1) % pool.capacity;
    pool.size--; // Уменьшение размера
    pthread_mutex_unlock(&pool.lock); // Разлочить мьютекс
    logger(DEBUG, "Client socket %d dequeued", client_socket);
    return client_socket;
}

void *thread_function(void *arg) {
    logger(INFO, "Thread started to work");
    while (1) {
        int client_socket = dequeue();  // Извлечь сокет из очереди
        if (client_socket == -1) break;  // Завершить поток, если `dequeue` вернул -1
        handle_client(client_socket);    // Выполнить задание
        logger(INFO, "Handled client with socket: %d", client_socket);
    }
    logger(INFO, "Thread exiting");
    return NULL;
}


void start_thread_pool() {
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
      // Создает потоки и запускает функцию thread_function для каждого потока
      if (pthread_create(&threads[i], NULL, thread_function, NULL) != 0) {
          logger(ERROR, "Failed to create thread %d", i);
          exit(EXIT_FAILURE);
      }
    }
    logger(INFO, "All threads started");
}

void stop_thread_pool() {
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_join(threads[i], NULL) != 0) {  // Дождаться завершения каждого потока
            logger(WARNING, "Failed to join thread %d", i);
        }
    }
    pthread_mutex_lock(&pool.lock); // Залочить мьютекс
    pool.stop = 1;                  // Установить флаг завершения
    pthread_cond_broadcast(&pool.cond);  // Разбудить все потоки
    pthread_mutex_unlock(&pool.lock); // Разлочить мьютекс

    pthread_mutex_destroy(&pool.lock); // Дестрой мютекса
    pthread_cond_destroy(&pool.cond); // Дестрой кондишена
    free(pool.queue); // Очистка очереди
    logger(INFO, "Thread pool stopped and all resources freed");
}
