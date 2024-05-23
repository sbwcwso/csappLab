# level3_asm.s
subq $0x10, %rsp
movq %rsp, %rdi
movb $0, 0x8(%rsp)
pushq $0x4018fa
ret
