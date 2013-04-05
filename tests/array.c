void double_arr (int* list, int n) {
	for (int i = 0; i < n; i++)
		list[i] *= 2;
}

int main () {
	int array[] = {0, 1, 2, 3, 4, 5, 6, 7};

	double_arr(array, 8);

	/*Returns 8*/
	return array[4];
}
