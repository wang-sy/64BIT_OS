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
 * 本文件是对task.h中内容的实现，完成了对进程的描述以及创建、切换等
 */

#include "task.h"
#include "memory.h"
#include "gate.h"
#include "printk.h"
#include "lib.h"

/* =========================================================================
 =                                 引用函数                                  =
 =========================================================================*/

void system_call();

/* =========================================================================
 =                               宏的定义                                   =
 =========================================================================*/

/**
 * 宏， TASK结构提对应的初始值
 */
#define INIT_TASK(tsk)	\
{			\
	.state = TASK_UNINTERRUPTIBLE,		\
	.flags = PF_KTHREAD,		\
	.mm = &init_mm,			\
	.thread = &init_thread,		\
	.addr_limit = 0xffff800000000000,	\
	.pid = 0,			\
	.counter = 1,		\
	.signal = 0,		\
	.priority = 0		\
}

/**
 * 宏， TSS结构提对应的初始值
 */
#define INIT_TSS \
{	.reserved0 = 0,	 \
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp1 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp2 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.reserved1 = 0,	 \
	.ist1 = 0xffff800000007c00,	\
	.ist2 = 0xffff800000007c00,	\
	.ist3 = 0xffff800000007c00,	\
	.ist4 = 0xffff800000007c00,	\
	.ist5 = 0xffff800000007c00,	\
	.ist6 = 0xffff800000007c00,	\
	.ist7 = 0xffff800000007c00,	\
	.reserved2 = 0,	\
	.reserved3 = 0,	\
	.iomapbaseaddr = 0	\
}

/* =========================================================================
 =                               数据初始化                                   =
 =========================================================================*/

struct TSSStruct init_tss[NR_CPUS] = { [0 ... NR_CPUS-1] = INIT_TSS };

union TaskUnion init_task_union __attribute__((__section__ (".data.init_task"))) = {INIT_TASK(init_task_union.task)};

struct TaskStruct *init_task[NR_CPUS] = {&init_task_union.task,0};
struct MemoryStruct init_mm = {0};

// 初始化调用现场

struct thread_struct init_thread = {
        .rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),
        .rsp = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),
        .fs = KERNEL_DS,
        .gs = KERNEL_DS,
        .cr2 = 0,
        .trap_nr = 0,
        .error_code = 0
};

/* =========================================================================
 =                               私有函数实现                                =
 =========================================================================*/

// ================ 进程创建函数 ================

/**
 * 基本实现进程控制结构体的创建以及相关数据的初始化工作
 * @param regs PTRegs参数
 * @param clone_flags 进程信息， flags
 * @param stack_start 栈的起始地址
 * @param stack_size 栈的大小
 */
unsigned long DoFork(struct PTRegs * regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size) {
    struct TaskStruct *tsk = NULL;
    struct thread_struct *thd = NULL;
    struct Page *p = NULL;

    printk("AllocPages,bitmap:%#018lx\n",*memory_management_struct.bits_map);

    p = AllocPages(ZONE_NORMAL,1,PG_PTable_Maped | PG_Active | PG_Kernel);// 申请页

    printk("AllocPages,bitmap:%#018lx\n",*memory_management_struct.bits_map);

    tsk = (struct TaskStruct *)CONVERT_PHYSICAL_ADDRESS_TO_VIRTUAL_ADDRESS(p->PHY_address); // 将线程描述子放到相应的页中
    printk("struct TaskStruct address:%#018lx\n",(unsigned long)tsk);

    memset(tsk,0,sizeof(*tsk)); // 复制当前进程(复制的是势力， 没有改变指针指向的位置)

    *tsk = *current; // 将current对象中的内容复制归来

    list_init(&tsk->list);
    list_add_to_before(&init_task_union.task.list,&tsk->list);	// 添加关系， 链接入进程队列并且伪造执行现场
    tsk->pid++;
    tsk->state = TASK_UNINTERRUPTIBLE;

    thd = (struct thread_struct *)(tsk + 1);
    tsk->thread = thd;

    memcpy(regs,(void *)((unsigned long)tsk + STACK_SIZE - sizeof(struct PTRegs)),sizeof(struct PTRegs));

    thd->rsp0 = (unsigned long)tsk + STACK_SIZE;
    thd->rip = regs->rip;
    thd->rsp = (unsigned long)tsk + STACK_SIZE - sizeof(struct PTRegs);

    // 如果
    if (!(tsk->flags & PF_KTHREAD)) {// 进程运行于应用层空间， 就将预执行函数设置为： ReturnFromSystemCall
        thd->rip = regs->rip = (unsigned long)ReturnFromSystemCall;
        printk("app run in appLabel, pre function is ReturnFromSystemCall");
    }

    tsk->state = TASK_RUNNING;

    printk("task index :: %#018lx\n",(unsigned long)tsk);
    printk("taks list index :: %#018lx\n", (unsigned long)&tsk->list);
    printk("init next list index :: %#018lx\n", (unsigned long)list_next(&init_task_union.task.list));
    printk("cur task next list index :: %#018lx\n", (unsigned long)list_next(&current->list));
    printk("init index :: %#018lx\n", (unsigned long)&init_task_union);
    printk("cur index :: %#018lx\n", (unsigned long)current);

    return 0;
}

/**
 * 创建进程的函数， 传入一个函数指针以及一些参数， 为操作系统创建一个进程
 */
int CreateKernelThread(unsigned long (* fn)(unsigned long), unsigned long arg, unsigned long flags) {
    struct PTRegs regs; // 用于保存现场的结构体
    memset(&regs,0,sizeof(regs));

    regs.rbx = (unsigned long)fn;
    regs.rdx = (unsigned long)arg;

    regs.ds = KERNEL_DS;
    regs.es = KERNEL_DS;
    regs.cs = KERNEL_CS;
    regs.ss = KERNEL_DS;
    regs.rflags = (1 << 9);
    regs.rip = (unsigned long)KernelThreadFunc;

    return DoFork(&regs,flags,0,0);
}

// ================ 进程切换函数 ================

/**
 * 进程切换的处理函数， 得到当前和下一个进程的地址， 进行切换
 * @param prev 切换的来源
 * @param next 切换到的目标进程
 *
 * 切换方法： 更换rsp和rip寄存器即可， 即可在返回后切换栈和执行地址
 */
void __switch_to(struct TaskStruct *prev,struct TaskStruct *next) {

	init_tss[0].rsp0 = next->thread->rsp0;
	SetTss64(init_tss[0].rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	__asm__ __volatile__("movq	%%fs,	%0 \n\t":"=a"(prev->thread->fs));
	__asm__ __volatile__("movq	%%gs,	%0 \n\t":"=a"(prev->thread->gs));

	__asm__ __volatile__("movq	%0,	%%fs \n\t"::"a"(next->thread->fs));
	__asm__ __volatile__("movq	%0,	%%gs \n\t"::"a"(next->thread->gs));

	printk("pre.therad.rip : %#018lx\n",prev->thread->rip);
	printk("next.therad.rip : %#018lx\n",next->thread->rip);

	printk("prev->thread->rsp0:%#018lx\n",prev->thread->rsp0);
	printk("next->thread->rsp0:%#018lx\n",next->thread->rsp0);
}

/**
 * 模拟的用户函数，借用系统调用，输出一段话，输出完毕后宕机
 */
void UserLevelFunction(){

	long ret = 0;
	char output_string[] = "Hello World!";
	__asm__    __volatile__    (    "leaq    sysexit_return_address(%%rip), %%rdx                           \n\t"
                                    "movq    %%rsp,    %%rcx            \n\t"
                                    "sysenter                           \n\t"
                                    "sysexit_return_address:            \n\t"
                                    :"=a"(ret):"0"(1),"D"(output_string):"memory");
    while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 第一个进程，清空屏幕并输出Hello World! 激动人心的时刻！
 */
unsigned long init(unsigned long arg) {

    struct PTRegs *regs;

    printk("init task is running,arg:%#018lx\n",arg);

    current->thread->rip = (unsigned long)ReturnFromSystemCall;
    current->thread->rsp = (unsigned long)current + STACK_SIZE - sizeof(struct PTRegs);
    regs = (struct PTRegs *)current->thread->rsp;

    __asm__    __volatile__ ( "movq    %1,    %%rsp    \n\t"
                              "pushq %2    \n\t"
                              "jmp   DoExecve \n\t"
                              ::"D"(regs),"m"(current->thread->rsp),"m"(current->thread->rip):"memory");

    return 1;
}

/**
 * 当init内核线程执行do_execve函数后，它会转变为一个用户级进程
 */
unsigned long DoExecve(struct PTRegs * regs){
    regs->rdx = 0x800000;    //RIP
    regs->rcx = 0xa00000;    //RSP
    regs->rax = 1;
    regs->ds = 0;
    regs->es = 0;
    printk("DoExecve task is running\n");

    memcpy(UserLevelFunction,(void *)0x800000,1024);
    return 0;
}

/**
 * 内核级进程结束时候执行的函数， CreateKernelThread创建内核级进程的时候会自动在内核进程结束后， 执行本函数
 */
unsigned long DoTaskExit(unsigned long code) {
    printk("exit task is running,arg:%#018lx\n",code);
    while(1){
        // Endless loop
        continue;
    }
}


/* =========================================================================
 =                             实现task.h中函数                              =
 =========================================================================*/

/*对进程进行初始化*/
void TaskInit() {
    struct TaskStruct *p = NULL;

	init_mm.pgd = (pml4t_t *)Global_CR3;

	init_mm.start_code = memory_management_struct.start_code;
	init_mm.end_code = memory_management_struct.end_code;

	init_mm.start_data = (unsigned long)&_data;
	init_mm.end_data = memory_management_struct.end_data;

	init_mm.start_rodata = (unsigned long)&_rodata; 
	init_mm.end_rodata = (unsigned long)&_erodata;

	init_mm.start_brk = 0;
	init_mm.end_brk = memory_management_struct.end_brk;

	init_mm.start_stack = _stack_start;

	wrmsr(0x174,KERNEL_CS);
	wrmsr(0x175,current->thread->rsp0);
	wrmsr(0x176,(unsigned long)system_call);

//	init_thread,init_tss
	SetTss64(init_thread.rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	init_tss[0].rsp0 = init_thread.rsp0;

	list_init(&init_task_union.task.list);

	CreateKernelThread(init,10,CLONE_FS | CLONE_FILES | CLONE_SIGNAL);

	init_task_union.task.state = TASK_RUNNING;
    

    printk("do get current in p = CONTAINER_OF *tsk\n");

	p = CONTAINER_OF(list_next(&current->list),struct TaskStruct,list);

    printk("%#018lx, %#018lx\n", (unsigned long)current, (unsigned long)p);

    struct TaskStruct* cur = get_current();

    printk("do switch_to fnction\n");

	switch_to(cur,p);
}

/**
 * 获取当前执行进程的联合体指针
 * @return 一个指针，指向当前运行的进程的联合体的首地址
 *
 * <p> 原理： 我们定义的是联合体，所以使用的栈和当前进程的描述子在同一片内存，同时我们定义的时候使用了32KB对齐，
 * 所以我们找到当前的栈顶，对当前栈顶地址进行32KB对齐后的地址就是联合体的首地址，同时由于我们定义的顺序，
 * Task描述子结构体在该联合体的首部，所以获取的联合体的首地址就是该Task描述子的首地址 </p>
 */
struct TaskStruct* get_current() {
    struct TaskStruct * cur = NULL;
    __asm__ __volatile__ ("andq %%rsp,%0 \n\t":"=r"(cur):"0"(~32767UL));
    return cur;
}



