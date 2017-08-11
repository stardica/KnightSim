/*
 * decode64.s
 *
 *  Created on: Aug 11, 2017
 *      Author: stardica
 */

.section .text
.globl decode64
.type decode64, @function
decode64:
 mov    %rdi,%rax
 ror    $0x11,%rax
 xor    %fs:0x30,%rax
 mov    %rax,%rdi
 mov    %rdi,%rax
 retq
