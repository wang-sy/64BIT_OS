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
 * 本章是对cpu.h中内容的实现，主要负责cpu功能的初始化
 */
/* =========================================================================
 =                                 头的引入                                  =
 =========================================================================*/
#include "printk.h"

/* =========================================================================
 =                                私有函数的实现                              =
 =========================================================================*/

/**
 * 封装的CPUID指令
 * @param main_operation: 主查询号
 * @param sub_operation: 辅查询号
 * @param res_eax: 返回的eax
 * @param res_ebx: 返回的ebx
 * @param res_ecx: 返回的ecx
 * @param res_edx: 返回的edx
 */
void DoCpuid(unsigned int main_operation, unsigned int sub_operation, unsigned int * res_eax, 
             unsigned int * res_ebx, unsigned int * res_ecx, unsigned int * res_edx) {
    __asm__ __volatile__  ("cpuid    \n\t"
                            :"=a"(*res_eax),"=b"(*res_ebx),"=c"(*res_ecx),"=d"(*res_edx)
                            :"0"(main_operation),"2"(sub_operation)
                        );
}

/* =========================================================================
 =                              gate.h中函数的实现                           =
 =========================================================================*/
void InitCpu(void) {
    int i,j;
    unsigned int cpu_factory_name[4] = {0,0,0,0};
    char    factory_name[17] = {0};
    //vendor_string
    DoCpuid(0,0,&cpu_factory_name[0],&cpu_factory_name[1],&cpu_factory_name[2],&cpu_factory_name[3]);
    *(unsigned int*)&factory_name[0] = cpu_factory_name[1];
    *(unsigned int*)&factory_name[4] = cpu_factory_name[3];
    *(unsigned int*)&factory_name[8] = cpu_factory_name[2];
    factory_name[12] = '\0';
    printk("%s\t%#010x\t%#010x\t%#010x\n",factory_name, cpu_factory_name[1],cpu_factory_name[3],cpu_factory_name[2]);
    //brand_string
    for(i = 0x80000002;i < 0x80000005;i++) {
        DoCpuid(i,0,&cpu_factory_name[0],&cpu_factory_name[1],&cpu_factory_name[2],&cpu_factory_name[3]);
        *(unsigned int*)&factory_name[0] = cpu_factory_name[0];
        *(unsigned int*)&factory_name[4] = cpu_factory_name[1];
        *(unsigned int*)&factory_name[8] = cpu_factory_name[2];
        *(unsigned int*)&factory_name[12] = cpu_factory_name[3];
        factory_name[16] = '\0';
        printk("%s",factory_name);
    }
    printk("\n");

    //Version Informatin Type,Family,Model,and Stepping ID
    DoCpuid(1,0,&cpu_factory_name[0],&cpu_factory_name[1],&cpu_factory_name[2],&cpu_factory_name[3]);
    printk("Family Code:%#010x,Extended Family:%#010x,Model Number:%#010x,Extended Model:%#010x,Processor Type:%#010x,Stepping ID:%#010x\n",(cpu_factory_name[0] >> 8 & 0xf),(cpu_factory_name[0] >> 20 & 0xff),(cpu_factory_name[0] >> 4 & 0xf),(cpu_factory_name[0] >> 16 & 0xf),(cpu_factory_name[0] >> 12 & 0x3), (cpu_factory_name[0] & 0xf));
    //get Linear/Physical Address size
    DoCpuid(0x80000008,0,&cpu_factory_name[0],&cpu_factory_name[1],&cpu_factory_name[2], &cpu_factory_name[3]);
    printk("Physical Address size:%08d,Linear Address size:%08d\n",(cpu_factory_name[0] & 0xff),(cpu_factory_name[0] >> 8 & 0xff));
    //max cpuid operation code
    DoCpuid(0,0,&cpu_factory_name[0],&cpu_factory_name[1],&cpu_factory_name[2],&cpu_factory_name[3]);
    printk("MAX Basic Operation Code :%#010x\t",cpu_factory_name[0]);
    DoCpuid(0x80000000,0,&cpu_factory_name[0],&cpu_factory_name[1],&cpu_factory_name[2], &cpu_factory_name[3]);
    printk("MAX Extended Operation Code :%#010x\n",cpu_factory_name[0]);

}