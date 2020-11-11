#ifndef _64BITOS_SRC_KERNEL_INTERRUPT_H
#define _64BITOS_SRC_KERNEL_INTERRUPT_H

/* =========================================================================
 =                                   函数声明                                =
 =========================================================================*/

/**
 * 初始化主/从8259A中断控制器以及中断描述符表IDT内的各门描述符
 */
void InitInterrupt();

/**
 * 键盘中断的处理函数， 会被BUILD_IRQ宏定义出的函数调用
 * @param regs 栈基址
 * @param nr 中断向量号
 */
void DoIRQ(unsigned long regs,unsigned long nr);

#endif
