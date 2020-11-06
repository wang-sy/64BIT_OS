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


////struct page attribute (alloc_pages flags)

// 经过页表映射的页
#define PG_PTable_Maped	(1 << 0)
// 内核初始化程序
#define PG_Kernel_Init	(1 << 1)
// 被引用的页
#define PG_Referenced	(1 << 2)
// 脏的页
#define PG_Dirty	(1 << 3)
// 使用中的页
#define PG_Active	(1 << 4)
// 最新的页？
#define PG_Up_To_Date	(1 << 5)
// 
#define PG_Device	(1 << 6)
// 内核层页
#define PG_Kernel	(1 << 7)
// 共享属性
#define PG_K_Share_To_U	(1 << 8)
//
#define PG_Slab		(1 << 9)


// 内核使用的内存
#define ZONE_DMA	(1 << 0)
// 普通内存
#define ZONE_NORMAL	(1 << 1)
// 不被页表映射的内存
#define ZONE_UNMAPED	(1 << 2)

typedef struct {unsigned long pml4t;} pml4t_t;
#define	mk_mpl4t(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_mpl4t(mpl4tptr,mpl4tval)	(*(mpl4tptr) = (mpl4tval))



#define flush_tlb()						\
do								\
{								\
	unsigned long	tmpreg;					\
	__asm__ __volatile__ 	(				\
				"movq	%%cr3,	%0	\n\t"	\
				"movq	%0,	%%cr3	\n\t"	\
				:"=r"(tmpreg)			\
				:				\
				:"memory"			\
				);				\
}while(0)

/**
 * 用于描述一个内存块的结构体
 * baseAddr 基地址
 * length 内存块长度
 * type 内存类型， 1 ：可被os使用， 2 ： 正在使用或不可使用 其他：没有意义
 */
struct Memory_Block_E820{
    unsigned long   baseAddr;
    unsigned long   length;
    unsigned int    type;
}__attribute__((packed));


/**
 * 全局的内存信息描述
 * @param e820 cpu探索内存后存储的信息
 * @param e820_length 记录有效的内存的长度，配合e820使用
 * @param bits_map 物理地址空间页映射位图
 * @param bits_size 物理地址空间页数量
 * @param bits_length 物理地址空间页映射位图长度
 * @param pages_struct 	指向全局struct page结构体数组的指针
 * @param pages_size struct page结构体总数
 * @param pages_length struct page结构体数组长度
 * @param zones_struct 指向全局zone结构体数组的指针
 * @param zones_size zone结构体数量
 * @param zones_length zone数组长度
 * @param start_code 内核程序起始段代码地址
 * @param end_code 内核程序结束代码段地址
 * @param end_data 内核程序结束数据段地址
 * @param end_brk 内核程序的结束地址
 * @param end_of_struct 内存页管理结构的结尾地址
 */
struct Global_Memory_Descriptor{
	struct Memory_Block_E820 	e820[32];
	unsigned long               e820_length;	

    unsigned long *   bits_map;
    unsigned long     bits_size;
    unsigned long     bits_length;

    struct Page *     pages_struct;
    unsigned long     pages_size;
    unsigned long     pages_length;

    struct Zone *     zones_struct;
    unsigned long     zones_size;
    unsigned long     zones_length;

    unsigned long     start_code , end_code , end_data , end_brk;

    unsigned long     end_of_struct;
};

extern struct Global_Memory_Descriptor memory_management_struct;


/**
 * 描述一个页的基本信息
 * @param zone_struct 指向本页所属的区域结构体
 * @param PHY_address 页的物理地址
 * @param attribute 页的属性
 * @param reference_count 描述该页的引用次数
 * @param age 描述该页的创建时间
 */
struct Page {
    struct Zone *        zone_struct;
    unsigned long        PHY_address;
    unsigned long        attribute;
    unsigned long        reference_count;
    unsigned long        age;
};


/**
 * 可用物理内存区域
 * @param pages_group 属于该zone的page的列表
 * @param pages_length 当前区域包含的页的数量
 * @param zone_start_address 本区域第一个页对齐后地址
 * @param zone_end_address  本区域最后一个页对齐后地址
 * @param zone_length 本区域经过页对齐后的地址长度
 * @param attribute 空间属性
 * @param GMD_struct 指针，指向全局描述Global_Memory_Descriptor 的实例化对象 memory_management_struct
 * @param page_using_count 已经使用的页数量
 * @param page_free_count 没有被使用的页的数量
 * @param total_pages_link 本区域物理页被引用次数之和
 */
struct Zone {
    struct Page *        pages_group;
    unsigned long        pages_length;
    unsigned long        zone_start_address;
    unsigned long        zone_end_address;
    unsigned long        zone_length;
    unsigned long        attribute;

    struct Global_Memory_Descriptor * GMD_struct;

    unsigned long        page_using_count;
    unsigned long        page_free_count;
    unsigned long        total_pages_link;
};

// 标记编译器生成的区间
extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

extern int ZONE_DMA_INDEX;
extern int ZONE_NORMAL_INDEX;
extern int ZONE_UNMAPED_INDEX;

extern unsigned long * Global_CR3;

// 从loader写入内存的内存块信息中读取内存信息，存储到结构体中
void init_memory();

// 对page结构体进行初始化， 根据flag中的特殊标志位进行特殊处理
unsigned long page_init(struct Page* page, unsigned long flag);

unsigned long * Get_gdt();


struct Page* alloc_pages(int zone_select,int number,unsigned long page_flags);


#endif