#include <xc.h>
extern unsigned char gcd(unsigned char a, unsigned char b);
int main(void) {
    volatile unsigned char ans = gcd(17, 5);
    while (1);
    return 0;
}
