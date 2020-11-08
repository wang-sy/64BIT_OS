#include "lib.h"
#include "memory.h"
#include "cpu.h"

// stack size 32K
#define STACK_SIZE 32768

// 偏移量
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
struct mm_struct {
    pml4t_t *pgd;// 指向一个页表

    unsigned long start_code,end_code;
    unsigned long start_data,end_data;
    unsigned long start_rodata,end_rodata;
    unsigned long start_brk,end_brk;
    unsigned long start_stack;
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
    unsigned long rsp0;

    unsigned long rip;
    unsigned long rsp;

    unsigned long fs;
    unsigned long gs;

    unsigned long cr2;
    unsigned long trap_nr;
    unsigned long error_code;
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
struct task_struct {
    struct List list;
    volatile long state;
    unsigned long flags;

    struct mm_struct *mm;
    struct thread_struct *thread;

    /*0x0000,0000,0000,0000 - 0x0000,7fff,ffff,ffff 用户层*/
    /*0xffff,8000,0000,0000 - 0xffff,ffff,ffff,ffff 内核层*/
    unsigned long addr_limit; 

    long pid;
    long counter;
    long signal;
    long priority;
};

/**
 * task_struct和内核层栈区共用一段内存
 * task --> stack 两者公用 stack的空间
 */
union task_union {
    struct task_struct task;
    unsigned long stack[STACK_SIZE / sizeof(unsigned long)];
}__attribute__((aligned (8)));    //8 Bytes align


// IA-32e tss
struct tss_struct{
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

// 用于记录系统调用的调用现场
struct pt_regs {
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long rbx;
    unsigned long rcx;
    unsigned long rdx;
    unsigned long rsi;
    unsigned long rdi;
    unsigned long rbp;
    unsigned long ds;
    unsigned long es;
    unsigned long rax;
    unsigned long func;
    unsigned long errcode;
    unsigned long rip;
    unsigned long cs;
    unsigned long rflags;
    unsigned long rsp;
    unsigned long ss;
};
struct task_struct* get_current();

#define GET_CURRENT                    \
    "movq    %rsp,    %rbx    \n\t"    \
    "andq    $-32768,%rbx     \n\t"

#define current (get_current())

// 从某个进程切换到另一个进程的函数
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

#define container_of(ptr,type,member)                            \
({    \
    typeof(((type *)0)->member) * p = (ptr);                    \
    (type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));        \
})

// 在进入相应的进程之前会先执行本函数
extern void kernel_thread_func(void);

extern void __switch_to(struct task_struct *prev,struct task_struct *next);

extern union task_union init_task_union __attribute__((__section__ (".data.init_task")));
extern struct task_struct *init_task[NR_CPUS];
extern struct mm_struct init_mm;
extern struct thread_struct init_thread;



// define in entry.S
void ret_system_call();
void ret_from_intr();

void task_init();

unsigned long do_exit(unsigned long code);

int kernel_thread(unsigned long (* fn)(unsigned long), unsigned long arg, unsigned long flags);

unsigned long do_fork(struct pt_regs * regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size);

unsigned long init(unsigned long arg);

void __switch_to(struct task_struct *prev,struct task_struct *next);

