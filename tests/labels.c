/*
CALL fn       BARE
CALL static   PTR
CALL reg      BARE
LD   fn       OFFSET
LD   static   BARE/PTR
LD   string   OFFSET
ST   static   PTR

_    static   => PTR     operandLabelMem
_    string   => OFFSET  operandLabelOffset
_    fn       =>         operandLabel

- fn call / jump to label: BARE
- fn to register / label to register: OFFSET
- static (extern, global?) variable store/load: BARE?
- string literal load: OFFSET?
- call / jump to register: BARE
- call / jump to static variable: PTR*/

void (*global)();

void f () {
	printf("global == %d\n", global);
}

int main () {
	(global = f)();
	return 0;
}