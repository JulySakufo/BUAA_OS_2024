#include <asm/asm.h>

LEAF(msyscall)
	// Just use 'syscall' instruction and return.

	/* Exercise 4.1: Your code here. */
	syscall
	jr ra /*系统调用后返回,epc在内核的系统调用处理程序里增加了*/
END(msyscall)
