using "stdio.h";
using "stdlib.h";

int main () {
    int x = 5;
    
    int (*read)(void) = [x; malloc]() (x++);
    
    printf("%d\n", read()); x++;
    printf("%d\n", read()); x++;
    printf("%d\n", read()); x++;
    
    free(read);
    
    return read() == 8 ? 0 : 1;
}
