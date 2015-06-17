//#include "stdio.h"

using "stdio.h";

int main (void) {
	int n = 20;
	/*x: bits 10 and 20*/
	int x = (1 << 10) | (1 << n);
	
	printf("1025: %d\n", x >> 10);
	
	/*x: bit 30*/
    x <<= n;
	
	printf("1024: %d\n", x >> n);
    return 0;
}