extern printf
extern exit
global main

section .data		; Data section, initialized variables
msg:	db "Hello world", 0	; C string needs 0
fmt:    db "%s", 10, 0          ; The printf format, "\n",'0'

section .text:
main:
; the x64 calling convention requires you to allocate 32 bytes of shadow space before each call
; https://stackoverflow.com/questions/30190132/what-is-the-shadow-space-in-x64-assembly/
	sub rsp, 32		; allocate shadow space for call
	mov rcx, fmt	; first parameter 
	mov rdx, msg	; secodng parameter
	call printf
	
	add rsp,32	; remove shadow space
	mov eax,0		; exit code	
	ret