#include <stdio.h>

#ifdef __cplusplus
#define ASM_FUNC extern "C"
#else
#define ASM_FUNC
#endif

/*
  在cpp中使用extern "C"声明函数，以避免编译时其函数名被修饰(mangle)，
  导致链接失败
*/
ASM_FUNC int maxofthree(int, int, int);
ASM_FUNC int add3(int, int, int);

int main() {
	printf("%d\n", add3(1, -4, -7));
	printf("%d\n", add3(1, 2, 3));
	printf("%d\n", add3(2, -6, 1));

	printf("------\n");
	
	printf("%d\n", maxofthree(1, -4, -7));
	printf("%d\n", maxofthree(2, -6, 1));
	printf("%d\n", maxofthree(2, 3, 1));
	printf("%d\n", maxofthree(-2, 4, 3));
	printf("%d\n", maxofthree(2, -6, 5));
	printf("%d\n", maxofthree(2, 4, 6));
	return 0;
}
