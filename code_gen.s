.section .data
.section .text
.global main
main:
push %rbp
mov %rsp, %rbp
sub $32, %rsp
mov $12, %rax
add $32, %rsp
pop %rbp
ret
