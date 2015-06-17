using "stdio.h";

struct vector {
	int length;
	void** buffer;
};

void iter (vector* v, void (*f)(void*, int)) {
	for (int i = 0; i < v->length; i++)
		f(v->buffer[i], i);
}

int main (int argc, char** argv) {
	vector args = {argc, argv};
	
	iter(&args, [](void* arg, int n) {
		printf("%d: %s\n", n, arg);
	});
	
	return 0;
}