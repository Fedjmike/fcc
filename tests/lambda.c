using "stdio.h";

int main (int argc, char** argv) {
	int x = 0;
	
	int (*add)(int*, int) = [](int* x, int y) {
		int old = *x;
		*x += y;
		return old;
	};
	
	printf("0: %d\n", add(&x, 5));
	printf("5: %d\n", add(&x, 2));
	printf("7: %d\n", add(&x, 3));
	printf("10: %d\n", x);
	return 0;
}