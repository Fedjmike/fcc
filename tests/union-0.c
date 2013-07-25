int printf (char*, ...);
int puts (char*);

union A {
	int x;
	char y[4];
	char z;
};

int main () {
	A a;
	a.x = (int) "h"[0] | ((int) "e"[0] << 8) | ((int) "y"[0] << 16);
	printf("%c ", (int) a.y[0]);
	puts(&a.z);
	return 0;
}