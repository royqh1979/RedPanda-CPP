	.text
	.globl main
main:
	# Microsoft X86_64 Calling convention:
	# - The first four integer or pointer parameters are passed in the rcx, rdx, r8, and r9 registers.
	# - The first four floating-point parameters are passed in the first four SSE registers, xmm0-xmm3.
	# - The caller reserves space on the stack for arguments passed in registers. The called function can use this space to spill the contents of registers to the stack.
	# - Any additional arguments are passed on the stack.
	# - An integer or pointer return value is returned in the rax register, while a floating-point return value is returned in xmm0.
	# see https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/x64-architecture
	
	sub 	$32, %rsp           # reserve stack space for the call
	leaq	fmt(%rip), %rcx		# first parameter 
	leaq	msg(%rip), %rdx		# second parameter
	call 	printf
	add 	$32, %rsp           # restore stack space

	xor 	%eax,%eax	        # set 0 as exit code 
	ret
	

msg:
	.asciz	"Hello world\n"  # asciz puts a 0 byte at the end
fmt:
	.asciz	"%s"	 # asciz puts a 0 byte at the end