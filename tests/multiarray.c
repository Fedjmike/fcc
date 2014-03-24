#include "stdio.h"
using "stdio.h";

void test2 () {
	int array[2][5] = {{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}};
	printf("%d\n", sizeof(array));
	
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 5; j++)
			printf("%d:%d  ", array[i][j], ((int*) array)[i*5+j]);
			
	puts("");
}

void test3 () {
	int array[3][2][5] = {{{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}},
						  {{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}},
						  {{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}}};
	printf("%d\n", sizeof(array));
	
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 2; j++)
			for (int k = 0; k < 5; k++)
				printf("%d:%d  ", array[i][j][k], ((int*) array)[i*10+j*5+k]);
			
	puts("");
}

int main () {
	test2();
	test3();
	return 0;
}