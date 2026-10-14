#include <xc.h>
extern unsigned int count_primes(unsigned int a, unsigned int b);
int main(void) {
    volatile unsigned int ans = count_primes(1, 65535);
    while (1);
    return 0;
}
