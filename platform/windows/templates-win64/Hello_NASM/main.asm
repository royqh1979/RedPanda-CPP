extern printf
extern exit
global main

section .data		; Data section, initialized variables
msg:	db "Hello world", 0	; C string needs 0
fmt:    db "%s", 10, 0          ; The printf format, "\n",'0'

section .text:
main:
	; Microsoft X86_64 Calling convention:
	; - The first four integer or pointer parameters are passed in the rcx, rdx, r8, and r9 registers.
	; - The first four floating-point parameters are passed in the first four SSE registers, xmm0-xmm3.
	; - The caller reserves space on the stack for arguments passed in registers. The called function can use this space to spill the contents of registers to the stack.
	; - Any additional arguments are passed on the stack.
	; - An integer or pointer return value is returned in the rax register, while a floating-point return value is returned in xmm0.
	; see https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/x64-architecture

	sub rsp, 32			; reserve stack space for call
	lea rcx, [rel fmt]	; first parameter 
	lea rdx, [rel msg]	; secodng parameter
	call printf	
	add rsp,32			; restore stack
	
	mov eax,0		; exit code	
	ret