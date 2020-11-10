#ifndef _64BITOS_SRC_KERNEL_GATE_H
#define _64BITOS_SRC_KERNEL_GATE_H

/* =========================================================================
 =                                   宏的定义                                =
 =========================================================================*/

/**
 * 将TSS段描述符的段选择子的首地址加载到TR寄存器
 * @param n 读取的表项的数量
 * @author 345538255@qq.com
 */
#define LOAD_TR(n) 							\
do{									\
	__asm__ __volatile__(	"ltr	%%ax"				\
				:					\
				:"a"(n << 3)				\
				:"memory");				\
}while(0)

/**
 * 用于向IDT表中添加门，以达到对不同中断向量号的中断进行分别处理的效果
 * @param gate_selector_addr 向中断向量号为 gate_selector_addr 的表项添加门级描述符
 * @param attr 用于填充门描述符的[40, 47]位，从左到右依次：
 * 指定调用门描述符是否有效(P-1bit), 用于判断描述符的权限(DPL-2bit),  标识门的类别等(剩余5bit)
 * @param ist  用于填充 TSS 中 IST 项
 * @param code_addr 中断处理例程的入口地址
 * @author 345538255@qq.com
 */
#define SET_GATE(gate_selector_addr,attr,ist,code_addr)	\
do								\
{	unsigned long __d0,__d1;				\
	__asm__ __volatile__	(	"movw	%%dx,	%%ax	\n\t"	\
					"andq	$0x7,	%%rcx	\n\t"	\
					"addq	%4,	%%rcx	\n\t"	\
					"shlq	$32,	%%rcx	\n\t"	\
					"addq	%%rcx,	%%rax	\n\t"	\
					"xorq	%%rcx,	%%rcx	\n\t"	\
					"movl	%%edx,	%%ecx	\n\t"	\
					"shrq	$16,	%%rcx	\n\t"	\
					"shlq	$48,	%%rcx	\n\t"	\
					"addq	%%rcx,	%%rax	\n\t"	\
					"movq	%%rax,	%0	\n\t"	\
					"shrq	$32,	%%rdx	\n\t"	\
					"movq	%%rdx,	%1	\n\t"	\
					:"=m"(*((unsigned long *)(gate_selector_addr)))	,					\
					 "=m"(*(1 + (unsigned long *)(gate_selector_addr))),"=&a"(__d0),"=&d"(__d1)		\
					:"i"(attr << 8),									\
					 "3"((unsigned long *)(code_addr)),"2"(0x8 << 16),"c"(ist)				\
					:"memory"		\
				);				\
}while(0)

/* =========================================================================
 =                                   数据声明                                =
 =========================================================================*/

/**
 * GDT表中的描述子，每一个描述子占64位
 * 其中的每一个char类型占了8位
 */
struct GDTDescribeStruct {
	unsigned char a_byte[8];
};
// 使用extern引用在head.S中定义的GDT表，即全局描述表
extern struct GDTDescribeStruct gdt_table[];

/**
 * IDT表中的门级描述子，每一个描述子占128位
 * 其中的每一个char类型占了8位
 */
struct IDTGateStruct {
	unsigned char a_byte[16];
};
// 使用extern引用在head.S中定义的IDT表，即中断描述表
extern struct IDTGateStruct idt_table[];

// 使用extern引用在head.S中定义的TSS表
extern unsigned int tss64_table[26];

/* =========================================================================
 =                                   函数声明                                =
 =========================================================================*/

/**
 * 创建一个中断门， 其权限等级为0(内核态)
 * @param n 中断向量号
 * @param ist 用于填充 TSS 中 IST 项
 * @param addr 相应中断号异常处理函数入口地址
 */
void SetInterruptGate(unsigned int n,unsigned char ist,void * addr);

/**
 * 创建一个陷阱门， 其权限等级为0(内核态)
 * @param n 中断向量号
 * @param ist 用于填充 TSS 中 IST 项
 * @param addr 相应中断号异常处理函数入口地址
 */
void SetTrapGate(unsigned int n,unsigned char ist,void * addr);

/**
 * 创建一个系统门， 其权限等级为3(用户态)
 * @param n 中断向量号
 * @param ist 用于填充 TSS 中 IST 项
 * @param addr 相应中断号异常处理函数入口地址
 */
void SetSystemGate(unsigned int n,unsigned char ist,void * addr);

/**
 * 配置TSS段内的各个RSP和IST项， 值得注意的是： 这里配置的是IA-32e模式下的TSS
 * 该模式下的TSS与保护模式下的TSS有巨大的不同， 仅由RSP0~2、IST1~7 这十个六十四位数构成
 */
void SetTss64(unsigned long rsp0, unsigned long rsp1, unsigned long rsp2, unsigned long ist1,
               unsigned long ist2, unsigned long ist3, unsigned long ist4, unsigned long ist5,
               unsigned long ist6, unsigned long ist7);


#endif