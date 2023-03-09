    .text
    .globl  maxofthree
    .globl  add3
maxofthree:
	#因为参数均为32位整型（int），所以使用ecx,eax等寄存器而非rcx，rax
	mov     %ecx, %eax		# 将参数x（在ecx中）放到函数返回值（eax)中
	cmp     %edx, %eax		# 比较x（eax）和y（edx）
	cmovl   %edx, %eax		# 如果x小于y，将y设置为返回值
	cmp     %r8d, %eax      # 比较xy中最大值（eax）和z
	cmovl   %r8d, %eax      # 如果比z小，将z设置为返回值
	ret                     # 返回eax中的结果
add3:
	mov %ecx, %eax
	add %edx, %eax
	add %r8d, %eax
	ret
