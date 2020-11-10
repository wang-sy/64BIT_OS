#ifndef _64BITOS_SRC_KERNEL_INTERRUPT_H
#define _64BITOS_SRC_KERNEL_INTERRUPT_H

/* =========================================================================
 =                                   引入头                                 =
 =========================================================================*/

#include "linkage.h"

/* =========================================================================
 =                                   宏的定义                                =
 =========================================================================*/

/**
 * <h3>保存中断发生时的现场</h3>
 */
#define SAVE_ALL				\
	"cld;			\n\t"		\
	"pushq	%rax;		\n\t"		\
	"pushq	%rax;		\n\t"		\
	"movq	%es,	%rax;	\n\t"		\
	"pushq	%rax;		\n\t"		\
	"movq	%ds,	%rax;	\n\t"		\
	"pushq	%rax;		\n\t"		\
	"xorq	%rax,	%rax;	\n\t"		\
	"pushq	%rbp;		\n\t"		\
	"pushq	%rdi;		\n\t"		\
	"pushq	%rsi;		\n\t"		\
	"pushq	%rdx;		\n\t"		\
	"pushq	%rcx;		\n\t"		\
	"pushq	%rbx;		\n\t"		\
	"pushq	%r8;		\n\t"		\
	"pushq	%r9;		\n\t"		\
	"pushq	%r10;		\n\t"		\
	"pushq	%r11;		\n\t"		\
	"pushq	%r12;		\n\t"		\
	"pushq	%r13;		\n\t"		\
	"pushq	%r14;		\n\t"		\
	"pushq	%r15;		\n\t"		\
	"movq	$0x10,	%rdx;	\n\t"		\
	"movq	%rdx,	%ds;	\n\t"		\
	"movq	%rdx,	%es;	\n\t"

/**
 * <h3>IRQ_NAME宏的辅助宏，用于在给出的nr后方加上Interrupt(void)</h3>
 * @param nr 给出的前缀信息
 *
 * <b>样例：</b>假设给出的 nr 为 IRQ0x20，那么返回 IRQ0x20Interrupt(void)
 */
#define IRQ_NAME2(nr) nr##Interrupt(void)

/**
 * <h3>用于生成中断处理函数入口函数的函数名， 与IRQ_NAME2配合使用</h3>
 * @param nr 输入的中断向量号
 *
 * <b>样例：</b>假设给出的 nr 为 0x20，那么返回 IRQ0x20Interrupt(void)
 * <p><b>原理：</b></p>
 * <ul>
 *     <li>使用##符号将输入的0x20与IRQ拼接，生成<b>IRQ0x20</b></li>
 *     <li>送入IRQ_NAME2进行进一步拼接，获取最终结果</li>
 * </ul>
 */
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##nr)

/**
 * <h3>用于生成中断处理入口函数的宏，给出中断向量号，直接生成相应的入口函数</h3>
 * @param nr 中断向量号
 *
 * <p><b>原理：</b></p>
 * <ul>
 *     <li>使用void IRQ_NAME(nr)生成函数名，相当于函数的声明</b></li>
 *     <li>使用__asm__内联汇编生成函数的实现</li>
 * </ul>
 * <p><b>生成内容：</b></p>
 * <ul>
 *     <li>生成的是名为<b>IRQ0nrInterrupt(void)</b>一个函数</li>
 *     <li>调用DoIRQ前，先使用SAVE_ALL宏保存调用现场</li>
 *     <li>该函数是一个中断处理函数的入口函数，会调用DoIRQ函数</li>
 *     <li>结束DoIRQ函数执行完毕后，会执行ResetFromInterrupt来回复现场</li>
 * </ul>
 * 所有基于此方法生成的函数最终都会跳转到DoIRQ函数进行处理，
 */
#define BUILD_IRQ(nr)							\
void IRQ_NAME(nr);						\
__asm__ (	SYMBOL_NAME_STR(IRQ)#nr"Interrupt:		\n\t"	\
			"pushq	$0x00				\n\t"	\
			SAVE_ALL					\
			"movq	%rsp,	%rdi			\n\t"	\
			"leaq	ResetFromInterrupt(%rip),	%rax	\n\t"	\
			"pushq	%rax				\n\t"	\
			"movq	$"#nr",	%rsi			\n\t"	\
			"jmp	DoIRQ	\n\t");

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
