using "stdio.h";

struct A {
	int x;
	int array[15];
	int y, z;
};

int main (void) {
	A a = {.array = {[1] = 2, 3, 4, [0] = 1}, 5, 6};
	
	printf("0: %d\n", a.x);
	printf("1: %d\n", a.array[0]);
	printf("2: %d\n", a.array[1]);
	printf("3: %d\n", a.array[2]);
	printf("4: %d\n", a.array[3]);
	printf("5: %d\n", a.y);
	printf("6: %d\n", a.z);
	
	return 0;
}