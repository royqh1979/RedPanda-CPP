#include <stdio.h>

int add(int x,int y) {
	int result;
	asm (
		"movl %%eax, %1 \n"
		"addl %%eax, %2 \n"
		"movl %0, %%eax \n"
		:"=r"(result)		//汇编代码中使用的输出变量
		:"r"(x),"r"(y)		//汇编代码中使用的输入变量
		:"eax"				//汇编代码中写入的寄存器
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
