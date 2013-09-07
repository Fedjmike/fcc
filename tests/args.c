using "stdio.h";

int main (int argc, char** argv) {
    printf("%s: ", argv[0]);
    
    for (int i = 1; i < argc; i++)
        printf("%s ", argv[i]);
        
    putchar((int) '\n');
    
    return 0;
}