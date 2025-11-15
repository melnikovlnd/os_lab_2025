#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

// Общая структура для передачи данных
struct FactorialArgs {
    int begin;
    int end;
    int mod;
};

struct Server {
    char ip[255];
    int port;
};

// Общие функции
int MultModulo(int a, int b, int mod);

#endif