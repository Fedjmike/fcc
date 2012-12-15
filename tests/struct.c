struct A {
	int x, y, z;
};

struct B {
	A* x;
	int y[3];
};

int f (B b) {
	return b.x->y;
}

int main () {
	A a;// = {1, 2, 3};
	B b;// = {0, {1, 2, 3}};
	
	a.y = 2;
	
	b.x = &a;
	b.x->y *= 2;
	b.y[1] = 5;
	
	/*Returns 9*/
	return b.y[1] + f(b);
}