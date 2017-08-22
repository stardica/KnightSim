.section .text
.globl decode32
.type decode32, @function
decode32:
mov    0x4(%esp),%eax
mov    %eax,%edx
ror    $0x9,%edx
xor    %gs:0x18,%edx
mov    %edx,%eax
ret
