#include <stdio.h>
#include <string.h>

typedef struct Data {
    char    buf[10];
    int     i;
    float   f;
} Data;

int main() {
    Data d1, d2;
    
    char buf[10] = "10001";
    memcpy(d1.buf, buf, 10);
    d1.i = 100;
    d1.f = 101.01;

    d2 = d1;
    printf("%s, %d, %f\n", d2.buf, d2.i, d2.f);
    return 0;
}
