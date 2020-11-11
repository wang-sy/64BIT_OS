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
 *
 * 当应用程序执行SYSENTER指令进入内核层时：
 * <ul>
 *      <li>过system_call模块保存应用程序的执行现场 （在entry.S 中定义）</li>
 *      <li>system_call模块会将当前栈指针作为参数传递给system_call_function函数</li>
 *      <li>使用CALL指令调用system_call_function函数, 执行与之相匹配的处理程序</li>
 * </ul>
 * 想要定义不同系统调用号执行何种系统调用例程， 进需更改相应的系统调用表即可
 */

/* =========================================================================
 =                               头的引入                                    =
 =========================================================================*/

#include "task.h"
#include "printk.h"

/* =========================================================================
 =                               宏的定义                                    =
 =========================================================================*/

#define MAX_SYSTEM_CALL_NR 128
typedef unsigned long (* system_call_t)(struct PTRegs * regs);

/* =========================================================================
 =                               私有函数实现                                =
 =========================================================================*/

/**
 * 1号 系统调用： 用于输出一个纯字符串
 * @param regs PTRegs指针
 *
 * 只能传入一个纯字符串，不能传入参数
 */
unsigned long SystemCallPrintf(struct PTRegs * regs) {
    printk((char *)regs->rdi);
    return 1;
}

/**
 * 在不存在系统调用时的响应函数，是所有系统调用的默认处理函数
 * @param regs PTRegs指针
 */
unsigned long SystemCallNotFound(struct PTRegs * regs) {
    printk("SystemCallNotFound is calling,NR:%#04x\n",regs->rax);
    printk("do exit\n");
    return -1;
}

// 系统调用表
system_call_t system_call_table[MAX_SYSTEM_CALL_NR] = {
        [0] = (system_call_t)SystemCallNotFound,
        [1] = (system_call_t)SystemCallPrintf,
        [2 ... MAX_SYSTEM_CALL_NR-1] = (system_call_t)SystemCallNotFound,
};

/**
 * 根据系统调用号返回一个系统调用处理函数
 * 本函数会被entry.S中的system_call函数调用
 */
unsigned long SystemCallFunction(struct PTRegs * regs) {
    return system_call_table[regs->rax](regs);
}
