#ifndef _64BITOS_SRC_KERNEL_TASK_H
#define _64BITOS_SRC_KERNEL_TASK_H

/* =========================================================================
 =                                   头的引用                               =
 =========================================================================*/

#include "memory.h"
#include "cpu.h"
#include "lib.h"

/* =========================================================================
 =                               数据型宏定义                                =
 =========================================================================*/

// stack size 32K
#define STACK_SIZE 32768

// 偏移量的定义
#define KERNEL_CS 	(0x08)
#define	KERNEL_DS 	(0x10)
#define	USER_CS		(0x28)
#define USER_DS		(0x30)

// 标记进程状态
#define TASK_RUNNING		(1 << 0)
#define TASK_INTERRUPTIBLE	(1 << 1)
#define	TASK_UNINTERRUPTIBLE	(1 << 2)
#define	TASK_ZOMBIE		(1 << 3)	
#define	TASK_STOPPED		(1 << 4)

#define PF_KTHREAD	(1 << 0)

#define CLONE_FS	(1 << 0)
#define CLONE_FILES	(1 << 1)
#define CLONE_SIGNAL	(1 << 2)

/* =========================================================================
 =                                 宏定义函数                                =
 =========================================================================*/

/**
 * 获取当前执行进程的联合体指针
 * @return 一个指针，指向当前运行的进程的联合体的首地址
 *
 * <p> 原理： 我们定义的是联合体，所以使用的栈和当前进程的描述子在同一片内存，同时我们定义的时候使用了32KB对齐，
 * 所以我们找到当前的栈顶，对当前栈顶地址进行32KB对齐后的地址就是联合体的首地址，同时由于我们定义的顺序，
 * Task描述子结构体在该联合体的首部，所以获取的联合体的首地址就是该Task描述子的首地址 </p>
 */
struct TaskStruct* get_current();

/**
 * 通过宏定义的方法获取当前正在运行的进程地址
 *
 * <p> 原理： 我们定义的是联合体，所以使用的栈和当前进程的描述子在同一片内存，同时我们定义的时候使用了32KB对齐，
 * 所以我们找到当前的栈顶，对当前栈顶地址进行32KB对齐后的地址就是联合体的首地址，同时由于我们定义的顺序，
 * Task描述子结构体在该联合体的首部，所以获取的联合体的首地址就是该Task描述子的首地址 </p>
 */
#define GET_CURRENT                    \
    "movq    %rsp,    %rbx    \n\t"    \
    "andq    $-32768,%rbx     \n\t"

/**
 * 定义的宏， 获取当前正在运行的进程地址， 直接调用前方定义的get_current函数
 *
 * <p> 原理： 我们定义的是联合体，所以使用的栈和当前进程的描述子在同一片内存，同时我们定义的时候使用了32KB对齐，
 * 所以我们找到当前的栈顶，对当前栈顶地址进行32KB对齐后的地址就是联合体的首地址，同时由于我们定义的顺序，
 * Task描述子结构体在该联合体的首部，所以获取的联合体的首地址就是该Task描述子的首地址 </p>
 */
#define current (get_current())


/**
 * 宏定义，完成从一个进程到另一个进程的切换
 * @param prev 从prev进程开始切换
 * @param next 切换到next进程
 */
#define switch_to(prev,next)			\
do{							\
	__asm__ __volatile__ (	"pushq	%%rbp	\n\t"	\
				"pushq	%%rax	\n\t"	\
				"movq	%%rsp,	%0	\n\t"	\
				"movq	%2,	%%rsp	\n\t"	\
				"leaq	1f(%%rip),	%%rax	\n\t"	\
				"movq	%%rax,	%1	\n\t"	\
				"pushq	%3		\n\t"	\
				"jmp	__switch_to	\n\t"	\
				"1:	\n\t"	\
				"popq	%%rax	\n\t"	\
				"popq	%%rbp	\n\t"	\
				:"=m"(prev->thread->rsp),"=m"(prev->thread->rip)		\
				:"m"(next->thread->rsp),"m"(next->thread->rip),"D"(prev),"S"(next)	\
				:"memory"		\
				);			\
}while(0)

/**
 * 通过某个结构体实例的某个成员的地址， 获取整个结构体的地址
 *
 * 原理：计算结构体实例成员的地址， 减掉该成员相对结构体头部的偏移量就是该结构体的起始地址
 */
#define CONTAINER_OF(ptr,type,member)                            \
({    \
    typeof(((type *)0)->member) * p = (ptr);                    \
    (type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));        \
})

/* =========================================================================
 =                                  数据引用                                 =
 =========================================================================*/

extern char _text;
extern char _etext;
extern char _data;
extern char _edata;
extern char _rodata;
extern char _erodata;
extern char _bss;
extern char _ebss;
extern char _end;

extern unsigned long _stack_start;

/* =========================================================================
 =                               结构体定义                                  =
 =========================================================================*/

/**
 * 用于描述一个进程的内存空间的结构体
 * @param pgd 内存页表指针(页目录基地址与页表属性的组合值)
 * @param start_code 代码段起始地址
 * @param end_code 代码段结束地址
 * @param start_data 数据段起始地址
 * @param end_data 数据段结束地址
 * @param start_rodata 只读数据段起始地址
 * @param end_rodata 只读数据段结束地址
 * @param start_brk 堆区起始地址
 * @param end_brk 堆区结束地址
 * @param start_stack 应用层栈基址
 * 
 */
struct MemoryStruct {
    pml4t_t *pgd;// 内存页表指针(页目录基地址与页表属性的组合值)

    unsigned long start_code,end_code; // 代码段起始地址 代码段结束地址
    unsigned long start_data,end_data; // 数据段起始地址 数据段结束地址
    unsigned long start_rodata,end_rodata; // 只读数据段起始地址 只读数据段结束地址
    unsigned long start_brk,end_brk; // 堆区起始地址 堆区结束地址
    unsigned long start_stack; // 应用层栈基址
};

/**
 * 用于保存进程调度、切换的现场
 * @param rsp0 内核层栈基地址
 * @param rip 内核层代码指针
 * @param rsp 内核层当前栈指针
 * @param fs 相应寄存器
 * @param gs  相应寄存器
 * @param cr2 相应寄存器
 * @param trap_nr 异常号
 * @param error_code 错误码
 */
struct thread_struct {
    unsigned long rsp0; // 内核层栈基地址

    unsigned long rip; // 内核层代码指针
    unsigned long rsp; // 内核层当前栈指针

    unsigned long fs; // 相应寄存器
    unsigned long gs; // 相应寄存器

    unsigned long cr2; // 相应寄存器
    unsigned long trap_nr; // 异常号
    unsigned long error_code; // 错误码
};

/**
 * 描述进程的资源使用情况的结构体
 * @param list 	双向链表，用于连接各个进程控制结构体
 * @param state 用于记录进程的状态，随时可能改变，所以使用volatile关键字声明
 * @param flags 进程标志，表示（线程、进程、内核线程）
 * @param mm 内存空间分布描述结构体，用来描述程序段信息、内存页表等
 * @param thread 进程切换时使得保留信息
 * @param addr_limit 进程地址空间范围 0x00000000,00000000 - 0x00007FFF,FFFFFFFF为应用程，0xffff,8000,0000,0000 - 0xffff,ffff,ffff,ffff 为内核层
 * @param pid 进程id
 * @param counter 进程可用时间片
 * @param signal 进程的信号量
 * @param priority 进程的优先级
 */
struct TaskStruct {
    struct List list; // 双向链表，用于连接各个进程控制结构体
    volatile long state; // 用于记录进程的状态，随时可能改变，所以使用volatile关键字声明
    unsigned long flags; // 进程标志，表示（线程、进程、内核线程）

    struct MemoryStruct *mm; // 内存空间分布描述结构体，用来描述程序段信息、内存页表等
    struct thread_struct *thread; // 进程切换时使得保留信息

    /*0x0000,0000,0000,0000 - 0x0000,7fff,ffff,ffff 用户层*/
    /*0xffff,8000,0000,0000 - 0xffff,ffff,ffff,ffff 内核层*/
    unsigned long addr_limit;  // 进程地址空间范围

    long pid; // 进程id
    long counter; // 进程可用时间片
    long signal; // 进程的信号量
    long priority; // 进程的优先级
};

/**
 * task_struct和内核层栈区共用一段内存
 * task --> stack 两者公用 stack的空间
 * @param task 进程的描述子
 * @param stack 进程的栈空间
 */
union TaskUnion {
    struct TaskStruct task;  // 进程的描述子
    unsigned long stack[STACK_SIZE / sizeof(unsigned long)];  // 进程的栈空间
}__attribute__((aligned (8)));    //8 Bytes align


/**
 * 描述一个TSS
 */
struct TSSStruct{
	unsigned int  reserved0;
	unsigned long rsp0;
	unsigned long rsp1;
	unsigned long rsp2;
	unsigned long reserved1;
	unsigned long ist1;
	unsigned long ist2;
	unsigned long ist3;
	unsigned long ist4;
	unsigned long ist5;
	unsigned long ist6;
	unsigned long ist7;
	unsigned long reserved2;
	unsigned short reserved3;
	unsigned short iomapbaseaddr;
}__attribute__((packed));


/**
 * 记录进程切换时的所有寄存器状态
 */
struct PTRegs {
    unsigned long r15;  // r15寄存器的值， 一个长整数
    unsigned long r14;  // r14寄存器的值， 一个长整数
    unsigned long r13;  // r13寄存器的值， 一个长整数
    unsigned long r12;  // r12寄存器的值， 一个长整数
    unsigned long r11;  // r11寄存器的值， 一个长整数
    unsigned long r10;  // r10寄存器的值， 一个长整数
    unsigned long r9;  // r9寄存器的值， 一个长整数
    unsigned long r8;  // r8寄存器的值， 一个长整数
    unsigned long rbx;  // rbx寄存器的值， 一个长整数
    unsigned long rcx;  // rcx寄存器的值， 一个长整数
    unsigned long rdx;  // rdx寄存器的值， 一个长整数
    unsigned long rsi;  // rsi寄存器的值， 一个长整数
    unsigned long rdi;  // rdi寄存器的值， 一个长整数
    unsigned long rbp;  // rbp寄存器的值， 一个长整数
    unsigned long ds;  // ds寄存器的值， 一个长整数
    unsigned long es;  // es寄存器的值， 一个长整数
    unsigned long rax;  // rax寄存器的值， 一个长整数
    unsigned long func;  // func寄存器的值， 一个长整数
    unsigned long errcode;  // errcode寄存器的值， 一个长整数
    unsigned long rip;  // rip寄存器的值， 一个长整数
    unsigned long cs;  // cs寄存器的值， 一个长整数
    unsigned long rflags;  // rflags寄存器的值， 一个长整数
    unsigned long rsp;  // rsp寄存器的值， 一个长整数
    unsigned long ss;  // ss寄存器的值， 一个长整数
};

/* =========================================================================
 =                                 引用方法                                  =
 =========================================================================*/

// 在进入相应的进程之前会先执行本函数
void KernelThreadFunc(void);
// define in entry.S
void ReturnFromSystemCall();
void ResetFromInterrupt();

/* =========================================================================
 =                                 引用数据                                  =
 =========================================================================*/

extern union TaskUnion init_task_union __attribute__((__section__ (".data.init_task")));
extern struct TaskStruct *init_task[NR_CPUS];
extern struct MemoryStruct init_mm;
extern struct thread_struct init_thread;

/* =========================================================================
 =                                 定义函数                                  =
 =========================================================================*/

/**
 * 对进程进行初始化, 在执行完本函数后，会调用switch_to宏切换到init进程：
 * <ul>
 *      <li>初始化描述子</li>
 *      <li>初始化全局信息</li>
 *      <li>切换到init进程执行</li>
 * </ul>
 */
void TaskInit();


#endif