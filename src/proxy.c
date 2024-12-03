#include "proxy.h"

pthread_mutex_t lock;  // Мьютекс для синхронизации доступа
cache *cache_ptr;

int proxy_init(int port) {
  init_thread_pool(); // Инициализация пула потоков
  // Инициализация кэша
  cache_ptr = cache_init(); // Добавление вызова функции инициализации кэша
  if (cache_ptr == NULL) {
      logger(ERROR, "Cache initialization failed");
      exit(EXIT_FAILURE);
  }
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr; // Структура для хранения адреса сервера

  // Настройка адреса сервера
  server_addr.sin_family = AF_INET;  // Используем IPv4
  server_addr.sin_port = htons(port); // Устанавливаем порт
  server_addr.sin_addr.s_addr = INADDR_ANY; // Привязываем сервер ко всем доступным интерфейсам

  if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { // Биндим сервер
      logger(ERROR, "Server bind failed");
      exit(EXIT_FAILURE);
  }
  logger(INFO, "Proxy server initialized");
  return server_socket;
}

void proxy_start(int port, int server_socket) {
    listen(server_socket, 10); // Слушаем на сокете подключение с бэклогом 10
    logger(INFO, "Proxy server listening on port %d...", port);
    start_thread_pool();  // Запуск пула потоков
    while (1) {
        struct sockaddr_in client_addr; // Структура для хранения адреса клиента
        socklen_t client_len = sizeof(client_addr); // Размер структуры клиента
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            logger(ERROR, "Client accept failed");
            continue;
        }
        logger(INFO, "Client send to queue");
        enqueue(client_socket); // Добавляем клиента в очередь для обработки
    }
    cache_destroy(cache_ptr); // Дестроем кэш
    stop_thread_pool(); // Останавливаем пул потоков
    close(server_socket); // Закрываем сокет
    logger(INFO, "Proxy server closed");
}

char *extract_url(char *buffer) {
    static char url[256];
    char method[8], protocol[16]; // Переменные для хранения метода, URL и протокола
    sscanf(buffer, "%s %s %s", method, url, protocol); // Извлекаем метод, URL и протокол

    if (strcmp(method, "GET") != 0) { // Обрабатываем только GET
        logger(INFO, "Non-GET request received, ignoring...");
        return NULL;
    }
    logger(INFO, "GET request received for URL: %s", url);
    return (void*)url; // Возвращаем URL из запроса
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE]; // Буфер для хранения полученных данных
    memset(buffer, 0, BUFFER_SIZE); // Обнуляем буфер
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0); // Получаем данные от клиента
    if (bytes_received < 0) {
        logger(ERROR, "Error receiving data from client");
        close(client_socket);
        return;
    }
    logger(INFO, "Received %d bytes from client", bytes_received);
    // Проверка в кэше
    char *url = extract_url(buffer);
    if (url == NULL) { // Функция для извлечения URL из запроса
      logger(INFO, "URL extraction failed, closing connection...");
      close(client_socket);
      return;
    }
    cache_entry *found_cache = cache_find(cache_ptr, url);  // Ищем URL в кэше
    if (found_cache != NULL) { // Если URL найден в кэше
        logger(INFO, "Cache hit for URL: %s", url);
        cache_print(cache_ptr);
        pthread_mutex_lock(&lock); // Лочим мьютекс
        send(client_socket, found_cache->data, found_cache->size, 0); // Отправляем данные из кэша клиенту
        pthread_mutex_unlock(&lock); // Освобождаем мьютекс
        logger(INFO, "Sent cached data to client");
        close(client_socket); // Закрываем соединение с клиентом
        return;
    }

    // Запрос к серверу если данных нет в кеше
    char host[128], path[128]; // Переменные для хранения хоста и пути
    sscanf(url, "http://%[^/]%s", host, path); // Извлекаем хост и путь из URL
    logger(INFO, "Resolving host: %s", host);
    struct hostent *server = gethostbyname(host); // Получаем IP-адрес сервера по имени хоста
    if (!server) {
        logger(ERROR, "Failed to resolve host: %s", host);
        close(client_socket); // Закрываем сокет клиента, если сервер не найден
        return; // Завершаем обработку клиента
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;  // Используем IPv4
    server_addr.sin_port = htons(80); // Устанавливаем порт 80
    server_addr.sin_addr = *(struct in_addr *)server->h_addr; // Устанавливаем IP-адрес сервера
    logger(INFO, "Connecting to server %s:%d", host, 80);
    // Подключаемся к серверу
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        logger(ERROR, "Failed to connect to server: %s", host);
        close(client_socket); // Закрываем сокет клиента в случае ошибки
        close(server_socket); // Закрываем сокет сервера в случае ошибки
        return;
    }
    logger(INFO, "Connected to server, sending GET request");
    // Формируем запрос к серверу
    snprintf(buffer, BUFFER_SIZE, "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    send(server_socket, buffer, strlen(buffer), 0); // Отправляем запрос на сервер

    // Получение ответа и кэширование
    char *response = malloc(BUFFER_SIZE); // Выделяем память для хранения ответа
    size_t size = 0; // Общий размер ответа
    time_t expiry = time(NULL) + 3600;
    while ((bytes_received = recv(server_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        response = realloc(response, size + bytes_received); // Увеличиваем память для ответа
        memcpy(response + size, buffer, bytes_received); // Копируем полученные данные в ответ
        size += bytes_received; // Увеличиваем общий размер данных
        send(client_socket, buffer, bytes_received, 0); // Отправляем полученные данные клиенту
    }
    logger(INFO, "Received %d bytes from server, caching response", size);
    // Кэшируем полученные данные
    cache_add(cache_ptr, url, response, size, expiry); // Добавляем ответ в кэш
    free(response); // Освобождаем память выделенную для ответа
    logger(INFO, "Closing connections");
    close(client_socket); // Закрываем сокет клиента в случае ошибки
    close(server_socket); // Закрываем сокет сервера в случае ошибки
}
