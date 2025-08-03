#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    printf("Basic main.cpp entrypoint.\n");
    while (true) {
        sleep_ms(1000);
    }
    return 0;
}
