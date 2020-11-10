#ifndef _64BITOS_SRC_KERNEL_TRAP_H
#define _64BITOS_SRC_KERNEL_TRAP_H

/* =========================================================================
 =                                   引入头                                 =
 =========================================================================*/

#include "linkage.h"
#include "printk.h"
#include "lib.h"

/* =========================================================================
 =                             函数声明——entry.S                             =
 =========================================================================*/

void DivideErrorEntry();
void DebugEntry();
void NMIEntry();
void Int3Entry();
void OverflowEntry();
void BoundsEntry();
void UndefinedOpcodeEntry();
void DevNotAvailableEntry();
void DoubleFaultEntry();
void CoprocessorSegmentOverrunEntry();
void InvalidTSSEntry();
void SegmentNotPresentEntry();
void StackSegmentFaultEntry();
void GeneralProtectionEntry();
void PageFaultEntry();
void x87FPUErrorEntry();
void AlignmentCheckEntry();
void MachineCheckEntry();
void SIMDExceptionEntry();
void VirtualizationExceptionEntry();

/* =========================================================================
 =                                   函数声明                                =
 =========================================================================*/

/**
 * <h3>函数，用于上方的中断处理函数入口制作为门，并且根据相应的中断向量号添加到IDT中</h3>
 */
void SystemInterruptVectorInit();


#endif
