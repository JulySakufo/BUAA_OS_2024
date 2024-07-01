#ifndef _KCLOCK_H_
#define _KCLOCK_H_

#include <asm/asm.h>

#define TIMER_INTERVAL (500000) // WARNING: DO NOT MODIFY THIS LINE!

// clang-format off
.macro RESET_KCLOCK //RESET_KCLOCK 宏完成了对 CP0 中 Timer 的配置
	li 	t0, TIMER_INTERVAL
	/*
	 * Hint:
	 *   Use 'mtc0' to write an appropriate value into the CP0_COUNT and CP0_COMPARE registers.
	 *   Writing to the CP0_COMPARE register will clear the timer interrupt.
	 *   The CP0_COUNT register increments at a fixed frequency. When the values of CP0_COUNT and
	 *   CP0_COMPARE registers are equal, the timer interrupt will be triggered.
	 *
	 */
	/* Exercise 3.11: Your code here. */
	mtc0	t0, CP0_COMPARE //将 Compare 寄存器配置为我们所期望的计时器周期
	mtc0	zero, CP0_COUNT //将 Count 寄存器清零 (这就对 Timer 完成了配置)

.endm
// clang-format on
#endif
