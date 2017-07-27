/*
 * setjmp64.s
 *
 *  Created on: Mar 25, 2017
 *      Author: stardica
 */

.section .text
.globl setjmp64_2
.type setjmp64_2, @function
setjmp64_2:
mov    %rbx,(%rdi)
mov    %rbp,%rax
xor    %fs:0x30,%rax
rol    $0x11,%rax
mov    %rax,0x8(%rdi)
mov    %r12,0x10(%rdi)
mov    %r13,0x18(%rdi)
mov    %r14,0x20(%rdi)
mov    %r15,0x28(%rdi)
lea    0x8(%rsp),%rdx
xor    %fs:0x30,%rdx
rol    $0x11,%rdx
mov    %rdx,0x30(%rdi)
mov    (%rsp),%rax
xor    %fs:0x30,%rax
rol    $0x11,%rax
mov    %rax,0x38(%rdi)
xor    %rax,%rax
ret
