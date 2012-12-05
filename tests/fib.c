int main () {
	int n = 10;
	int fibs[10] = {1, 1};

	for	int i = 2; i < n; i++
		fibs[i] = fibs[i-1]+fibs[i-2];

	/*Returns 55*/
	return fibs[n-1];
}
