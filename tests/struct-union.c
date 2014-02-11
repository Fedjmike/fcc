using "stdio.h";

struct A {
    union {
        int v;
        struct {
            char w, x, y, z;
        };
    };
};

int main () {
    A a;
    a.v = (int) 'f' << 8;
    printf("%c\n", a.x);
    return 0;
}