#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"


#define PTRS_PER_PAGE    512 // 页表中条目的个数

#define PAGE_OFFSET ((unsigned long)0xffff800000000000) // 内核层起始线性地址

#define PAGE_1G_SHIFT    30 // 1GB = 1Byte << 30
#define PAGE_2M_SHIFT    21 // 2MB = 1Byte << 21
#define PAGE_4K_SHIFT    12 // 4kB = 1Byte << 12

#define PAGE_2M_SIZE     (1UL << PAGE_2M_SHIFT) // 2M内存的大小
#define PAGE_4K_SIZE     (1UL << PAGE_4K_SHIFT) // 4K内存的大小

// Mask， 类似于子网掩码
#define PAGE_2M_MASK     (~ (PAGE_2M_SIZE - 1)) // 1111..0000 
#define PAGE_4K_MASK     (~ (PAGE_4K_SIZE - 1)) // 1111..0000 

// 将参数addr按2 MB页的上边界对齐
#define PAGE_2M_ALIGN(addr)   (((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr)   (((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

// 将内核层虚拟地址转换成物理地址
#define Virt_To_Phy(addr)  ((unsigned long)(addr) - PAGE_OFFSET)

// 将物理地址转换成内核层虚拟地址
#define Phy_To_Virt(addr)  ((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))

/**
 * 用于描述一个内存块的结构体
 * baseAddr 基地址
 * length 内存块长度
 * type 内存类型， 1 ：可被os使用， 2 ： 正在使用或不可使用 其他：没有意义
 */
struct Memory_Block_E820{
    unsigned long baseAddr;
    unsigned long length;
    unsigned int type;
}__attribute__((packed));


/**
 * 全局的内存信息描述
 */
struct Global_Memory_Descriptor{
	struct Memory_Block_E820 	e820[32];
	unsigned long   e820_length;	
};


extern struct Global_Memory_Descriptor memory_management_struct;

// 从loader写入内存的内存块信息中读取内存信息，存储到结构体中
void init_memory();

#endif