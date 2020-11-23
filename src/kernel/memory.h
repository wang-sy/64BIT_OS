#ifndef _64BITOS_SRC_KERNEL_MEMORY_H
#define _64BITOS_SRC_KERNEL_MEMORY_H

/* =========================================================================
 =                                 宏的定义                                  =
 =========================================================================*/

// 页表中条目的个数
#define PTRS_PER_PAGE    512

// 内核层起始线性地址
#define PAGE_OFFSET ((unsigned long)0xffff800000000000)

#define PAGE_GDT_SHIFT	 39
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

/**
 * 将线性地址转化为物理地址， 仅在内核层中生效
 * @param addr: 被转化的线性地址
 *
 * 实现方法： 将线性地址减掉 PAGE_OFFSET， 就得到物理地址了
 */
#define CONVERT_VIRTUAL_ADDRESS_TO_PHYSICAL_ADDRESS(addr)  ((unsigned long)(addr) - PAGE_OFFSET)

/**
 * 将物理地址转化为线性地址， 仅在内核层中生效
 * @param addr: 被转化的物理地址
 *
 * 实现方法： 将物理地址加上 PAGE_OFFSET， 就得到线性地址了
 */
#define CONVERT_PHYSICAL_ADDRESS_TO_VIRTUAL_ADDRESS(addr)  ((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))


#define Virt_To_2M_Page(kaddr)	(memory_management_struct.pages_struct + (CONVERT_VIRTUAL_ADDRESS_TO_PHYSICAL_ADDRESS(kaddr) >> PAGE_2M_SHIFT))
#define Phy_to_2M_Page(kaddr)	(memory_management_struct.pages_struct + ((unsigned long)(kaddr) >> PAGE_2M_SHIFT))
// 标记页的属性

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

#define PG_Device	(1 << 6)
// 内核层页
#define PG_Kernel	(1 << 7)
// 共享属性
#define PG_K_Share_To_U	(1 << 8)

#define PG_Slab		(1 << 9)

////page table attribute

//	bit 63	Execution Disable:
#define PAGE_XD		(1UL << 63)

//	bit 12	Page Attribute Table
#define	PAGE_PAT	(1UL << 12)

//	bit 8	Global Page:1,global;0,part
#define	PAGE_Global	(1UL << 8)

//	bit 7	Page Size:1,big page;0,small page;
#define	PAGE_PS		(1UL << 7)

//	bit 6	Dirty:1,dirty;0,clean;
#define	PAGE_Dirty	(1UL << 6)

//	bit 5	Accessed:1,visited;0,unvisited;
#define	PAGE_Accessed	(1UL << 5)

//	bit 4	Page Level Cache Disable
#define PAGE_PCD	(1UL << 4)

//	bit 3	Page Level Write Through
#define PAGE_PWT	(1UL << 3)

//	bit 2	User Supervisor:1,user and supervisor;0,supervisor;
#define	PAGE_U_S	(1UL << 2)

//	bit 1	Read Write:1,read and write;0,read;
#define	PAGE_R_W	(1UL << 1)

//	bit 0	Present:1,present;0,no present;
#define	PAGE_Present	(1UL << 0)


//
#define PAGE_KERNEL_GDT		(PAGE_R_W | PAGE_Present)

//1,0	
#define PAGE_KERNEL_Dir		(PAGE_R_W | PAGE_Present)

//7,1,0
#define	PAGE_KERNEL_Page	(PAGE_PS  | PAGE_R_W | PAGE_Present)

//2,1,0
#define PAGE_USER_Dir		(PAGE_U_S | PAGE_R_W | PAGE_Present)

//7,2,1,0
#define	PAGE_USER_Page		(PAGE_PS  | PAGE_U_S | PAGE_R_W | PAGE_Present)

// 标记内存类型

// 内核使用的内存
#define ZONE_DMA	(1 << 0)
// 普通内存
#define ZONE_NORMAL	(1 << 1)
// 不被页表映射的内存
#define ZONE_UNMAPED	(1 << 2)

typedef struct {unsigned long pml4t;} pml4t_t;
#define	mk_mpl4t(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_mpl4t(mpl4tptr,mpl4tval)	(*(mpl4tptr) = (mpl4tval))

typedef struct {unsigned long pdt;} pdt_t;
#define mk_pdt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdt(pdtptr,pdtval)		(*(pdtptr) = (pdtval))

typedef struct {unsigned long pdpt;} pdpt_t;
#define mk_pdpt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdpt(pdptptr,pdptval)	(*(pdptptr) = (pdptval))

#define SIZEOF_LONG_ALIGN(size) ((size + sizeof(long) - 1) & ~(sizeof(long) - 1) )
#define SIZEOF_INT_ALIGN(size) ((size + sizeof(int) - 1) & ~(sizeof(int) - 1) )

/**
 * 刷新TLB， 重新读取
 */
#define FLUSH_TLB()						\
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

/* =========================================================================
 =                               结构题定义                                  =
 =========================================================================*/

/**
 * 用于描述一个内存块的结构体
 * @param baseAddr 基地址
 * @param length 内存块长度
 * @param type 内存类型， 1 ：可被os使用， 2 ： 正在使用或不可使用 其他：没有意义
 */
struct MemoryBlockE820{
    unsigned long   baseAddr;  // 基地址
    unsigned long   length;  // 内存块长度
    unsigned int    type;  // 内存类型， 1 ：可被os使用， 2 ： 正在使用或不可使用 其他：没有意义
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
struct GlobalMemoryDescriptor{
	struct MemoryBlockE820 	e820[32];  // cpu探索内存后存储的信息
	unsigned long               e820_length;  // 记录有效的内存的长度，配合e820使用

    unsigned long *   bits_map;  // 物理地址空间页映射位图
    unsigned long     bits_size;  // 物理地址空间页数量
    unsigned long     bits_length;  // 物理地址空间页映射位图长度

    struct Page *     pages_struct;  // 指向全局struct page结构体数组的指针
    unsigned long     pages_size;  // struct page结构体总数
    unsigned long     pages_length;  // struct page结构体数组长度

    struct Zone *     zones_struct;  // 指向全局zone结构体数组的指针
    unsigned long     zones_size;  // zone结构体数量
    unsigned long     zones_length;  // zone数组长度

    unsigned long     start_code , end_code , end_data , end_brk; // 程序与数据的开头与结尾
    unsigned long     end_of_struct;  // 内存页管理结构的结尾地址
};
// 引用memory.c中的内存管理结构提
extern struct GlobalMemoryDescriptor memory_management_struct;


/**
 * 描述一个页的基本信息
 * @param zone_struct 指向本页所属的区域结构体
 * @param PHY_address 页的物理地址
 * @param attribute 页的属性
 * @param reference_count 描述该页的引用次数
 * @param age 描述该页的创建时间
 */
struct Page {
    struct Zone *        zone_struct; // 指向本页所属的区域结构体
    unsigned long        PHY_address; // 页的物理地址
    unsigned long        attribute; // 页的属性
    unsigned long        reference_count; // 描述该页的引用次数
    unsigned long        age; // 描述该页的创建时间
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
    struct Page *        pages_group;  // 属于该zone的page的列表
    unsigned long        pages_length;  // 当前区域包含的页的数量
    unsigned long        zone_start_address;  // 本区域第一个页对齐后地址
    unsigned long        zone_end_address;  // 本区域最后一个页对齐后地址
    unsigned long        zone_length;  // 本区域经过页对齐后的地址长度
    unsigned long        attribute;  // 空间属性

    struct GlobalMemoryDescriptor * GMD_struct;  // 指针，指向全局描述Global_Memory_Descriptor 的实例化对象 memory_management_struct

    unsigned long        page_using_count;  // 已经使用的页数量
    unsigned long        page_free_count;  // 没有被使用的页的数量
    unsigned long        total_pages_link;  // 本区域物理页被引用次数之和
};

/* =========================================================================
 =                               数据引用                                   =
 =========================================================================*/

// 标记编译器生成的区间
extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

// 不同类型Zone的起始编号
extern int ZONE_DMA_INDEX;
extern int ZONE_NORMAL_INDEX;
extern int ZONE_UNMAPED_INDEX;

extern unsigned long * Global_CR3;

/* =========================================================================
 =                               函数声明                                   =
 =========================================================================*/

/**
 * 读取内存中存储的内存块信息
 * 默认读取基地址为0x7e00，这里填写线性地址
 * 会输出每一块内存的信息，并且输出总可用内存，
 * 并且将可用的内存信息保存在memory_management_struct结构体中
 */
void InitMemory();

/**
 * 对某个内存页进行初始化， 在申请新的page时，对申请到的page调用本函数进行初始化
 *  @param page 指针，指向想要被初始化的 page
 *  @param flag 初始化时的参数
 */
unsigned long PageInit(struct Page * page, unsigned long flag);

/**
 * 根据给出的需求，返回一个初始化后的page列表的起始地址
 * @param zone_select 需求的页的类型
 * @param number 需求的页的数量
 * @param page_flags 需求的状态
 */
struct Page* AllocPages(int zone_select,int number,unsigned long page_flags);

void free_pages(struct Page * page,int number);

unsigned long page_clean(struct Page * page);

void FrameBufferInit();
void PageTableInit();

#endif