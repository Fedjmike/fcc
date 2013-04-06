int dbl (int x) {
	return 2*x;
}

void map (int (*f)(int), int* list, int n) {
	for (int i = 0; i < n; i++)
		list[i] = f(list[i]);
}

int main () {
	int array[] = {0, 1, 2, 3, 4, 5, 6, 7};

	map(dbl, array, 8);

	/*Returns 8*/
	return array[4];
}
