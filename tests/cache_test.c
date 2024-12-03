#include "cache.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    cache *cache = cache_init();
    if (!cache) return 1;

    cache_add(cache, "http://example.com", "Example Data", 13, time(NULL) + 10);
    cache_add(cache, "http://google.com", "Google Data", 12, time(NULL) + 5);

    cache_print(cache);

    if (cache_find(cache, "http://example.com")) {
        printf("Found: http://example.com\n");
    }

    sleep(6);

    cache_remove_expired(cache);
    cache_print(cache);

    cache_destroy(cache);
    return 0;
}
