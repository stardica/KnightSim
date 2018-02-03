.section .text
.globl encode32
.type encode32, @function
encode32:
mov    0x4(%esp),%eax
mov    %eax,%edx
xor    %gs:0x18,%edx
rol    $0x9,%edx
mov    %edx,%eax
ret
