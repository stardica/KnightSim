.section .text
.globl get_stack_ptr64
.type get_stack_ptr64, @function
get_stack_ptr64:
 mov   %rsp,%rax
 retq
