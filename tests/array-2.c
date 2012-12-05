/**
 * Test the various ways that a pointer/array can be a value
 */

//Test lvalue pointer indexing and rvalue pointer indexing
void g (int* ref) {
	ref[2] = 3*ref[2]; 
}

//Test rvalue pointer passing (as well as those two other cases)
void f (int* ref) {
	g(ref);

	ref[1] = 2*ref[1];
}

//Test rvalue array passing
//and lvalue array indexing and rvalue array indexing
int main () {
	int array[] = {0, 1, 2, 3, 4};
	
	f(array);
	array[1] *= 2;
	
	/*Returns 10*/
	return array[1] + array[2];
}