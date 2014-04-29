using "stdio.h";

int main () {
	int x = 5, y = 6;

	[](int* x, int* y) {
		int tmp = *x;
		*x = *y;
		*y = tmp;
	} (&x, &y);
	
	printf("%d %d\n", x, y);
	return 0;
}