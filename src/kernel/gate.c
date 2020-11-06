#include "gate.h"

/**
 * 创建陷阱门
 * @param n 对应的IDT表的条目号
 * @param ist 用于填充TSS
 * @param addr 跳转的目标地址，需要传递一个函数指针
 */ 
void set_intr_gate(unsigned int n,unsigned char ist,void * addr) 
{
	_set_gate(IDT_Table + n , 0x8E , ist , addr);	//P,DPL=0,TYPE=E
}

/**
 * 创建一个陷阱门
 * @param n 对应的IDT表的条目号
 * @param ist 用于填充TSS
 * @param addr 跳转的目标地址，需要传递一个函数指针
 */ 
void set_trap_gate(unsigned int n,unsigned char ist,void * addr)
{
	_set_gate(IDT_Table + n , 0x8F , ist , addr);	//P,DPL=0,TYPE=F
}

/**
 * 创建一个DPL为3的陷阱门
 * @param n 对应的IDT表的条目号
 * @param ist 用于填充TSS
 * @param addr 跳转的目标地址，需要传递一个函数指针
 */ 
void set_system_gate(unsigned int n,unsigned char ist,void * addr)
{
	_set_gate(IDT_Table + n , 0xEF , ist , addr);	//P,DPL=3,TYPE=F
}

/**
 * 创建一个DPL是0的中断门
 * @param n 对应的IDT表的条目号
 * @param ist 用于填充TSS
 * @param addr 跳转的目标地址，需要传递一个函数指针
 */ 
void set_system_intr_gate(unsigned int n,unsigned char ist,void * addr)	//int3
{
	_set_gate(IDT_Table + n , 0xEE , ist , addr);	//P,DPL=3,TYPE=E
}


/**
 * 重置Tss
 */
void set_tss64(unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,
unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7)
{
	*(unsigned long *)(TSS64_Table+1) = rsp0;
	*(unsigned long *)(TSS64_Table+3) = rsp1;
	*(unsigned long *)(TSS64_Table+5) = rsp2;

	*(unsigned long *)(TSS64_Table+9) = ist1;
	*(unsigned long *)(TSS64_Table+11) = ist2;
	*(unsigned long *)(TSS64_Table+13) = ist3;
	*(unsigned long *)(TSS64_Table+15) = ist4;
	*(unsigned long *)(TSS64_Table+17) = ist5;
	*(unsigned long *)(TSS64_Table+19) = ist6;
	*(unsigned long *)(TSS64_Table+21) = ist7;	
}
