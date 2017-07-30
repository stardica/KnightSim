/*
 * longjmp64.s
 *
 *  Created on: Mar 25, 2017
 *      Author: stardica
 */

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


/*
.section .text
.globl longjmp64_2
.type longjmp64_2, @function
longjmp64_2:
mov    0x30(%rdi),%r8
mov    0x8(%rdi),%r9
mov    0x38(%rdi),%rdx
ror    $0x11,%r8
xor    %fs:0x30,%r8
ror    $0x11,%r9
xor    %fs:0x30,%r9
ror    $0x11,%rdx
xor    %fs:0x30,%rdx
mov    (%rdi),%rbx
mov    0x10(%rdi),%r12
mov    0x18(%rdi),%r13
mov    0x20(%rdi),%r14
mov    0x28(%rdi),%r15
mov    %esi,%eax
mov    %r8,%rsp
mov    %r9,%rbp
jmpq   *%rdx
*/
