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
 * 本文件内容是对gate.h中声明的函数的实现，完成了添加不同门级描述符、重置TSS的功能
 */

#include "gate.h"

/*
 * 创建一个中断门，其权限等级为0(内核态)
 */
void SetInterruptGate(unsigned int n,unsigned char ist,void * addr){
	SET_GATE(idt_table + n , 0x8E , ist , addr);	//P,DPL=0,TYPE=E
}

/*
 * 创建一个陷阱门，其权限等级为0(内核态)
 */
void SetTrapGate(unsigned int n,unsigned char ist,void * addr){
	SET_GATE(idt_table + n , 0x8F , ist , addr);	//P,DPL=0,TYPE=F
}

/*
 * 创建一个系统门，其权限等级为3(用户态)
 */
void SetSystemGate(unsigned int n,unsigned char ist,void * addr){
	SET_GATE(idt_table + n , 0xEF , ist , addr);	//P,DPL=3,TYPE=F
}

/*
 * <p>配置IA-32e模式下TSS段内的各个RSP和IST项</p>
 * 在IA-32e模式下，TSS由RSP0~2、IST1~7 这十个六十四位数构成，他们被存储在一片连续的内存空间中：
 *  <ul>
 *      <li> 该内存空间在head.S中进行了定义 </li>
 *      <li> 在gate.h中进行了声明：tss64_table </li>
 *      <li> 通过LOAD_TR宏加载到TR寄存器中 </li>
 *  </ul>
 * CPU就知道了TSS的首地址，这里更改了相应的地址空间，达到了更改TSS的目标
 */
void SetTss64(unsigned long rsp0, unsigned long rsp1, unsigned long rsp2, unsigned long ist1,
              unsigned long ist2, unsigned long ist3, unsigned long ist4, unsigned long ist5,
              unsigned long ist6, unsigned long ist7){
    // 填写rsp部分，一次填写64位
	*(unsigned long *)(tss64_table+1) = rsp0;
	*(unsigned long *)(tss64_table+3) = rsp1;
	*(unsigned long *)(tss64_table+5) = rsp2;
    // 填写ist部分，一次填写64位
	*(unsigned long *)(tss64_table+9) = ist1;
	*(unsigned long *)(tss64_table+11) = ist2;
	*(unsigned long *)(tss64_table+13) = ist3;
	*(unsigned long *)(tss64_table+15) = ist4;
	*(unsigned long *)(tss64_table+17) = ist5;
	*(unsigned long *)(tss64_table+19) = ist6;
	*(unsigned long *)(tss64_table+21) = ist7;	
}
