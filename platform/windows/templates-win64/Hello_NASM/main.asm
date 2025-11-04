	global main
	extern printf
section .text	
main:
    ; Microsoft X86_64 stack usage: 
	; - stack should be maintained 16-byte aligned 
	; see https://learn.microsoft.com/en-us/cpp/build/stack-usage
	
	; Microsoft X86_64 Calling convention:
	; - The first four integer or pointer parameters are passed in the rcx, rdx, r8, and r9 registers.
	; - The first four floating-point parameters are passed in the first four SSE registers, xmm0-xmm3.
	; - The caller reserves space on the stack for arguments passed in registers(32 bytes). Debuggers can use it to store debug infos.
	; - Any additional arguments are passed on the stack.
	; - An integer or pointer return value is returned in the rax register, while a floating-point return value is returned in xmm0.
	; see https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/x64-architecture
	
	; By default on x64, gcc generate and link obj files using Position Independent Code(PIC) codes.
	; So we should use RIP addressing in NASM.
	
	sub 	rsp, 32           ; reserve stack space for the call
	lea	rcx, [rel fmt]		; first parameter (RIP addressing) 
	lea	rdx, [rel msg]		; second parameter (RIP addressing)
	call 	printf
	add 	rsp,32           ; restore stack space

	xor 	eax,eax	        ; set 0 as exit code 
	ret
	

msg: db	"Hello world\n",0  
fmt: db	"%s",0	 
