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

#include "interrupt.h"
#include "linkage.h"
#include "lib.h"
#include "printk.h"
#include "gate.h"

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