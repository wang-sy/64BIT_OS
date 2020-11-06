#include "trap.h"
#include "gate.h"

/**
 * 0: 除法错误，主要原因是零作为了除数，这里直接将错误原因进行了输出
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_divide_error(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_divide_error(0),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 1： 调试异常，这里不知道具体是干什么的，好像是Intel处理器进行的特殊异常
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_debug(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_debug(1),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 2： 不可屏蔽中断， 这里实际上是一个保留的中断，nmi是一个引脚
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_nmi(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_nmi(2),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * int 3 断点，用于调试
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_int3(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_int3(3),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 当eflags的OF (overflow)标志被设置时，into (检查溢出)指令被执行
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_overflow(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_overflow(4),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 访问了不存在的地址范围的操作数： 越界异常
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_bounds(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_bounds(5),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 读取到了没有意义的机器码
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_undefined_opcode(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_undefined_opcode(6),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 设备故障一类的？
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_dev_not_available(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_dev_not_available(7),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 正常情况下，当CPU正试图为前一个异常调用处理程序时，同时又检测到一个异常，两个异常能被串行处理。
 * 然而，在少数情况下，处理器不能串行地处理它们，因而产生这种异常。
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_double_fault(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_double_fault(8),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 协处理器段越界，好像只会在特定的cpu上产生这种错误
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_coprocessor_segment_overrun(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_coprocessor_segment_overrun(9),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}


/**
 * 无效的TSS段, 可能发生在:访问TSS段或者任务切换时(这个时候也访问TSS)
 * 包含一个错误码, 该错误码由五个部分组成:
 * - 段选择子\ TI\ IDT\ EXT _ 保留位
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 * 
 */ 
void do_invalid_TSS(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_invalid_TSS(10),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	switch (error_code & 6) {
        case 2: printk("Ref to IDT");break; // IDT
        case 4: printk("Ref to LDT");break; // LDT
        case 0: printk("Ref to GDT");break; // GDT
        default: printk("Ref ERROR!!!");break; // 错误码出错了
    }

	printk("Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1);
}

/**
 * 段不存在, 引用了一个不存在的内存段
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_segment_not_present(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_segment_not_present(11),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	if(error_code & 0x01)
		printk("The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

	if(error_code & 0x02)
		printk("Refers to a gate descriptor in the IDT;\n");
	else
		printk("Refers to a descriptor in the GDT or the current LDT;\n");

	if((error_code & 0x02) == 0)
		if(error_code & 0x04)
			printk("Refers to a segment or gate descriptor in the LDT;\n");
		else
			printk("Refers to a descriptor in the current GDT;\n");

	printk("Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1);
}

/**
 * SS段错误, 试图超过栈段界限的指令或ss标识的段不在内存
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_stack_segment_fault(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_stack_segment_fault(12),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	if(error_code & 0x01) printk("The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

	if(error_code & 0x02) printk("Refers to a gate descriptor in the IDT;\n");
	else printk("Refers to a descriptor in the GDT or the current LDT;\n");

	if((error_code & 0x02) == 0){
		if(error_code & 0x04)
			printk("Refers to a segment or gate descriptor in the LDT;\n");
		else
			printk("Refers to a descriptor in the current GDT;\n");
	}

	printk("Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1);
}

/**
 * 通用保护性异常， 违反了保护膜似的约定
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_general_protection(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_general_protection(13),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	if(error_code & 0x01)
		printk("The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

	if(error_code & 0x02)
		printk("Refers to a gate descriptor in the IDT;\n");
	else
		printk("Refers to a descriptor in the GDT or the current LDT;\n");

	if((error_code & 0x02) == 0)
		if(error_code & 0x04)
			printk("Refers to a segment or gate descriptor in the LDT;\n");
		else
			printk("Refers to a descriptor in the current GDT;\n");

	printk("Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1);
}

/**
 * 页错误，在访问内存出错时出现, 输出相应的错误类型
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_page_fault(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	unsigned long cr2 = 0;

	__asm__	__volatile__("movq	%%cr2,	%0":"=r"(cr2)::"memory");

	p = (unsigned long *)(rsp + 0x98);
	printk("do_page_fault(14),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	// 如果是0，就说明是缺页异常， 否则就说明是页级保护异常
	if(!(error_code & 0x01)) printk("Page Not-Present,\t");
	else printk("Page is Protected!\t"); 

	// 检查W/R位，如果为1就说明写入错误，否则读取错误
	if(error_code & 0x02) printk("Write Cause Fault,\t");
	else printk("Read Cause Fault,\t");

	// 检查U/S位，1代表普通用户访问时出现错误，否则超级用户访问时错误
	if(error_code & 0x04) printk("Fault in user(3)\t");
	else printk("Fault in supervisor(0,1,2)\t");

	// RSVD位，如果为1则说明页表的保留项引发异常
	if(error_code & 0x08) printk(",Reserved Bit Cause Fault\t");

	// I/D位，如果为1则说明获取指令时发生异常
	if(error_code & 0x10) printk(",Instruction fetch Cause Fault");

	printk("\n");
	printk("CR2:%#018lx\n",cr2);

	while(1);
}

/**
 * x87 FPU错误，特殊错误， 集成到CPU芯片中的浮点单元用信号通知一个错误情形，如数字溢出，或被0除。
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_x87_FPU_error(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_x87_FPU_error(16),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 对齐检测， 操作数的地址没有正确地对齐
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_alignment_check(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_alignment_check(17),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 机器检测， CPU错误或总线错误
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_machine_check(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_machine_check(18),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 	SIMD浮点异常， 不是很懂
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_SIMD_exception(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_SIMD_exception(19),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/**
 * 虚拟化异常
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_virtualization_exception(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_virtualization_exception(20),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}


/**
 * 初始化IDT中的所有门
 */
void sys_vector_init(){
	set_trap_gate(0,1,divide_error);
	set_trap_gate(1,1,debug);
	set_intr_gate(2,1,nmi);
	set_system_gate(3,1,int3);
	set_system_gate(4,1,overflow);
	set_system_gate(5,1,bounds);
	set_trap_gate(6,1,undefined_opcode);
	set_trap_gate(7,1,dev_not_available);
	set_trap_gate(8,1,double_fault);
	set_trap_gate(9,1,coprocessor_segment_overrun);
	set_trap_gate(10,1,invalid_TSS);
	set_trap_gate(11,1,segment_not_present);
	set_trap_gate(12,1,stack_segment_fault);
	set_trap_gate(13,1,general_protection);
	set_trap_gate(14,1,page_fault);
	//15 Intel reserved. Do not use.
	set_trap_gate(16,1,x87_FPU_error);
	set_trap_gate(17,1,alignment_check);
	set_trap_gate(18,1,machine_check);
	set_trap_gate(19,1,SIMD_exception);
	set_trap_gate(20,1,virtualization_exception);

	//set_system_gate(SYSTEM_CALL_VECTOR,7,system_call);

}

