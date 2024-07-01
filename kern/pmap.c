#include <bitops.h>
#include <env.h>
#include <malta.h>
#include <mmu.h>
#include <pmap.h>
#include <printk.h>

/* These variables are set by mips_detect_memory(ram_low_size); */
static u_long memsize; /* Maximum physical address */
u_long npage;	       /* Amount of memory(in pages) */

Pde *cur_pgdir;

struct Page *pages;
static u_long freemem;

struct Page_list page_free_list; /* Free list of physical pages */
//Page_list是个结构体，不是指针！
/* Overview:
 *   Use '_memsize' from bootloader to initialize 'memsize' and
 *   calculate the corresponding 'npage' value.
 */
void mips_detect_memory(u_int _memsize) { //获取总物理内存大小，并根据物理内存计算分页数。
	/* Step 1: Initialize memsize. */
	memsize = _memsize;

	/* Step 2: Calculate the corresponding 'npage' value. */
	/* Exercise 2.1: Your code here. */
	npage = memsize / PAGE_SIZE;//需要查看mmu.h的定义的宏得到每一页的大小,从而得到页数
	printk("Memory size: %lu KiB, number of pages: %lu\n", memsize / 1024, npage);
}

/* Lab 2 Key Code "alloc" */
/* Overview:
    Allocate `n` bytes physical memory with alignment `align`, if `clear` is set, clear the
    allocated memory.
    This allocator is used only while setting up virtual memory system.
   Post-Condition:
    If we're out of memory, should panic, else return this address of memory we have allocated.*/
void *alloc(u_int n, u_int align, int clear) {
	extern char end[];
	u_long alloced_mem;

	/* Initialize `freemem` if this is the first time. The first virtual address that the
	 * linker did *not* assign to any kernel code or global variables. */
	if (freemem == 0) {
		freemem = (u_long)end; // end
	}

	/* Step 1: Round up `freemem` up to be aligned properly */
	freemem = ROUND(freemem, align);

	/* Step 2: Save current value of `freemem` as allocated chunk. */
	alloced_mem = freemem;

	/* Step 3: Increase `freemem` to record allocation. */
	freemem = freemem + n;

	// Panic if we're out of memory.
	panic_on(PADDR(freemem) >= memsize);

	/* Step 4: Clear allocated chunk if parameter `clear` is set. */
	if (clear) {
		memset((void *)alloced_mem, 0, n);
	}

	/* Step 5: return allocated chunk. */
	return (void *)alloced_mem;
}
/* End of Key Code "alloc" */

/* Overview:
    Set up two-level page table.
   Hint:
    You can get more details about `UPAGES` and `UENVS` in include/mmu.h. */
void mips_vm_init() {
	/* Allocate proper size of physical memory for global array `pages`,
	 * for physical memory management. Then, map virtual address `UPAGES` to
	 * physical address `pages` allocated before. For consideration of alignment,
	 * you should round up the memory size before map. */
	pages = (struct Page *)alloc(npage * sizeof(struct Page), PAGE_SIZE, 1);
	printk("to memory %x for struct Pages.\n", freemem);
	printk("pmap.c:\t mips vm init success\n");
}

/* Overview:
 *   Initialize page structure and memory free list. The 'pages' array has one 'struct Page' entry
 * per physical page. Pages are reference counted, and free pages are kept on a linked list.
 *
 * Hint: Use 'LIST_INSERT_HEAD' to insert free pages to 'page_free_list'.
 */
void page_init(void) {
	/* Step 1: Initialize page_free_list. */
	/* Hint: Use macro `LIST_INIT` defined in include/queue.h. */
	/* Exercise 2.3: Your code here. (1/4) */
	LIST_INIT(&page_free_list); //page_free_list是个结构体，LIST_INIT需要指针，因此取地址
	/* Step 2: Align `freemem` up to multiple of PAGE_SIZE. */
	/* Exercise 2.3: Your code here. (2/4) */
	freemem = ROUND(freemem, PAGE_SIZE);
	/* Step 3: Mark all memory below `freemem` as used (set `pp_ref` to 1) */
	/* Exercise 2.3: Your code here. (3/4) */
	for(u_long i=0; i<npage ; i++ ){ //freemem代表虚拟地址
		if(page2kva(&pages[i])<freemem){ //求出第i页的虚拟地址，若比freemem小，那么置1，表示被用了 pages[i]表示第i页 (&pages[i]-pages得到i)
			pages[i].pp_ref = 1;
		}else{
			pages[i].pp_ref = 0;
			LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);//插入空闲链表,field在此处通过函数了解到调用函数久就成了(&pages[i])->pp_link，因为解引用所以有->
		}
	}
	/* Step 4: Mark the other memory as free. */
	/* Exercise 2.3: Your code here. (4/4) */

}

/* Overview:
 *   Allocate a physical page from free memory, and fill this page with zero.
 *
 * Post-Condition:
 *   If failed to allocate a new page (out of memory, there's no free page), return -E_NO_MEM.
 *   Otherwise, set the address of the allocated 'Page' to *pp, and return 0.
 *
 * Note:
 *   This does NOT increase the reference count 'pp_ref' of the page - the caller must do these if
 *   necessary (either explicitly or via page_insert).
 *
 * Hint: Use LIST_FIRST and LIST_REMOVE defined in include/queue.h.
 */
int page_alloc(struct Page **new) { //现在还没有建立内存管理机制，全是对虚拟地址kseg0进行操作
	/* Step 1: Get a page from free memory. If fails, return the error code.*/
	struct Page *pp;//指向页
	/* Exercise 2.4: Your code here. (1/2) */
	if(LIST_EMPTY(&page_free_list)){ //如果没有空闲空间
		return -E_NO_MEM;
	} else{
		pp = LIST_FIRST(&page_free_list);//指向页的指针
	}
	LIST_REMOVE(pp, pp_link);//从page_free_list删去这页
	/* Step 2: Initialize this page with zero.
	 * Hint: use `memset`. */
	/* Exercise 2.4: Your code here. (2/2) */
	/*考虑 kseg0 段的性质，该段上的虚拟地址被线性映射到物理地址，操作系统通过访问该段的地址来直接操作物理内存,例如当写虚拟地址 0x80012340时，由于 kseg0 段的性质，事实上在写物理地址 0x12340。*/
	memset((void*)page2kva(pp), 0, sizeof(struct Page));//因此此处对page2kva清0，经过映射，实际上是对物理空间做操作了
	*new = pp;
	return 0;
}

/* Overview:
 *   Release a page 'pp', mark it as free.
 *
 * Pre-Condition:
 *   'pp->pp_ref' is '0'.
 */
void page_free(struct Page *pp) {
	assert(pp->pp_ref == 0);
	/* Just insert it into 'page_free_list'. */
	/* Exercise 2.5: Your code here. */
	LIST_INSERT_HEAD(&page_free_list, pp, pp_link);//将该页重新插回空闲链表即可
	//field装着pp_link，作为特征
}

/* Overview:
 *   Given 'pgdir', a pointer to a page directory, 'pgdir_walk' returns a pointer to
 *   the page table entry for virtual address 'va'.
 *
 * Pre-Condition:
 *   'pgdir' is a two-level page table structure.
 *   'ppte' is a valid pointer, i.e., it should NOT be NULL.
 *
 * Post-Condition:
 *   If we're out of memory, return -E_NO_MEM.
 *   Otherwise, we get the page table entry, store
 *   the value of page table entry to *ppte, and return 0, indicating success.
 *
 * Hint:
 *   We use a two-level pointer to store page table entry and return a state code to indicate
 *   whether this function succeeds or not.
 */
static int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte) { //这个函数会获取想要转换的虚拟地址对应的（二级）页表项地址，通过 pte 返回。
	Pde *pgdir_entryp;
	struct Page *pp;
	/* Step 1: Get the corresponding page directory entry. */
	/* Exercise 2.6: Your code here. (1/3) */
	pgdir_entryp = &pgdir[PDX(va)]; //pgdir是页目录的基地址，加上索引，得到对应的一级页目录的页表项,每个页表项前20代表PTBase，后12代表perm权限
	/* Step 2: If the corresponding page table is not existent (valid) then:
	 *   * If parameter `create` is set, create one. Set the permission bits 'PTE_C_CACHEABLE |
	 *     PTE_V' for this new page in the page directory. If failed to allocate a new page (out
	 *     of memory), return the error.
	 *   * Otherwise, assign NULL to '*ppte' and return 0.
	 */
	/* Exercise 2.6: Your code here. (2/3) */
	if((*pgdir_entryp & PTE_V)==0){ //对应的二级页表不存在
		if(create){ //不为0创建二级页表再查找页表项
			if(page_alloc(&pp)==0){ //还有空闲页
				pp->pp_ref++; //根据page_alloc的性质增加引用次数,其余不用管，都在page_alloc设置好了，此处只需要维护pp_ref
				*pgdir_entryp = page2pa(pp) | PTE_C_CACHEABLE | PTE_V;//看图说话，数据中需要有物理地址和权限，物理地址用page2pa获得PTBase(PA)
			}else{
				return -E_NO_MEM;//返回异常码，任务结束了
			}
		}else{ //create为0将*ppte设置为空指针
			*ppte = NULL;//利用函数传值不传地址的性质，设置外面的Pte *
			return 0;//ppte赋值完毕，任务结束了
		}
	}
	/* Step 3: Assign the kernel virtual address of the page table entry to '*ppte'. */
	/* Exercise 2.6: Your code here. (3/3) */
	//*ppte = (Pte *)KADDR(page2pa(pp)) + PTX(va);//不能用page2pa(pp)，因为如果存在的话，pp是没有分配的，还是个空
	*ppte = (Pte *)KADDR(PTE_ADDR(*pgdir_entryp)) + PTX(va);//PTE_ADDR取出前20位，即对应的page的物理地址,KADDR将其转为虚拟地址(看图说话)
	return 0;
}

/* Overview:
 *   Map the physical page 'pp' at virtual address 'va'. The permission (the low 12 bits) of the
 *   page table entry should be set to 'perm | PTE_C_CACHEABLE | PTE_V'.
 *
 * Post-Condition:
 *   Return 0 on success
 *   Return -E_NO_MEM, if page table couldn't be allocated
 *
 * Hint:
 *   If there is already a page mapped at `va`, call page_remove() to release this mapping.
 *   The `pp_ref` should be incremented if the insertion succeeds.
 */
int page_insert(Pde *pgdir, u_int asid, struct Page *pp, u_long va, u_int perm) {
	Pte *pte;

	/* Step 1: Get corresponding page table entry. */
	pgdir_walk(pgdir, va, 0, &pte);

	if (pte && (*pte & PTE_V)) {
		if (pa2page(*pte) != pp) {
			page_remove(pgdir, asid, va);
		} else {
			tlb_invalidate(asid, va);
			*pte = page2pa(pp) | perm | PTE_C_CACHEABLE | PTE_V; //看参考图，发现前20位是物理页号，所以用page2pa，而不是page2kva
			return 0;
		}
	}

	/* Step 2: Flush TLB with 'tlb_invalidate'. */
	/* Exercise 2.7: Your code here. (1/3) */
	tlb_invalidate(asid, va);
	/* Step 3: Re-get or create the page table entry. */
	/* If failed to create, return the error. */
	/* Exercise 2.7: Your code here. (2/3) */
	if(pgdir_walk(pgdir, va, 1, &pte)!=0){ //创建失败，直接返回错误码就行
		return -E_NO_MEM;
	} //否则pte已经被附上了正确的值
	/* Step 4: Insert the page to the page table entry with 'perm | PTE_C_CACHEABLE | PTE_V'
	 * and increase its 'pp_ref'. */
	/* Exercise 2.7: Your code here. (3/3) */
	pp->pp_ref++;//分清page_alloc的pp是申请的二级页表，这里的pp是物理页表，是不同的!
	*pte = page2pa(pp) | perm | PTE_C_CACHEABLE | PTE_V; 
	return 0;//一定要看懂给的参考图，分清地址和数据！什么时候是va，什么时候是pa，图都说的很清楚
}

/* Lab 2 Key Code "page_lookup" */
/*Overview:
    Look up the Page that virtual address `va` map to.
  Post-Condition:
    Return a pointer to corresponding Page, and store it's page table entry to *ppte.
    If `va` doesn't mapped to any Page, return NULL.*/
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte) {
	struct Page *pp;
	Pte *pte;

	/* Step 1: Get the page table entry. */
	pgdir_walk(pgdir, va, 0, &pte);

	/* Hint: Check if the page table entry doesn't exist or is not valid. */
	if (pte == NULL || (*pte & PTE_V) == 0) {
		return NULL;
	}

	/* Step 2: Get the corresponding Page struct. */
	/* Hint: Use function `pa2page`, defined in include/pmap.h . */
	pp = pa2page(*pte);
	if (ppte) {
		*ppte = pte;
	}

	return pp;
}
/* End of Key Code "page_lookup" */

/* Overview:
 *   Decrease the 'pp_ref' value of Page 'pp'.
 *   When there's no references (mapped virtual address) to this page, release it.
 */
void page_decref(struct Page *pp) {
	assert(pp->pp_ref > 0);

	/* If 'pp_ref' reaches to 0, free this page. */
	if (--pp->pp_ref == 0) {
		page_free(pp);
	}
}

/* Lab 2 Key Code "page_remove" */
// Overview:
//   Unmap the physical page at virtual address 'va'.
void page_remove(Pde *pgdir, u_int asid, u_long va) {
	Pte *pte;

	/* Step 1: Get the page table entry, and check if the page table entry is valid. */
	struct Page *pp = page_lookup(pgdir, va, &pte);
	if (pp == NULL) {
		return;
	}

	/* Step 2: Decrease reference count on 'pp'. */
	page_decref(pp);

	/* Step 3: Flush TLB. */
	*pte = 0;
	tlb_invalidate(asid, va);
	return;
}
/* End of Key Code "page_remove" */

void physical_memory_manage_check(void) {
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;
	int *temp;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	// now this page_free list must be empty!!!!
	LIST_INIT(&page_free_list);
	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	temp = (int *)page2kva(pp0);
	// write 1000 to pp0
	*temp = 1000;
	// free pp0
	page_free(pp0);
	printk("The number in address temp is %d\n", *temp);

	// alloc again
	assert(page_alloc(&pp0) == 0);
	assert(pp0);

	// pp0 should not change
	assert(temp == (int *)page2kva(pp0));
	// pp0 should be zero
	assert(*temp == 0);

	page_free_list = fl;
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);
	struct Page_list test_free;
	struct Page *test_pages;
	test_pages = (struct Page *)alloc(10 * sizeof(struct Page), PAGE_SIZE, 1);
	LIST_INIT(&test_free);
	// LIST_FIRST(&test_free) = &test_pages[0];
	int i, j = 0;
	struct Page *p, *q;
	for (i = 9; i >= 0; i--) {
		test_pages[i].pp_ref = i;
		// test_pages[i].pp_link=NULL;
		// printk("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
		LIST_INSERT_HEAD(&test_free, &test_pages[i], pp_link);
		// printk("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
	}
	p = LIST_FIRST(&test_free);
	int answer1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	assert(p != NULL);
	while (p != NULL) {
		// printk("%d %d\n",p->pp_ref,answer1[j]);
		assert(p->pp_ref == answer1[j++]);
		// printk("ptr: 0x%x v: %d\n",(p->pp_link).le_next,((p->pp_link).le_next)->pp_ref);
		p = LIST_NEXT(p, pp_link);
	}
	// insert_after test
	int answer2[] = {0, 1, 2, 3, 4, 20, 5, 6, 7, 8, 9};
	q = (struct Page *)alloc(sizeof(struct Page), PAGE_SIZE, 1);
	q->pp_ref = 20;

	// printk("---%d\n",test_pages[4].pp_ref);
	LIST_INSERT_AFTER(&test_pages[4], q, pp_link);
	// printk("---%d\n",LIST_NEXT(&test_pages[4],pp_link)->pp_ref);
	p = LIST_FIRST(&test_free);
	j = 0;
	// printk("into test\n");
	while (p != NULL) {
		//      printk("%d %d\n",p->pp_ref,answer2[j]);
		assert(p->pp_ref == answer2[j++]);
		p = LIST_NEXT(p, pp_link);
	}

	printk("physical_memory_manage_check() succeeded\n");
}

void page_check(void) {
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;

	// should be able to allocate a page for directory
	assert(page_alloc(&pp) == 0);
	Pde *boot_pgdir = (Pde *)page2kva(pp);

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	// now this page_free list must be empty!!!!
	LIST_INIT(&page_free_list);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// there is no free memory, so we can't allocate a page table
	assert(page_insert(boot_pgdir, 0, pp1, 0x0, 0) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	assert(page_insert(boot_pgdir, 0, pp1, 0x0, 0) == 0);
	assert(PTE_FLAGS(boot_pgdir[0]) == (PTE_C_CACHEABLE | PTE_V));
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	assert(PTE_FLAGS(*(Pte *)page2kva(pp0)) == (PTE_C_CACHEABLE | PTE_V));

	printk("va2pa(boot_pgdir, 0x0) is %x\n", va2pa(boot_pgdir, 0x0));
	printk("page2pa(pp1) is %x\n", page2pa(pp1));
	//  printk("pp1->pp_ref is %d\n",pp1->pp_ref);
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(pp1->pp_ref == 1);

	// should be able to map pp2 at PAGE_SIZE because pp0 is already allocated for page table
	assert(page_insert(boot_pgdir, 0, pp2, PAGE_SIZE, 0) == 0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	printk("start page_insert\n");
	// should be able to map pp2 at PAGE_SIZE because it's already there
	assert(page_insert(boot_pgdir, 0, pp2, PAGE_SIZE, 0) == 0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in page_insert
	assert(page_alloc(&pp) == -E_NO_MEM);

	// should not be able to map at PDMAP because need free page for page table
	assert(page_insert(boot_pgdir, 0, pp0, PDMAP, 0) < 0);

	// insert pp1 at PAGE_SIZE (replacing pp2)
	assert(page_insert(boot_pgdir, 0, pp1, PAGE_SIZE, 0) == 0);

	// should have pp1 at both 0 and PAGE_SIZE, pp2 nowhere, ...
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	printk("pp2->pp_ref %d\n", pp2->pp_ref);
	assert(pp2->pp_ref == 0);
	printk("end page_insert\n");

	// pp2 should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at PAGE_SIZE
	page_remove(boot_pgdir, 0, 0x0);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// unmapping pp1 at PAGE_SIZE should free it
	page_remove(boot_pgdir, 0, PAGE_SIZE);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// forcibly take pp0 back
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	boot_pgdir[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);
	page_free(pa2page(PADDR(boot_pgdir)));

	printk("page_check() succeeded!\n");
}

u_int page_filter(Pde *pgdir, u_long va_lower_limit, u_long va_upper_limit, u_int num){
	u_int count = 0;
	Pde * pde;
	Pte * pte;
	u_long va = va_lower_limit;
	for(;va<va_upper_limit;va = va + 4096){
		pde = pgdir + PDX(va);
		if(*pde & PTE_V){
			pte = (Pte*)KADDR(PTE_ADDR(*pde))+ PTX(va);
			if(*pte & PTE_V){
				struct Page * pp = page_lookup(pgdir,va,&pte);
				if(pp->pp_ref>=num){
					count++;
				}
			}			
		}
	}
	return count;
}

//以下内容为2023lab2-extra
/*#include <swap.h> 
struct Page_list page_free_swapable_list;
static u_char *disk_alloc();
static void disk_free(u_char *pdisk);
void swap_init()
{
    LIST_INIT(&page_free_swapable_list);
    for (int i = SWAP_PAGE_BASE; i < SWAP_PAGE_END; i += BY2PG)
    {
        struct Page *pp = pa2page(i);
        LIST_REMOVE(pp, pp_link);
        LIST_INSERT_HEAD(&page_free_swapable_list, pp, pp_link);
    }
}
// Interface for 'Passive Swap Out'
struct Page *swap_alloc(Pde *pgdir, u_int asid) {
	// Step 1: Ensure free page
	if (LIST_EMPTY(&page_free_swapable_list)) {
		u_char  *disk_swap = disk_alloc();
		u_long da = (u_long)disk_swap;
		struct Page *p = pa2page(SWAP_PAGE_BASE);//这里策略是只换0x3900000处的页
		for (u_long i = 0; i < 1024; i++) { //改变所有页表中映射到0x3900000的页表项
			Pde *pde = pgdir + i;
			if ((*pde) & PTE_V) {
				for (u_long j = 0; j < 1024; j++) {
					Pte *pte = (Pte*)KADDR(PTE_ADDR(*pde)) + j;
					if (((*pte) & PTE_V) && (PTE_ADDR(*pte) == SWAP_PAGE_BASE) ) {
						(*pte) = ((da / BY2PG) << 12) | ((*pte) & 0xfff);
						//上式作用等价于(*pte) = PTE_ADDR(da) | ((*pte) & 0xfff);保留后面的所有12位offset
						(*pte) = ((*pte) | PTE_SWP)  & (~PTE_V);
						tlb_invalidate(asid, (i << 22) | (j << 12) ); //tlb_invalidate(asid, va);
					}
				}
			}
		}
		memcpy((void *)da, (void *)page2kva(p), BY2PG);
		//这里没有再删掉页控制块p对应的内容，是因为跳出该if之后，会执行剩下的语句，其中倒数第2句会帮助情空
		LIST_INSERT_HEAD(&page_free_swapable_list, p, pp_link);
	}

	// Step 2: Get a free page and clear it
	struct Page *pp = LIST_FIRST(&page_free_swapable_list);
	LIST_REMOVE(pp, pp_link);
	memset((void *)page2kva(pp), 0, BY2PG);

	return pp;
}

// Interfaces for 'Active Swap In'
static int is_swapped(Pde *pgdir, u_long va) {
	Pde *pde = pgdir + PDX(va);
	if (*pde & PTE_V) {
		Pte *pte = (Pte*)KADDR(PTE_ADDR(*pde)) + PTX(va);
		if ((*pte & PTE_SWP) && !(*pte & PTE_V)) {
			return 1;
		}
	}
	return 0;
}

static void swap(Pde *pgdir, u_int asid, u_long va) {
	struct Page *pp = swap_alloc(pgdir, asid); //可用于交换的内存块的页控制块
	u_long da = PTE_ADDR(*((Pte*)KADDR(PTE_ADDR(*(pgdir + PDX(va)))) + PTX(va))); //外存地址
	memcpy((void *)page2kva(pp), (void *)da, BY2PG);

	for (u_long i = 0; i < 1024; i++) { //所有页表项记录的映射在外存的页表项改到新申请下来的swap的内存地址
		Pde *pde = pgdir + i;
		if (*pde & PTE_V) {
			for (u_long j = 0; j < 1024; j++) {
				Pte *pte = (Pte*)KADDR(PTE_ADDR(*pde)) + j;
				if (!(*pte & PTE_V) && (*pte & PTE_SWP) && (PTE_ADDR(*pte) == da)) {
					//以下三句话含义均可类比于 swap_alloc
					(*pte) = ((page2pa(pp) / BY2PG) << 12) | ((*pte) & 0xfff);
					(*pte) = ((*pte) & ~PTE_SWP) | PTE_V;
					tlb_invalidate(asid, (i << 22) | (j << 12) );
				}
			}
		}
	}
	disk_free((u_char *)da);
	return;
}*/
