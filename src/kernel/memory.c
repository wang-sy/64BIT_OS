#include "memory.h"
#include "lib.h"


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
		printk("Address:%#010x,%08x\tLength:%#010x,%08x\tType:%#010x\n", \
            p->baseAddrHigh,p->baseAddrLow,p->lengthHigh,p->lengthLow,p->type);

        // 统计可用内存
		unsigned long tmp = 0;
		if(p->type == 1){ // 如果内存可用
			tmp = p->lengthHigh;
			TotalMem +=  p->lengthLow;
			TotalMem +=  tmp  << 32;
		}

		if(p->type > 4)
			break;		
	}

    // 输出总可用内存
	printk("OS Can Used Total RAM:%#018lx\n",TotalMem);
}
