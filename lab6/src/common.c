#include "common.h"
#include <errno.h>
#include <stdlib.h>

int MultModulo(int a, int b, int mod) {
    int result = 0;
    a = a % mod;
    while (b > 0) {
        if (b % 2 == 1)
            result = (result + a) % mod;
        a = (a * 2) % mod;
        b /= 2;
    }
    return result % mod;
}