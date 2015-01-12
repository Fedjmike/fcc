using "stdio.h";

int main (void) {
    int array[] = {0, 1, 2, 3, 4, 5};
    int length = sizeof(array) / sizeof(array[0]);
    
    for (int i = 0; i < length; i++)
        printf("%d ", array[i]);
        
    puts("");
    
    return 0;
}