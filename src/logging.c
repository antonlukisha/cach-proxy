#include "logging.h"

// Имена уровней логов
const char *log_level_names[] = { "INFO", "DEBUG", "WARNING", "ERROR", "RESET" };

// Цвета для уровней логов
const char *log_level_colors[] = { COLOR_INFO, COLOR_DEBUG, COLOR_WARNING, COLOR_ERROR, COLOR_RESET };

void logger(log_level_t level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    time_t now = time(NULL); // Получение текущего времени
    struct tm *local_time = localtime(&now);

    // Вывод логов с временной меткой, уровнем и идентификатором потока
    fprintf(stdout, "%s[%02d:%02d:%02d] [%s] [Thread %lu] ",
        log_level_colors[level], // цвет
        local_time->tm_hour, // часы
        local_time->tm_min, // минуты
        local_time->tm_sec, // секунды
        log_level_names[level], // уровень
        pthread_self()
    );
    vfprintf(stdout, format, args); // Вывод сообщения
    fprintf(stdout, "%s\n", COLOR_RESET); // Перенос строки и возврат цвета в исходный
    va_end(args);
}
