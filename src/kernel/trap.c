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
 * 本文件内容是对0～20号中断的中断处理函数的实现，以及对0～20号中断入口函数的加载。
 * 值得注意的是：
 *  - 15号中断被英特尔保留，不可使用。
 *  - 被加载到IDT中断描述表中的是中断入口函数，中断入口函数在entry.S中定义，
 *    执行完中断入口函数后会跳转到中断处理函数中执行具体的中断处理。
 */
#include "trap.h"
#include "gate.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

/**
 * 除法错误，主要原因是零作为了除数，这里直接将错误原因进行了输出
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoDivideError(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoDivideError(0),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 调试异常，这里不知道具体是干什么的，好像是Intel处理器进行的特殊异常
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoDebug(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoDebug(1),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 不可屏蔽中断， 这里实际上是一个保留的中断，nmi是一个引脚
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoNMI(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoNMI(2),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * int 3 断点，用于调试
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoInt3(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoInt3(3),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 当eflags的OF (overflow)标志被设置时，into (检查溢出)指令被执行
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoOverflow(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoOverflow(4),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 访问了不存在的地址范围的操作数： 越界异常
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoBounds(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoBounds(5),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 读取到了没有意义的机器码
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoUndefinedOPCode(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoUndefinedOPCode(6),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 设备故障一类的？
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoDevNotAvailable(unsigned long rsp, unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoDevNotAvailable(7),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 正常情况下，当CPU正试图为前一个异常调用处理程序时，同时又检测到一个异常，两个异常能被串行处理。
 * 然而，在少数情况下，处理器不能串行地处理它们，因而产生这种异常。
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoDoubleFault(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoDoubleFault(8),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 协处理器段越界，好像只会在特定的cpu上产生这种错误
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoCoprocessorSegmentOverrun(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoCoprocessorSegmentOverrun(9),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 无效的TSS段, 可能发生在:访问TSS段或者任务切换时(这个时候也访问TSS)
 * 包含一个错误码, 该错误码由五个部分组成:
 * - 段选择子\ TI\ IDT\ EXT _ 保留位
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */ 
void DoInvalidTSS(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoInvalidTSS(10),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	switch (error_code & 6) {
        case 2: printk("Ref to IDT");break; // IDT
        case 4: printk("Ref to LDT");break; // LDT
        case 0: printk("Ref to GDT");break; // GDT
        default: printk("Ref ERROR!!!");break; // 错误码出错了
    }

	printk("Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 段不存在, 引用了一个不存在的内存段
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoSegmentNotPresent(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoSegmentNotPresent(11),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	if (error_code & 0x01){
        printk("The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");
	}

	if (error_code & 0x02){
        printk("Refers to a gate descriptor in the IDT;\n");
	} else{
        printk("Refers to a descriptor in the GDT or the current LDT;\n");
    }

	if ((error_code & 0x02) == 0){
        if (error_code & 0x04){
            printk("Refers to a segment or gate descriptor in the LDT;\n");
        } else{
            printk("Refers to a descriptor in the current GDT;\n");
        }
	}
	printk("Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * SS段错误, 试图超过栈段界限的指令或ss标识的段不在内存
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoStackSegmentFault(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoStackSegmentFault(12),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	if (error_code & 0x01) {
		printk("The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");
	}
	if (error_code & 0x02) {
		printk("Refers to a gate descriptor in the IDT;\n");
	} else {
		printk("Refers to a descriptor in the GDT or the current LDT;\n");
	}
	if ((error_code & 0x02) == 0) {
		if (error_code & 0x04) {
				printk("Refers to a segment or gate descriptor in the LDT;\n");
		} else {
				printk("Refers to a descriptor in the current GDT;\n");
		}
	}
	printk("Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 通用保护性异常， 违反了保护膜似的约定
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoGeneralProtection(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoGeneralProtection(13),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	if (error_code & 0x01){
        printk("The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");
	}

	if (error_code & 0x02){
		printk("Refers to a gate descriptor in the IDT;\n");
	} else{
		printk("Refers to a descriptor in the GDT or the current LDT;\n");
	}

	if ((error_code & 0x02) == 0){
		if (error_code & 0x04){
			printk("Refers to a segment or gate descriptor in the LDT;\n");
		} else{
			printk("Refers to a descriptor in the current GDT;\n");
		}
	}
	printk("Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 页错误，在访问内存出错时出现, 输出相应的错误类型
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoPageFault(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	unsigned long cr2 = 0;

	__asm__	__volatile__("movq	%%cr2,	%0":"=r"(cr2)::"memory");

	p = (unsigned long *)(rsp + 0x98);
	printk("DoPageFault(14),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	// 如果是0，就说明是缺页异常， 否则就说明是页级保护异常
	if (!(error_code & 0x01)) {
		printk("Page Not-Present,\t");
	} else {
		printk("Page is Protected!\t"); 
	}
	// 检查W/R位，如果为1就说明写入错误，否则读取错误
	if (error_code & 0x02) {
		printk("Write Cause Fault,\t");
	} else {
		printk("Read Cause Fault,\t");
	}
	// 检查U/S位，1代表普通用户访问时出现错误，否则超级用户访问时错误
	if (error_code & 0x04) {
		printk("Fault in user(3)\t");
	} else {
		printk("Fault in supervisor(0,1,2)\t");
	}
	// RSVD位，如果为1则说明页表的保留项引发异常
	if (error_code & 0x08) {
		printk(",Reserved Bit Cause Fault\t");
	}
	// I/D位，如果为1则说明获取指令时发生异常
	if (error_code & 0x10) {
		printk(",Instruction fetch Cause Fault");
	}

	printk("\n");
	printk("CR2:%#018lx\n",cr2);

	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * x87 FPU错误，特殊错误， 集成到CPU芯片中的浮点单元用信号通知一个错误情形，如数字溢出，或被0除。
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void Dox87FPUError(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("Dox87FPUError(16),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 对齐检测， 操作数的地址没有正确地对齐
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoAlignmentCheck(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoAlignmentCheck(17),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 机器检测， CPU错误或总线错误
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoMachineCheck(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoMachineCheck(18),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 	SIMD浮点异常， 不是很懂
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoSIMDException(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoSIMDException(19),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}

/**
 * 虚拟化异常
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void DoVirtualizationException(unsigned long rsp,unsigned long error_code){
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("DoVirtualizationException(20),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1){
	    // Endless loop
	    continue;
	}
}


/*使用gate.h中声明的函数将中断处理入口函数添加到IDT中
其中，15号中断被英特尔保留，我们不去使用*/
void SystemInterruptVectorInit(){
	SetTrapGate(0,1,DivideErrorEntry);
	SetTrapGate(1,1,DebugEntry);
	SetInterruptGate(2,1,NMIEntry);
	SetSystemGate(3,1,Int3Entry);
	SetSystemGate(4,1,OverflowEntry);
	SetSystemGate(5,1,BoundsEntry);
	SetTrapGate(6,1,UndefinedOpcodeEntry);
	SetTrapGate(7,1,DevNotAvailableEntry);
	SetTrapGate(8,1,DoubleFaultEntry);
	SetTrapGate(9,1,CoprocessorSegmentOverrunEntry);
	SetTrapGate(10,1,InvalidTSSEntry);
	SetTrapGate(11,1,SegmentNotPresentEntry);
	SetTrapGate(12,1,StackSegmentFaultEntry);
	SetTrapGate(13,1,GeneralProtectionEntry);
	SetTrapGate(14,1,PageFaultEntry);
	// 15号中断被英特尔保留
	SetTrapGate(16,1,x87FPUErrorEntry);
	SetTrapGate(17,1,AlignmentCheckEntry);
	SetTrapGate(18,1,MachineCheckEntry);
	SetTrapGate(19,1,SIMDExceptionEntry);
	SetTrapGate(20,1,VirtualizationExceptionEntry);
}
#pragma clang diagnostic pop