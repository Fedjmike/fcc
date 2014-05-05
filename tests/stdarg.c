using "stdarg.h";
using "stdio.h";

int average (int n, ...) {
	va_list args;
	va_start(args, n);
	
	int sum = 0;
	
	for (int i = 0; i < n; i++) {
		vprintf("%d ", args);
		sum += va_arg(args, int);
	}
	
	va_end(args);
	
	return sum/n;
}

int main (void) {
	printf("%d\n", average(5, 1, 2, 3, 4, 5));
	return 0;
}