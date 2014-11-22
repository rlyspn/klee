//#include <stdio.h>
#include <klee/klee.h>

int max(int a, int b, int c, int d) {
    if(a > b && a > c && a > d) return a;
    if(b > c && b > d) return b;
    if(c > d) return c;
    return d;
}

int main() {
    int a1 = 3, a2 = 77, a3 = 8, a4 = 2;

    int m0 = max(a1, a2, a3, a4);

    klee_make_symbolic(&a1, sizeof(a1), "a1");
    int m1 = max(a1, a2, a3, a4);

    klee_make_symbolic(&a2, sizeof(a2), "a2");
    int m2 = max(a1, a2, a3, a4);

    klee_make_symbolic(&a3, sizeof(a3), "a3");
    int m3 = max(a1, a2, a3, a4);

    klee_make_symbolic(&a4, sizeof(a4), "a4");
    int m4 = max(a1, a2, a3, a4);

    //printf("%d %d %d %d %d\n", m0, m1, m2, m3, m4);

    return 0;
}
