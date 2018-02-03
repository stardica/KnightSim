.section .text
.globl setjmp32_2
.type setjmp32_2, @function
setjmp32_2:
 xor    %eax,%eax
 mov    0x4(%esp),%edx
 mov    %ebx,(%edx)
 mov    %esi,0x4(%edx)
 mov    %edi,0x8(%edx)
 lea    0x4(%esp),%ecx
 mov    %ecx,0x10(%edx)
 mov    (%esp),%ecx
 mov    %ecx,0x14(%edx)
 mov    %ebp,0xc(%edx)
 mov    %eax,0x18(%edx)
 ret
