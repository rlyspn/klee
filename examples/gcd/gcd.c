// #include <klee/klee.h>

long gcd (long a, long b)
{
    long c;
    while (a != 0) {
        c = a;
        a = b % a;
        b = c;
    }
    return b;
}

int main()
{
    long a, b, c;
    a = 17;
    b = 119;
    c = 332;
    
    // klee_make_symbolic(&b, sizeof(b), "b");
    gcd(a, b);
    gcd(b, c);
    gcd(a, c);
    // gcd(c, b);
}
