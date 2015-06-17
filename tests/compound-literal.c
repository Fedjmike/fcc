using "stdio.h";

typedef struct A {
    int x, y;
} A;

int f (int *p, int n) {
	for (int i = 0; i < n; i++)
		printf("%d  ", p[i]);
		
	puts("");
}

int main () {
    A a = (A) {0, 0};
	
	f((int[]) {0, 1, 2}, 3);
		
    return 0;
}