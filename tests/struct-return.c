using "stdio.h";

struct A {
	int x, y, z;
};

A f () {
	A a = {0, 1, 2};
	return a;
}

int main () {
	A a = f();
	printf("%d %d %d\n", a.x, a.y, a.z);
	return 0;
}