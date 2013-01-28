#include <stdio.h>

void main()
{
	struct A{
		char tr[3];
		double b;
	};

	struct A *p;
	struct A st[2] = {"gao", 10, 5};
	p = &st;

	printf("%lu\n", (unsigned long)p);
	printf("%lu\n", (unsigned long)(p + 1));
	printf("sizeof(int) is: %d, and sizeof(double) is: %d, and %d\n", sizeof(int), sizeof(double), sizeof(struct A));

	return;
}
	
	
