#ifndef LOGGING_H
#define LOGGING_H

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

// Цветовые коды ANSI
#define COLOR_RESET   "\033[0m"
#define COLOR_INFO    "\033[32m"  // Зеленый
#define COLOR_DEBUG   "\033[34m"  // Синий
#define COLOR_WARNING "\033[33m"  // Желтый
#define COLOR_ERROR   "\033[31m"  // Красный

// Уровни логов
typedef enum { INFO, DEBUG, WARNING, ERROR, RESET } log_level_t;

// Функция логирования
void logger(log_level_t, const char *, ...);

#endif
