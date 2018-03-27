.section .text
.globl get_stack_ptr32
.type get_stack_ptr32, @function
get_stack_ptr32:
 mov   %esp,%eax
 ret
