	.text
	.globl main
	.extern printf

main:
	# AMD64 Calling convention:
	# - The first six parameters are passed in the rsi, rbi, rdx, rdx, r8, and r9 registers.
	# - Any additional arguments are passed on the stack.
	# - An return value is returned in the rax register.
	# see https://en.wikipedia.org/wiki/X86_calling_conventions#x86-64_calling_conventions	

	leaq	fmt(%rip), %rdi		# first parameter 
	leaq	msg(%rip), %rsi		# second parameter
	mov 	$0, %eax
	call 	printf

	xor 	%eax,%eax	        # set 0 as exit code 
	ret
	

msg:
	.asciz	"Hello world\n"  # asciz puts a 0 byte at the end
fmt:
	.asciz	"%s"	 # asciz puts a 0 byte at the end
