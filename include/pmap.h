#ifndef _PMAP_H_
#define _PMAP_H_

#include <mmu.h>
#include <printk.h>
#include <queue.h>
#include <types.h>

extern Pde *cur_pgdir;

LIST_HEAD(Page_list, Page);//定义了名为Page_list的结构体，该结构体内有struct Page* lh_first，该指针指向一个Page结构体
typedef LIST_ENTRY(Page) Page_LIST_entry_t;

struct Page {
	Page_LIST_entry_t pp_link; /* free list link */

	// Ref is the count of pointers (usually in page table entries)
	// to this page.  This only holds for pages allocated using
	// page_alloc.  Pages allocated at boot time using pmap.c's "alloc"
	// do not have valid reference count fields.

	u_short pp_ref;//引用次数
};

extern struct Page *pages;
extern struct Page_list page_free_list;

static inline u_long page2ppn(struct Page *pp) {
	return pp - pages;
}

static inline u_long page2pa(struct Page *pp) { //通过页表来找到物理地址
	return page2ppn(pp) << PGSHIFT;
}

static inline struct Page *pa2page(u_long pa) {
	if (PPN(pa) >= npage) {
		panic("pa2page called with invalid pa: %x", pa);
	}
	return &pages[PPN(pa)];
}

static inline u_long page2kva(struct Page *pp) { //通过页表来找到虚拟地址，KADDR=PADDR+0x80000000,kaddr和paddr就是直接物理地址与虚拟地址加减0x80000000，不会涉及到页表映射的查找
	return KADDR(page2pa(pp));
}

static inline u_long va2pa(Pde *pgdir, u_long va) {
	Pte *p;

	pgdir = &pgdir[PDX(va)];
	if (!(*pgdir & PTE_V)) {
		return ~0;
	}
	p = (Pte *)KADDR(PTE_ADDR(*pgdir));
	if (!(p[PTX(va)] & PTE_V)) {
		return ~0;
	}
	return PTE_ADDR(p[PTX(va)]);
}

void mips_detect_memory(u_int _memsize);
void mips_vm_init(void);
void mips_init(u_int argc, char **argv, char **penv, u_int ram_low_size);
void page_init(void);
void *alloc(u_int n, u_int align, int clear);

int page_alloc(struct Page **pp);
void page_free(struct Page *pp);
void page_decref(struct Page *pp);
int page_insert(Pde *pgdir, u_int asid, struct Page *pp, u_long va, u_int perm);
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte);
void page_remove(Pde *pgdir, u_int asid, u_long va);

extern struct Page *pages;

void physical_memory_manage_check(void);
void page_check(void);
u_int page_filter(Pde *pgdir, u_long va_lower_limit, u_long va_upper_limit, u_int num);
#endif /* _PMAP_H_ */
