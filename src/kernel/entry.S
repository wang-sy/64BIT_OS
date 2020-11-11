/***************************************************
*		版权声明
*
*	本操作系统名为：MINE
*	该操作系统未经授权不得以盈利或非盈利为目的进行开发，
*	只允许个人学习以及公开交流使用
*
*	代码最终所有权及解释权归田宇所有；
*
*	本模块作者：	田宇
*	EMail:		345538255@qq.com
*   本模块二次开发：王赛宇
*   EMail:      wangsaiyu@cqu.edu.cn
*
*
***************************************************/

#include "linkage.h"

R15	=	0x00
R14	=	0x08
R13	=	0x10
R12	=	0x18
R11	=	0x20
R10	=	0x28
R9	=	0x30
R8	=	0x38
RBX	=	0x40
RCX	=	0x48
RDX	=	0x50
RSI	=	0x58
RDI	=	0x60
RBP	=	0x68
DS	=	0x70
ES	=	0x78
RAX	=	0x80
FUNC	=	0x88
ERRCODE	=	0x90
RIP	=	0x98
CS	=	0xa0
RFLAGS	=	0xa8
OLDRSP	=	0xb0
OLDSS	=	0xb8

RESTORE_ALL:
	popq	%r15;		
	popq	%r14;		
	popq	%r13;		
	popq	%r12;		
	popq	%r11;		
	popq	%r10;		
	popq	%r9;		
	popq	%r8;		
	popq	%rbx;		
	popq	%rcx;		
	popq	%rdx;		
	popq	%rsi;		
	popq	%rdi;		
	popq	%rbp;		
	popq	%rax;		
	movq	%rax,	%ds;	
	popq	%rax;		
	movq	%rax,	%es;	
	popq	%rax;		
	addq	$0x10,	%rsp;	
	iretq;

#define GET_CURRENT(reg)	\
	movq	$-32768,reg;	\
	andq	%rsp,	reg

ret_from_exception:
	/*GET_CURRENT(%ebx)	need rewrite*/
ENTRY(ResetFromInterrupt)
	jmp	RESTORE_ALL	/*need rewrite*/

ENTRY(system_call)
	sti
	subq	$0x38,	%rsp			 
	cld;					 

	pushq	%rax;				 	
	movq	%es,	%rax;			 	
	pushq	%rax;				 	
	movq	%ds,	%rax;			 	
	pushq	%rax;				 	
	xorq	%rax,	%rax;			 	
	pushq	%rbp;				 	
	pushq	%rdi;				 	
	pushq	%rsi;				 	
	pushq	%rdx;				 	
	pushq	%rcx;				 
	pushq	%rbx;				 	
	pushq	%r8;				 	
	pushq	%r9;				 	
	pushq	%r10;				 
	pushq	%r11;				 
	pushq	%r12;				 	
	pushq	%r13;				 
	pushq	%r14;				 	
	pushq	%r15;				 	
	movq	$0x10,	%rdx;			 	
	movq	%rdx,	%ds;			 	
	movq	%rdx,	%es;			 
	movq	%rsp,	%rdi			 	
			
	callq	SystemCallFunction		 	////////

ENTRY(ReturnFromSystemCall)						
	movq	%rax,	0x80(%rsp)		 
	popq	%r15				 
	popq	%r14				 	
	popq	%r13				 	
	popq	%r12				 	
	popq	%r11				 	
	popq	%r10				 	
	popq	%r9				 	
	popq	%r8				 	
	popq	%rbx				 	
	popq	%rcx				 	
	popq	%rdx				 	
	popq	%rsi				 	
	popq	%rdi				 	
	popq	%rbp				 	
	popq	%rax				 	
	movq	%rax,	%ds			 
	popq	%rax				 
	movq	%rax,	%es			 
	popq	%rax				 
	addq	$0x38,	%rsp	
	.byte	0x48		 
	sysexit					 


ENTRY(DivideErrorEntry)
	pushq	$0
	pushq	%rax
	leaq	DoDivideError(%rip),	%rax
	xchgq	%rax,	(%rsp)

error_code:
	pushq	%rax
	movq	%es,	%rax
	pushq	%rax
	movq	%ds,	%rax
	pushq	%rax
	xorq	%rax,	%rax

	pushq	%rbp
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15	
	
	cld
	movq	ERRCODE(%rsp),	%rsi
	movq	FUNC(%rsp),	%rdx	

	movq	$0x10,	%rdi
	movq	%rdi,	%ds
	movq	%rdi,	%es

	movq	%rsp,	%rdi
	////GET_CURRENT(%ebx)

	callq 	*%rdx

	jmp	ret_from_exception	

ENTRY(DebugEntry)
	pushq	$0
	pushq	%rax
	leaq	DoDebug(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(NMIEntry)
	pushq	%rax
	cld;			
	pushq	%rax;	
	
	pushq	%rax
	movq	%es,	%rax
	pushq	%rax
	movq	%ds,	%rax
	pushq	%rax
	xorq	%rax,	%rax
	
	pushq	%rbp;		
	pushq	%rdi;		
	pushq	%rsi;		
	pushq	%rdx;		
	pushq	%rcx;		
	pushq	%rbx;		
	pushq	%r8;		
	pushq	%r9;		
	pushq	%r10;		
	pushq	%r11;		
	pushq	%r12;		
	pushq	%r13;		
	pushq	%r14;		
	pushq	%r15;
	
	movq	$0x10,	%rdx;	
	movq	%rdx,	%ds;	
	movq	%rdx,	%es;
	
	movq	$0,	%rsi
	movq	%rsp,	%rdi

	callq	DoNMI

	jmp	RESTORE_ALL

ENTRY(Int3Entry)
	pushq	$0
	pushq	%rax
	leaq	DoInt3(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(OverflowEntry)
	pushq	$0
	pushq	%rax
	leaq	DoOverflow(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(BoundsEntry)
	pushq	$0
	pushq	%rax
	leaq	DoBounds(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(UndefinedOpcodeEntry)
	pushq	$0
	pushq	%rax
	leaq	DoUndefinedOPCode(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(DevNotAvailableEntry)	/* need rewrite */
	pushq	$0
	pushq	%rax
	leaq	DoDevNotAvailable(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(DoubleFaultEntry)
	pushq	%rax
	leaq	DoDoubleFault(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(CoprocessorSegmentOverrunEntry)
	pushq	$0
	pushq	%rax
	leaq	DoCoprocessorSegmentOverrun(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(InvalidTSSEntry)
	pushq	%rax
	leaq	DoInvalidTSS(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(SegmentNotPresentEntry)
	pushq	%rax
	leaq	DoSegmentNotPresent(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(StackSegmentFaultEntry)
	pushq	%rax
	leaq	DoStackSegmentFault(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(GeneralProtectionEntry)
	pushq	%rax
	leaq	DoGeneralProtection(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(PageFaultEntry)
	pushq	%rax
	leaq	DoPageFault(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(x87FPUErrorEntry)
	pushq	$0
	pushq	%rax
	leaq	Dox87FPUError(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(AlignmentCheckEntry)
	pushq	%rax
	leaq	DoAlignmentCheck(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(MachineCheckEntry)
	pushq	$0
	pushq	%rax
	leaq	DoMachineCheck(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(SIMDExceptionEntry)
	pushq	$0
	pushq	%rax
	leaq	DoSIMDException(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code

ENTRY(VirtualizationExceptionEntry)
	pushq	$0
	pushq	%rax
	leaq	DoVirtualizationException(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code


.global KernelThreadFunc
KernelThreadFunc:
	popq	%r15	
	popq	%r14	
	popq	%r13	
	popq	%r12	
	popq	%r11	
	popq	%r10	
	popq	%r9	
	popq	%r8	
	popq	%rbx	
	popq	%rcx	
	popq	%rdx	
	popq	%rsi	
	popq	%rdi	
	popq	%rbp	
	popq	%rax	
	movq	%rax,	%ds	
	popq	%rax		
	movq	%rax,	%es	
	popq	%rax		
	addq	$0x38,	%rsp	
	
	movq	%rdx,	%rdi	
	callq	*%rbx		
	movq	%rax,	%rdi	
	callq	DoTaskExit		

