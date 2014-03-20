struct ast;

ast astCreate (ast, ast* x, ast const const y);

int main () {
	ast Node, *current;
	Node.a;
	Node->a;
	current->a;
	*current;
	current[0];
	return 0;
}

//9 errors expected, lines 3, 3, 3, 3, 6, 8, 9, 10, 11