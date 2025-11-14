    global  maxofthree
    global  add3
section .text
maxofthree:
	; 因为参数均为32位整型（int），所以使用ecx,eax等寄存器而非rcx，rax
	mov     eax, ecx		; 将参数x（在ecx中）放到函数返回值（eax)中
	cmp     eax, edx		; 比较x（eax）和y（edx）
	cmovl   eax, edx		; 如果x小于y，将y设置为返回值
	cmp     eax, r8d      ; 比较xy中最大值（eax）和z
	cmovl   eax, r8d      ; 如果比z小，将z设置为返回值
	ret                     ; 返回eax中的结果
add3:
	mov eax, ecx
	add eax, edx
	add eax, r8d
	ret