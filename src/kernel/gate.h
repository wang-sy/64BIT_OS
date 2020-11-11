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