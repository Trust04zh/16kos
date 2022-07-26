#include <mmu.h>
#include <memlayout.h>

    .section .text,"ax",%progbits
    .globl kern_entry
kern_entry:
    # t0 := 三级页表的物理地址
    la     t0, boot_page_table_sv47
    # t0 变为三级页表的物理页号
    srli    t0, t0, PGSHIFT

    # t1 := 14 << 60，设置 satp 的 MODE 字段为 Sv47
    li      t1, 14 << 60
    # 将刚才计算出的预设三级页表物理页号附加到 satp 中
    or      t0, t0, t1
    # 将算出的 t0(即新的MODE|页表基址物理页号) 覆盖到 satp 中
    csrw    satp, t0
    # 使用 sfence.vma 指令刷新 TLB
    sfence.vma
    # 从此，我们给内核搭建出了一个完美的虚拟内存空间！
    #nop # 可能映射的位置有些bug。。插入一个nop

    # 设置sp
    la sp, bootstacktop
    li t1, PHYSICAL_MEMORY_OFFSET
    or sp, sp, t1

    # 跳转到 kern_init
    la t0, kern_init
    li t1, PHYSICAL_MEMORY_OFFSET
    or t0, t0, t1
    jr t0

.section .data
    .align PGSHIFT
    .global bootstack
bootstack:
    .space KSTACKSIZE
    .global bootstacktop
bootstacktop:

.section .data
    .align PGSHIFT
    .global boot_page_table_sv47
# 分配一个页给预设的第一级页表
boot_page_table_sv47:
    # 0xffff_fff0_0000_0000 map to 0x00000000 (64G)
    # 前 2047 个页表项均设置为 0 ，因此 V=0 ，意味着是空的(unmapped)
    .zero 8 * ((1 << PGLEVEL_BITS) - 1)
    # 设置最后一个页表项，标志位 VRWXAD 均为 1
    .quad 0xcf # VRWXAD
