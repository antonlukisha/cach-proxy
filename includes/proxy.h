#ifndef PROXY_H
#define PROXY_H

#include "cache.h"
#include "thread_pool.h"
#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define PORT 8080

int proxy_init(int);
void proxy_start(int, int);
char *extract_url(char *);
void handle_client(int);

#endif
