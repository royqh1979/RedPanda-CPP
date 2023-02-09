extern printf
extern exit
global main

section .data		; Data section, initialized variables
msg:	db "Hello world", 0	; C string needs 0
fmt:    db "%s", 10, 0          ; The printf format, "\n",'0'

section .text:
main:
	push rbp		; align our stack at entry
	mov rbp, rsp	; use RBP as frame reference
	sub rsp, 32		; our 16-byte aligned local storage area
	
	mov rcx, fmt
	mov rdx, msg
	call printf
	
	mov eax,0		; exit code	
	add rsp,32		; restore stack
	pop rbp			; restore stack
	ret
