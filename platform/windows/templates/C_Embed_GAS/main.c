#include <stdio.h>

int add(int x,int y) {
	int result;
	asm (
		"movl %%eax, %1 \n"
		"addl %%eax, %2 \n"
		"movl %0, %%eax \n"
		:"=r"(result)		//output operands used in the instructions
		:"r"(x),"r"(y)		//input operands used in the instructions
		:"eax"				//registers changed by the assembler instructions
		);
	return result;
}

int main() {
	int a,b,c;
	scanf("%d,%d",&a,&b);
	c=a+b;
	printf("%d\n",c);
	return 0;
}