#include "swap.h"

void Swap(char *left, char *right)
{
	*left = *left + *right;
    *right = *left - *right;
    *left = *left - *right;
}
