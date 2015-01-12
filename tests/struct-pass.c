using "stdio.h";

struct A {
	int x, y, z;
};

void f (A a) {
	printf("%d %d %d\n", a.x, a.y, a.z);
}

int main () {
	f((A) {1, 2, 3});
	return 0;
}