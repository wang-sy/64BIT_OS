#include "interrupt.h"
#include "linkage.h"
#include "lib.h"
#include "printk.h"
#include "memory.h"
#include "gate.h"

Build_IRQ(0x20)
Build_IRQ(0x21)
Build_IRQ(0x22)
Build_IRQ(0x23)
Build_IRQ(0x24)
Build_IRQ(0x25)
Build_IRQ(0x26)
Build_IRQ(0x27)
Build_IRQ(0x28)
Build_IRQ(0x29)
Build_IRQ(0x2a)
Build_IRQ(0x2b)
Build_IRQ(0x2c)
Build_IRQ(0x2d)
Build_IRQ(0x2e)
Build_IRQ(0x2f)
Build_IRQ(0x30)
Build_IRQ(0x31)
Build_IRQ(0x32)
Build_IRQ(0x33)
Build_IRQ(0x34)
Build_IRQ(0x35)
Build_IRQ(0x36)
Build_IRQ(0x37)

// 定义并初始化中断处理函数表
void (* interrupt[24])(void) = {
	IRQ0x20_interrupt,IRQ0x21_interrupt,IRQ0x22_interrupt,
    IRQ0x23_interrupt,IRQ0x24_interrupt,IRQ0x25_interrupt,
    IRQ0x26_interrupt,IRQ0x27_interrupt,IRQ0x28_interrupt,
    IRQ0x29_interrupt,IRQ0x2a_interrupt,IRQ0x2b_interrupt,
    IRQ0x2c_interrupt,IRQ0x2d_interrupt,IRQ0x2e_interrupt,
    IRQ0x2f_interrupt,IRQ0x30_interrupt,IRQ0x31_interrupt,
    IRQ0x32_interrupt,IRQ0x33_interrupt,IRQ0x34_interrupt,
    IRQ0x35_interrupt,IRQ0x36_interrupt,IRQ0x37_interrupt,
};

/**
 * 初始化中断处理程序的地址
 */
void init_interrupt(){

    // 装载 IDT Table
    for(int i = 32;i < 56;i++)
        set_intr_gate(i , 2 , interrupt[i - 32]);

    printk("8259A init \n");

    // 设置寄存器
    //8259A-master    ICW1-4
    io_out8(0x20,0x11);
    io_out8(0x21,0x20);
    io_out8(0x21,0x04);
    io_out8(0x21,0x01);

    //8259A-slave    ICW1-4
    io_out8(0xa0,0x11);
    io_out8(0xa1,0x28);
    io_out8(0xa1,0x02);
    io_out8(0xa1,0x01);

    //8259A-M/S    OCW1
    io_out8(0x21,0x00);
    io_out8(0xa1,0x00);

    sti();
}


// 中断处理程序do_IRQ
void do_IRQ(unsigned long regs,unsigned long nr) { 
	printk("do_IRQ:%#08x\t",nr); // 中断处理（直接输出）
	io_out8(0x20,0x20); // 向主芯片返回信息
}