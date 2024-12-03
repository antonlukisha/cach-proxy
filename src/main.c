#include "proxy.h"

#define PORT 8080

int main() {
    int socket = proxy_init(PORT);
    proxy_start(PORT, socket);
    return 0;
}
