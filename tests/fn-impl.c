int (*f)(int, int);

int f(int x, int x) {
	int x;
	int x;
}

int (*f)(int, int) {
	//Not allowed
}

struct f {
};

/*Errors:
- Redeclared var, param
	analyzerDecl, analyzerDeclParam!!!
- Function implemented and declared as differing types
	analyzerDecl
- Reimplemented function, struct, enum
	parserDeclCall, parserDeclStruct, parserDeclEnum
- Redeclared as different symbol type
	parserName*/