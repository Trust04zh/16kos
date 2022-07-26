#ifndef __KERN_MM_MMU_H__
#define __KERN_MM_MMU_H__

#ifndef __ASSEMBLER__
#include <defs.h>
#endif

// A linear address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |     Index      |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(la) --/ \--- PTX(la) --/ \---- PGOFF(la) ----/
//  \----------- PPN(la) -----------/
//
// The PDX, PTX, PGOFF, and PPN macros decompose linear addresses as shown.
// To construct a linear address la from PDX(la), PTX(la), and PGOFF(la),
// use PGADDR(PDX(la), PTX(la), PGOFF(la)).

// RISC-V uses 32-bit virtual address to access 34-bit physical address!
// Sv32 page table entry:
// +---------12----------+--------10-------+---2----+-------8-------+
// |       PPN[1]        |      PPN[0]     |Reserved|D|A|G|U|X|W|R|V|
// +---------12----------+-----------------+--------+---------------+

/*
 * RV32Sv32 page table entry:
 * | 31 10 | 9             7 | 6 | 5 | 4  1 | 0
 *    PFN    reserved for SW   D   R   TYPE   V
 *
 * RV64Sv39 / RV64Sv48 page table entry:
 * | 63           48 | 47 10 | 9             7 | 6 | 5 | 4  1 | 0
 *   reserved for HW    PFN    reserved for SW   D   R   TYPE   V
 */

// page directory index
#define PDX1(la) ((((uintptr_t)(la)) >> PDX1SHIFT) & PGLEVEL_BITS_MASK) // la here is virtual address
#define PDX0(la) ((((uintptr_t)(la)) >> PDX0SHIFT) & PGLEVEL_BITS_MASK)
// page table index
#define PTX(la) ((((uintptr_t)(la)) >> PTXSHIFT) & PGLEVEL_BITS_MASK)

// page number field of address
#define PASIZE          56                      // bit size of physical address
#define PPN_MASK        (((uint64_t)1 << PASIZE) - 1)
#define PPN(la) ((((uintptr_t)(la)) >> PTXSHIFT) & PPN_MASK) 

// offset in page
#define PGOFF(la) (((uintptr_t)(la)) & PGSHIFT_MASK)

// construct linear address from indexes and offset
#define PGADDR(d1, d0, t, o) ((uintptr_t)((d1) << PDX1SHIFT | (d0) << PDX0SHIFT | (t) << PTXSHIFT | (o)))

// address in page table or page directory entry
#define PTE_ADDR(pte)   (((uintptr_t)(pte) & ~0x3FF) << (PTXSHIFT - PTE_PPN_SHIFT))
#define PDE_ADDR(pde)   PTE_ADDR(pde)

/* page directory and page table constants */
#define PTE_SIZE        8
#define NPDEENTRY       NPTEENTRY               // page directory entries per page directory
#define NPTEENTRY       (PGSIZE / PTE_SIZE)     // page table entries per page table

#define PGSHIFT         14                      // log2(PGSIZE)
#define PGSHIFT_MASK    ((1 << PGSHIFT) - 1)    // mask for page offset
#define PGSIZE          (1 << PGSHIFT)          // bytes mapped by a page
#define PGLEVEL_BITS    11                      
#define PGLEVEL_BITS_MASK   ((1 << PGLEVEL_BITS) - 1)   // mask for page level
#define PTSIZE          (PGSIZE * NPTEENTRY)    // bytes mapped by a page directory entry
#define PTSHIFT         (PGSHIFT + PGLEVEL_BITS)    // log2(PTSIZE)

#define PTXSHIFT        PGSHIFT                 // offset of PTX in a linear address
#define PDX0SHIFT       (PGSHIFT + PGLEVEL_BITS)    // offset of PDX0 in a linear address
#define PDX1SHIFT       (PDX0SHIFT + PGLEVEL_BITS)  // offset of PDX0 in a linear address
#define PTE_PPN_SHIFT   10                      // offset of PPN in a physical address

// page table entry (PTE) fields
#define PTE_V     0x001 // Valid
#define PTE_R     0x002 // Read
#define PTE_W     0x004 // Write
#define PTE_X     0x008 // Execute
#define PTE_U     0x010 // User
#define PTE_G     0x020 // Global
#define PTE_A     0x040 // Accessed
#define PTE_D     0x080 // Dirty
#define PTE_SOFT  0x300 // Reserved for Software

#define PAGE_TABLE_DIR (PTE_V)
#define READ_ONLY (PTE_R | PTE_V)
#define READ_WRITE (PTE_R | PTE_W | PTE_V)
#define EXEC_ONLY (PTE_X | PTE_V)
#define READ_EXEC (PTE_R | PTE_X | PTE_V)
#define READ_WRITE_EXEC (PTE_R | PTE_W | PTE_X | PTE_V)

#define PTE_USER (PTE_R | PTE_W | PTE_X | PTE_U | PTE_V)

#endif /* !__KERN_MM_MMU_H__ */
