#include "memory.h"
#include "lib.h"

struct Global_Memory_Descriptor memory_management_struct = {{0},0};;


/**
 * 读取内存中存储的内存块信息
 * 默认读取基地址为0x7e00，这里填写线性地址
 * 会输出每一块内存的信息，并且输出总可用内存
 */ 
void init_memory(){
	unsigned long TotalMem = 0 ;
	struct Memory_Block_E820 *p = NULL;	
	
	printk("Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
	p = (struct Memory_Block_E820 *)0xffff800000007e00; // 将基地址对齐

    // 循环遍历，输出内存信息
	for(int i = 0;i < 32;i++, p++){

        // 输出内存信息
		printk("Address:%#018lx\tLength:%#018lx\tType:%#010x\n", p->baseAddr,p->length,p->type);

        // 统计可用内存
		unsigned long tmp = 0;
		if(p->type == 1) TotalMem += p->length;

		// 存储内存块信息， 更新长度
		memory_management_struct.e820[i] = (struct Memory_Block_E820){p->baseAddr, p->length, p->type};
		memory_management_struct.e820_length = i;

		if(p->type > 4) break;		
	}

    // 输出总可用内存
	printk("OS Can Used Total RAM:%#018lx\n",TotalMem);

	TotalMem = 0;

	for(int i = 0; i <= memory_management_struct.e820_length; i ++){
		
		if(memory_management_struct.e820[i].type != 1) continue;
		
		unsigned long start, end;
		start = PAGE_2M_ALIGN(memory_management_struct.e820[i].baseAddr); // 计算开头的向后对齐地址
		end = (
			(memory_management_struct.e820[i].baseAddr + memory_management_struct.e820[i].length)
			 >> PAGE_2M_SHIFT
		) << PAGE_2M_SHIFT; // 计算结尾的向前对齐地址

		if(end <= start) continue;

        TotalMem += (end - start) >> PAGE_2M_SHIFT;
		  
		printk("OS Can Used Total 2M PAGEs:%#010x=%010d\n",TotalMem,TotalMem);

	}
}
