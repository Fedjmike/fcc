struct ast;

ast astCreate
	(ast,
	 ast* x,
	 ast const const y);

int main () {
	ast Node,
		*current;
	Node.a;
	Node->a;
	current->a;
	*current;
	current[0];
	return 0;
}

//5 errors expected, lines 3, 4, 6, 12, 13
//4 elided on 6, 9, 14, 15