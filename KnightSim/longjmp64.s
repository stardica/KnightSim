.section .text
.globl longjmp64_2
.type longjmp64_2, @function
longjmp64_2:
mov    0x8(%rdi),%r9
mov    0x10(%rdi),%r12
mov    0x18(%rdi),%r13
mov    0x20(%rdi),%r14
mov    0x28(%rdi),%r15
mov    0x30(%rdi),%r8
mov    0x38(%rdi),%rdx
mov    (%rdi),%rbx
mov    %esi,%eax
mov    %r8,%rsp
mov    %r9,%rbp
jmpq   *%rdx

