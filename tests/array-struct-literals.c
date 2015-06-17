struct A {
	int x[2], y[2];
};

int f (A a, int* array) {
	return (a.x[0]+a.x[1])*array[0] +
		   (a.y[0]+a.y[1])*array[1];
}

int main () {
	/*Returns 17*/
	return f((A) {{1, 2}, {3, 4}}, (int[]) {1, 2});
}