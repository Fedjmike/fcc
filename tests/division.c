using "stdio.h";

int main (void) {
    int x = 12;
    x /= 3;

    printf("4: %d \t3: %d\t2: %d\n", x, 11/3, 11 % 3);
    return (x == 4) && (11/3 == 3) && (11 % 3 == 2) ? 0 : 1;
}