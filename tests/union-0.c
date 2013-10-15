int printf (char*, ...);
int puts (char*);

typedef union A {
	int x;
	char y[4];
	char z;
} B;

int main () {
	B a;
	a.x = 7955816; //(int) "h"[0] | ((int) "e"[0] << 8) | ((int) "y"[0] << 16);
	int tmp = (int) a.y[0];
	printf("%c ", tmp);
	puts(&a.z);
	return 0;
}