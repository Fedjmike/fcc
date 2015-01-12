using "stdio.h";

int f () {
	return 10;
}

int main () {
	int x = 5 + (5 + (f() + (10 / 5)));
	printf("%d\n", x);
	return 0;
}