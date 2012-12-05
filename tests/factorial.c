int factRecursive (int n) {
	if n <= 1
		return 1;
		
	else
		return n*factRecursive(n-1);
}

int factLoop (int n) {
	int ret = 1;

	while n > 1
		ret = ret*n--;

	return ret;
}

int main () {
	/*Returns 6! % 256 == 208*/
	return factRecursive(6);
}
