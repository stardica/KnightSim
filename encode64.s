.section .text
.globl encode64
.type encode64, @function
encode64:
 mov    %rdi,%rax
 xor    %fs:0x30,%rax
 rol    $0x11,%rax
 mov    %rax,%rdi
 mov    %rdi,%rax
 retq
