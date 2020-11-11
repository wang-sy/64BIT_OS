/**
 * MIT License
 *
 * Copyright (c) 2020 王赛宇

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @author wangsaiyu@cqu.edu.cn
 * 本文件内容是对interrupt.h中声明的函数的实现，完成了：
 * <ul>
 *      <li>基于interrupt.h中BUILD_IRQ宏，对0x20~0x37中断进行定义</li>
 *      <li>基于InitInterrupt完成对8259A芯片参数的初始化以及对中断门的装载</li>
 *      <li>DoIRQ函数，实现对IRQ中断的处理</li>
 * </ul>
 */

/* =========================================================================
 =                                 头的引入                                  =
 =========================================================================*/

#include "interrupt.h"
#include "linkage.h"
#include "lib.h"
#include "printk.h"
#include "gate.h"

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
 =                            函数的声明与定义                                =
 =========================================================================*/

// 使用BUILD_IRQ宏，对IRQ中断进行声明
BUILD_IRQ(0x20)
BUILD_IRQ(0x21)
BUILD_IRQ(0x22)
BUILD_IRQ(0x23)
BUILD_IRQ(0x24)
BUILD_IRQ(0x25)
BUILD_IRQ(0x26)
BUILD_IRQ(0x27)
BUILD_IRQ(0x28)
BUILD_IRQ(0x29)
BUILD_IRQ(0x2a)
BUILD_IRQ(0x2b)
BUILD_IRQ(0x2c)
BUILD_IRQ(0x2d)
BUILD_IRQ(0x2e)
BUILD_IRQ(0x2f)
BUILD_IRQ(0x30)
BUILD_IRQ(0x31)
BUILD_IRQ(0x32)
BUILD_IRQ(0x33)
BUILD_IRQ(0x34)
BUILD_IRQ(0x35)
BUILD_IRQ(0x36)
BUILD_IRQ(0x37)

/*定义并初始化中断处理函数表
其中的函数名是由上方的BUILD_IRQ宏中宏定义而成的*/
void (* interrupt[24])(void) = {
	IRQ0x20Interrupt, IRQ0x21Interrupt, IRQ0x22Interrupt,
    IRQ0x23Interrupt, IRQ0x24Interrupt, IRQ0x25Interrupt,
    IRQ0x26Interrupt, IRQ0x27Interrupt, IRQ0x28Interrupt,
    IRQ0x29Interrupt, IRQ0x2aInterrupt, IRQ0x2bInterrupt,
    IRQ0x2cInterrupt, IRQ0x2dInterrupt, IRQ0x2eInterrupt,
    IRQ0x2fInterrupt, IRQ0x30Interrupt, IRQ0x31Interrupt,
    IRQ0x32Interrupt, IRQ0x33Interrupt, IRQ0x34Interrupt,
    IRQ0x35Interrupt, IRQ0x36Interrupt, IRQ0x37Interrupt,
};

/* =========================================================================
 =                       interrupt.h中函数的实现                              =
 =========================================================================*/

/*用于初始化IRQ中断
该函数执行了三方面的操作：
一、将上方定义的函数装载到了IDT中，用于中断处理
二、配置主从芯片中的参数
三、禁止芯片发出时钟中断（要不然就会一直输出时钟终端信息）*/
void InitInterrupt(){

    // 装载 IDT Table
    for(int i = 32; i < 56; ++i) {
        SetInterruptGate(i , 2 , interrupt[i - 32]);
    }

    // 配置主芯片中的参数
    io_out8(0x20,0x11);
    io_out8(0x21,0x20);
    io_out8(0x21,0x04);
    io_out8(0x21,0x01);

    // 配置从芯片中的参数
    io_out8(0xa0,0x11);
    io_out8(0xa1,0x28);
    io_out8(0xa1,0x02);
    io_out8(0xa1,0x01);

    // 屏蔽时钟中断
	io_out8(0x21,0xfd);
	io_out8(0xa1,0xff);

	// 加载变化
    sti();
}


/*IRQ中断处理函数
首先输出作为参数传入的中断向量号，随后通过IN从I/O端口地址60h处读取出键盘扫描码并进行输出，最后对相应缓存进行清空
若不清空缓存，无法接收到后续来自键盘的中断信号*/
void DoIRQ(unsigned long regs,unsigned long nr) {
    printk("DoIRQ:%#08x\t",nr); // 中断向量号
    unsigned char x = io_in8(0x60); // 请求键盘扫描码
    printk("key code:%#08x\n",x);
    io_out8(0x20,0x20); // 清空缓存
}