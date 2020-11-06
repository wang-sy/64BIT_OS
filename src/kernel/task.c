#include "task.h"
#include "position.h"
#include "memory.h"
#include "gate.h"

/** 实例化第一个进程 */

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

struct tss_struct init_tss[NR_CPUS] = { [0 ... NR_CPUS-1] = INIT_TSS };


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

union task_union init_task_union __attribute__((__section__ (".data.init_task"))) = {INIT_TASK(init_task_union.task)};

struct task_struct *init_task[NR_CPUS] = {&init_task_union.task,0};
struct mm_struct init_mm = {0};


// 初始化调用现场

struct thread_struct init_thread = 
{
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),
	.rsp = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),
	.fs = KERNEL_DS,
	.gs = KERNEL_DS,
	.cr2 = 0,
	.trap_nr = 0,
	.error_code = 0
};



/**
 * 进程切换的处理函数， 得到当前和下一个进程的地址， 进行切换
 */
void __switch_to(struct task_struct *prev,struct task_struct *next) {

	printk("do __switch to begin\n");
	init_tss[0].rsp0 = next->thread->rsp0;
	set_tss64(init_tss[0].rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	__asm__ __volatile__("movq	%%fs,	%0 \n\t":"=a"(prev->thread->fs));
	__asm__ __volatile__("movq	%%gs,	%0 \n\t":"=a"(prev->thread->gs));

	__asm__ __volatile__("movq	%0,	%%fs \n\t"::"a"(next->thread->fs));
	__asm__ __volatile__("movq	%0,	%%gs \n\t"::"a"(next->thread->gs));

	printk("pre.therad.rip : %#018lx\n",prev->thread->rip);
	printk("next.therad.rip : %#018lx\n",next->thread->rip);
	printk("kernel_thread_func addr : %#018lx\n",kernel_thread_func);

	printk("__switch_to addr : %#018lx\n",__switch_to);

	

	printk("prev->thread->rsp0:%#018lx\n",prev->thread->rsp0);
	printk("next->thread->rsp0:%#018lx\n",next->thread->rsp0);
	printk("do __switch to over\n");
}

/**
 * 第一个进程，清空屏幕并输出Hello World! 激动人心的时刻！
 */
unsigned long init(unsigned long arg) {

    doClear(&globalPosition);

    printk("Hello World!\ninit task is running,arg:%#018lx\n",arg);

    return 1;
}

struct task_struct* get_current() {
    struct task_struct * cur = NULL;
    __asm__ __volatile__ ("andq %%rsp,%0 \n\t":"=r"(cur):"0"(~32767UL));
    return cur;
}

unsigned long do_fork(struct pt_regs * regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size) {
	struct task_struct *tsk = NULL;
	struct thread_struct *thd = NULL;
	struct Page *p = NULL;
	
	printk("alloc_pages,bitmap:%#018lx\n",*memory_management_struct.bits_map);

	p = alloc_pages(ZONE_NORMAL,1,PG_PTable_Maped | PG_Active | PG_Kernel);// 申请页

	printk("alloc_pages,bitmap:%#018lx\n",*memory_management_struct.bits_map);

	tsk = (struct task_struct *)Phy_To_Virt(p->PHY_address); // 将线程描述子放到相应的页中
	printk("struct task_struct address:%#018lx\n",(unsigned long)tsk);

	memset(tsk,0,sizeof(*tsk)); // 复制当前进程(复制的是势力， 没有改变指针指向的位置)

	*tsk = *current; // 将current对象中的内容复制归来

	list_init(&tsk->list); 
	list_add_to_before(&init_task_union.task.list,&tsk->list);	// 添加关系， 链接入进程队列并且伪造执行现场
	tsk->pid++;
	tsk->state = TASK_UNINTERRUPTIBLE;

	thd = (struct thread_struct *)(tsk + 1);
	tsk->thread = thd;	

	memcpy(regs,(void *)((unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs)),sizeof(struct pt_regs));

	thd->rsp0 = (unsigned long)tsk + STACK_SIZE;
	thd->rip = regs->rip;
	thd->rsp = (unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs);

    // 如果
	if(!(tsk->flags & PF_KTHREAD)) {// 进程运行于应用层空间， 就将预执行函数设置为： ret_from_intr
		thd->rip = regs->rip = (unsigned long)ret_from_intr;
		printk("app run in appLabel, pre function is ret_from_intr");
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
int kernel_thread(unsigned long (* fn)(unsigned long), unsigned long arg, unsigned long flags) {
    struct pt_regs regs; // 用于保存现场的结构体
    memset(&regs,0,sizeof(regs));


    regs.rbx = (unsigned long)fn;
    regs.rdx = (unsigned long)arg;

    regs.ds = KERNEL_DS;
    regs.es = KERNEL_DS;
    regs.cs = KERNEL_CS;
    regs.ss = KERNEL_DS;
    regs.rflags = (1 << 9);
    regs.rip = (unsigned long)kernel_thread_func;

    return do_fork(&regs,flags,0,0);
}

/**
 * 初始化init进程， 并且进行进程切换
 */
void task_init() {
    struct task_struct *p = NULL;

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

//	init_thread,init_tss
	set_tss64(init_thread.rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	init_tss[0].rsp0 = init_thread.rsp0;

	list_init(&init_task_union.task.list);

	kernel_thread(init,10,CLONE_FS | CLONE_FILES | CLONE_SIGNAL);

	init_task_union.task.state = TASK_RUNNING;
    

    printk("do get current in p = container_of *tsk\n");

	p = container_of(list_next(&current->list),struct task_struct,list);

    printk("%#018lx, %#018lx\n", (unsigned long)current, (unsigned long)p);


    struct task_struct* cur = get_current();

    printk("do switch_to fnction\n");

	switch_to(cur,p);
}



unsigned long do_exit(unsigned long code) {
    printk("exit task is running,arg:%#018lx\n",code);
    while(1);
}
