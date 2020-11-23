#include "memory.h"
#include "printk.h"
#include "slab.h"
#include "lib.h"

// 通用内存管理函数



/**
 * 用于记录不同内存池的size
 */
struct SlabCache kmalloc_slab_cache[16] = {
    {32,        0,  0,  NULL,   NULL,   NULL,   NULL},
    {64,        0,  0,  NULL,   NULL,   NULL,   NULL},
    {128,       0,  0,  NULL,   NULL,   NULL,   NULL},
    {256,       0,  0,  NULL,   NULL,   NULL,   NULL},
    {512,       0,  0,  NULL,   NULL,   NULL,   NULL},
    {1024,      0,  0,  NULL,   NULL,   NULL,   NULL},
    {2048,      0,  0,  NULL,   NULL,   NULL,   NULL},
    {4096,      0,  0,  NULL,   NULL,   NULL,   NULL},
    {8192,      0,  0,  NULL,   NULL,   NULL,   NULL},
    {16384,     0,  0,  NULL,   NULL,   NULL,   NULL},
    {32768,     0,  0,  NULL,   NULL,   NULL,   NULL},
    {65536,     0,  0,  NULL,   NULL,   NULL,   NULL},
    {131072,    0,  0,  NULL,   NULL,   NULL,   NULL},
    {262144,    0,  0,  NULL,   NULL,   NULL,   NULL},
    {524288,    0,  0,  NULL,   NULL,   NULL,   NULL},
    {1048576,   0,  0,  NULL,   NULL,   NULL,   NULL},
};

struct Slab * kmalloc_create(unsigned long size) {
	int i;
	struct Slab * slab = NULL;
	struct Page * page = NULL;
	unsigned long * vaddresss = NULL;
	long structsize = 0;

	page = AllocPages(ZONE_NORMAL,1, 0);
	
	if (page == NULL) {
		printk("kmalloc_create()->AllocPages()=>page == NULL\n");
		return NULL;
	}
	
	PageInit(page,PG_Kernel);

	switch (size) {
		// 对于单元较小的，其Slab与colormap的总体积较大，正常申请即可
		case 32:
		case 64:
		case 128:
		case 256:
		case 512:

			vaddresss = CONVERT_PHYSICAL_ADDRESS_TO_VIRTUAL_ADDRESS(page->PHY_address);
			structsize = sizeof(struct Slab) + PAGE_2M_SIZE / size / 8;
			slab = (struct Slab *)((unsigned char *)vaddresss + PAGE_2M_SIZE - structsize);
			slab->color_map = (unsigned long *)((unsigned char *)slab + sizeof(struct Slab));
			slab->free_count = (PAGE_2M_SIZE - (PAGE_2M_SIZE / size / 8) - sizeof(struct Slab)) / size;
			slab->using_count = 0;
			slab->color_count = slab->free_count;
			slab->virtual_address = vaddresss;
			slab->page = page;
			list_init(&slab->list);
			slab->color_length = ((slab->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
			memset(slab->color_map,0xff,slab->color_length);
			for(i = 0;i < slab->color_count;i++){
                *(slab->color_map + (i >> 6)) ^= 1UL << i % 64;
            }
			break;

		// 对于体积较大的，其块大小较大，所以colormap元素数量少，导致占用空间少，直接使用kmalloc进行申请即可
		case 1024:
		case 2048:
		case 4096:
		case 8192:
		case 16384:
		case 32768:
		case 65536:
		case 131072:
		case 262144:
		case 524288:
		case 1048576:

			slab = (struct Slab *)kmalloc(sizeof(struct Slab),0);
			slab->free_count = PAGE_2M_SIZE / size;
			slab->using_count = 0;
			slab->color_count = slab->free_count;
			slab->color_length = ((slab->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
			slab->color_map = (unsigned long *)kmalloc(slab->color_length,0);
			memset(slab->color_map,0xff,slab->color_length);
			slab->virtual_address = CONVERT_PHYSICAL_ADDRESS_TO_VIRTUAL_ADDRESS(page->PHY_address);
			slab->page = page;
			list_init(&slab->list);
			for(i = 0;i < slab->color_count;i++){ 
                *(slab->color_map + (i >> 6)) ^= 1UL << i % 64;
            }
				
			break;

		default:

			printk("kmalloc_create() ERROR: wrong size:%08d\n",size);
			free_pages(page,1);
			
			return NULL;
	}	
	return slab;
}


/**
 * 释放一个由Slab管理的内存区域
 * @param address 想要释放的区域的首地址
 * @return 如果成功，那么返回1, 否则返回0
 * 
 * <h3>原理</h3>
 * <p>
 *      我们知道，每个Slab都直接管理一个物理页，同时，我们的物理页都是对齐了的。
 * </p>
 * <p>
 *      我们将被释放的内存的首地址与被管理的内存页的首地址进行比较，如果这两个地址完全相同
 *      那么就说明：想要被释放的内存在这个物理页中，即：使用的内存，被当前的Slab进行管理
 * </p>
 */
unsigned long kfree(void * address) { // 用于释放内存
	int i;
	int index;
	struct Slab * slab = NULL;
	void * page_base_address = (void *)((unsigned long)address & PAGE_2M_MASK);

	for(i = 0;i < 16;i++) { // 查找当前内存在哪个Slab的管辖内
		slab = kmalloc_slab_cache[i].cache_pool;
		do {
			if (slab->virtual_address == page_base_address) {
				index = (address - slab->virtual_address) / kmalloc_slab_cache[i].size;
				*(slab->color_map + (index >> 6)) ^= 1UL << index % 64;
				slab->free_count++;
				slab->using_count--;
				kmalloc_slab_cache[i].total_free++;
				kmalloc_slab_cache[i].total_using--;
				if ((slab->using_count == 0) && (kmalloc_slab_cache[i].total_free >= slab->color_count * 3 / 2) && (kmalloc_slab_cache[i].cache_pool != slab)) {
					switch(kmalloc_slab_cache[i].size) { // 对于小单元的处理
						case 32:
						case 64:
						case 128:
						case 256:	
						case 512:
							list_del(&slab->list);
							kmalloc_slab_cache[i].total_free -= slab->color_count;
							page_clean(slab->page);
							free_pages(slab->page,1);
							break;
						default:
							list_del(&slab->list);
							kmalloc_slab_cache[i].total_free -= slab->color_count;
							kfree(slab->color_map);
							page_clean(slab->page);
							free_pages(slab->page,1);
							kfree(slab);
							break;
					}
				}
				return 1;
			} else {
				slab = CONTAINER_OF(list_next(&slab->list),struct Slab,list);
            }

		} while (slab != kmalloc_slab_cache[i].cache_pool);
	
	}
	
	printk("kfree() ERROR: can`t free memory\n");
	
	return 0;
}


/**
 * 使用Slab资源请求内存区域的方法
 * @param size 请求的内存资源大小， 这个大小不能超过我们最大的 slab 块的大小
 * @param gfp_flags 暂时保留
 * 
 * @return 返回一个地址，表示请求到的地址，如果返回的地址是NULL那么就说明没有请求到有效地址，大概率是因为内存中已经没有可用的空间了
 */
void * kmalloc(unsigned long size,unsigned long gfp_flages) {
	int i,j;
	struct Slab * slab = NULL;

    // 确定在哪个内存池中进行搜索

	if (size > 1048576) {  // 如果申请的内存大小大于我们能够接受的最大大小，那么就结束
		printk("kmalloc() ERROR: kmalloc size too long:%08d\n",size);
		return NULL;
	}
	for(i = 0;i < 16;i++) {  // 试探能够使用的大小，找到比请求的大的里面最小的
        if (kmalloc_slab_cache[i].size >= size) {  // 找到就停止
            break;
        }
    }
    
    // 在内存池中进行搜索

	slab = kmalloc_slab_cache[i].cache_pool; 
	if (kmalloc_slab_cache[i].total_free != 0) { // 如果当前的SlabCache中有空闲单元，那么就找到这个空闲单元所属的Slab
		do {
			if (slab->free_count == 0)
				slab = CONTAINER_OF(list_next(&slab->list),struct Slab,list);
			else
				break;
		}while(slab != kmalloc_slab_cache[i].cache_pool);	
	} else {  // 如果当前的SlabCache没有空闲单元那么就直接为当前的SlabCache添加一个Slab单元
		slab = kmalloc_create(kmalloc_slab_cache[i].size);
		if (slab == NULL) {  // 如果没有请求到相应的Slab那么就说明内存地址不足，那么就返回错误
			printk("kmalloc()->kmalloc_create()=>slab == NULL\n");
			return NULL;
		}
        // 否则就将请求到的Slab添加到当前的链表中
		kmalloc_slab_cache[i].total_free += slab->color_count;
		printk("kmalloc()->kmalloc_create()<=size:%#010x\n",kmalloc_slab_cache[i].size);///////
		list_add_to_before(&kmalloc_slab_cache[i].cache_pool->list,&slab->list);
	}

    // 当前的Slab是有空闲的Slab，在当前的Slab中进行搜索即可
	for(j = 0;j < slab->color_count;j++) {
		if (*(slab->color_map + (j >> 6)) == 0xffffffffffffffffUL) {
			j += 63;
			continue;
		}
		if ( (*(slab->color_map + (j >> 6)) & (1UL << (j % 64))) == 0 ) {
			*(slab->color_map + (j >> 6)) |= 1UL << (j % 64);
			slab->using_count++;
			slab->free_count--;
			kmalloc_slab_cache[i].total_free--;
			kmalloc_slab_cache[i].total_using++;
			return (void *)((char *)slab->virtual_address + kmalloc_slab_cache[i].size * j);
		}
	}

	printk("kmalloc() ERROR: no memory can alloc\n");
	return NULL;
}


/**
 * 用于删除一个SlabCache
 * @param slab_cache 被删除的SlabCache
 */
unsigned long SlabDestroy(struct SlabCache * slab_cache) {

    if (slab_cache->total_using != 0) { // 如果有正在被使用的Slab，那么就不能删除
        printk("slab_cache->total_using != 0\n \
                SlabDestroy could only Delete a Unused SlabCache!\n");
        return 0;
    }

    struct Slab * slab_p = slab_cache->cache_pool;
    struct Slab * tmp_slab = NULL;
    while (!list_is_empty(&slab_p->list)) { // 通过链表遍历删除每一个被管理的Slab
        tmp_slab = slab_p;
        slab_p = CONTAINER_OF(list_next(&slab_p->list),struct Slab,list);
        list_del(&tmp_slab->list);  // 删除当前链表的前后关系
        kfree(tmp_slab->color_map);  // 删除当前Slab管理的color_map
        page_clean(tmp_slab->page);  // 清空当前页
        free_pages(tmp_slab->page,1);  // 释放相应的内存
        kfree(tmp_slab); // 释放当前Slab占用的内存空间
    }

    // 删除最后一个Slab对应的空间
    kfree(slab_p->color_map);
    page_clean(slab_p->page);
    free_pages(slab_p->page,1);
    kfree(slab_p);

    // 删除当前 SlabCache 占用的空间
    kfree(slab_cache);
    return 1;
}

/**
 * 新建一个slab，需要给出新建的slab的大小、以及用于输出错误位置的字符串
 * @param slab_size 一个无符号长整数，表示新建slab的大小
 * @param origin_stirng 用于定位错误的字符串，用于标记谁调用了本函数
 */
struct Slab* CreateSlab(unsigned long slab_size, const char origin_stirng[]) {
    struct Slab* res_slab = (struct Slab *)kmalloc(sizeof(struct Slab),0); // 用于返回的Slab

    if (res_slab == NULL) { // 如果没有申请到空间，那么就直接返回
        printk("%s->kmalloc()=>res_slab == NULL\n\
                 Memory is not enough!\n", origin_stirng); 
        return NULL;
    }

    memset(res_slab,0,sizeof(struct Slab)); // 清空当前区域，进行初始化
    list_init(&res_slab->list);
    res_slab->page = AllocPages(ZONE_NORMAL,1,0);

    if (res_slab->page == NULL) { // 如果请求空间失败，那么就释放空间并且返回
        printk("%s->AllocPages()=>res_slab->page == NULL\n\
                 Memory is not enough!\n", origin_stirng);
        kfree(res_slab);
        return NULL;
    }
    PageInit(res_slab->page,PG_Kernel);
    *res_slab = (struct Slab) { // 初始化
        res_slab->list,  // list
        res_slab->page,  // 本Slab管理的页
        0, PAGE_2M_SIZE/slab_size,  // using_count， free_count
        CONVERT_PHYSICAL_ADDRESS_TO_VIRTUAL_ADDRESS(res_slab->page->PHY_address),  // virtual_address
        res_slab->free_count,  // color_count
        ((res_slab->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3,  // color_length
        (unsigned long*)kmalloc(res_slab->color_length,0)  // color_map
    };

    if (res_slab->color_map == NULL) { // 如果请求空间失败，那么就释放空间并且返回
        printk("%s->kmalloc()=>res_slab->color_map == NULL\n\
                 Memory is not enough!\n", origin_stirng);
        free_pages(res_slab->page,1);
        kfree(res_slab);
        return NULL;
    }

    memset(res_slab->color_map,0,res_slab->color_length);

    return res_slab;
}


/**
 * 给出一个SlabCache管理的内存单元的大小，以及其构造函数、析构函数，创建并初始化一个SlabCache
 * @param size          新建一个Slab内存池
 * @param constructor   一个函数指针，指向一个构造函数
 * @param destructor    一个函数指针，指向一个析构函数
 * @param arg           给出的参数
 */
struct SlabCache * CreateSlabCache(
    unsigned long size,
    void *(* constructor)(void * virtual_address,unsigned long arg),
    void *(* destructor) (void * virtual_address,unsigned long arg),
    unsigned long arg) {
    
    // 为SlabCache申请相应的内存
    struct SlabCache * slab_cache = NULL;
    slab_cache = (struct SlabCache *)kmalloc(sizeof(struct SlabCache),0);

    if (slab_cache == NULL) { // 如果没有申请到内存的话，那么就报错退出
        printk("CreateSlabCache()->kmalloc()=>slab_cache == NULL!!!\n \
                Memory is not enough!\n");
        return NULL;
    }

    // 初始化新建 slab_cache  
    memset(slab_cache,0,sizeof(struct SlabCache));
    *slab_cache = (struct SlabCache) {
        SIZEOF_LONG_ALIGN(size),  // size
        0, 0,  // using, free
        (struct Slab *)kmalloc(sizeof(struct Slab),0),  // cache_poll
        NULL,   // cache_dma_poll
        constructor, destructor  // constructor, destructor function pointer
    };

    if (slab_cache->cache_pool == NULL) { // 如果没有申请到cache_pool的空间，那么报错
        printk("CreateSlabCache()->kmalloc()=>slab_cache->cache_pool == NULL!!!\n\
                 Memory is not enough!\n");
        kfree(slab_cache); // 将前面申请的空间释放
        return NULL;
    }
    // 初始化被管理的Slab
    memset(slab_cache->cache_pool, 0, sizeof(struct Slab));
    list_init(&slab_cache->cache_pool->list); // 对管理的Slab链表头进行初始化

    // 为创建的Slab分配内存
    slab_cache->cache_pool->page = AllocPages(ZONE_NORMAL,1,0);
    if (slab_cache->cache_pool->page == NULL) {   
        printk("CreateSlabCache()->AllocPages()=>slab_cache->cache_pool->page == NULL!!!\n \
                Memory is not enough!\n");
        // 将前期分配的内存释放
        kfree(slab_cache->cache_pool); 
        kfree(slab_cache);
        return NULL;
    }

    PageInit(slab_cache->cache_pool->page, PG_Kernel); // 对申请到的页进行初始化

    (*slab_cache->cache_pool) = (struct Slab) {
        slab_cache->cache_pool->list,  // list
        slab_cache->cache_pool->page,  // 本Slab管理的页
        0, PAGE_2M_SIZE/slab_cache->size,  // using_count， free_count
        CONVERT_PHYSICAL_ADDRESS_TO_VIRTUAL_ADDRESS(slab_cache->cache_pool->page->PHY_address),  // virtual_address
        slab_cache->cache_pool->free_count,  // color_count
        ((slab_cache->cache_pool->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3,  // color_length
        (unsigned long*)kmalloc(slab_cache->cache_pool->color_length,0)  // color_map
    };

    slab_cache->total_free = slab_cache->cache_pool->free_count;

    if (slab_cache->cache_pool->color_map == NULL) { // 如果没有申请到color_map空间，那么就报错并且清空
        printk("CreateSlabCache()->kmalloc()=>slab_cache->cache_pool->color_map == NULL\n\
                Memory is not enough!\n");
        // 将前期分配的内存释放
        free_pages(slab_cache->cache_pool->page,1);
        kfree(slab_cache->cache_pool);
        kfree(slab_cache);
        return NULL;
    }
    // 清空申请到的数据
    memset(slab_cache->cache_pool->color_map, 0, slab_cache->cache_pool->color_length);

    return slab_cache;
}


/**
 * 向某一个SlabCache内存池请求一个内存区域（请求区域大小由slab_cache->size确定）
 * @param slab_cache 请求的内存池
 * @param arg 请求的参数
 */
void * SlabMalloc(struct SlabCache* slab_cache, unsigned long arg) {
	struct Slab * slab_p = slab_cache->cache_pool;
	struct Slab * tmp_slab = NULL;
	int j = 0;

	if (slab_cache->total_free == 0) { // 如果没有可用的页
        struct Slab* tmp_slab = CreateSlab(slab_cache->size, "SlabMalloc()");
        if (tmp_slab == NULL) return NULL; // 如果出现错误，那么就直接返回（在出现错误的地方已经报过错了）
        list_add_to_behind(&slab_cache->cache_pool->list,&tmp_slab->list);
		slab_cache->total_free  += tmp_slab->color_count;
		for(j = 0;j < tmp_slab->color_count;j++) { // 进行搜寻
			if ( (*(tmp_slab->color_map + (j >> 6)) & (1UL << (j % 64))) == 0 ) {
				*(tmp_slab->color_map + (j >> 6)) |= 1UL << (j % 64);
				tmp_slab->using_count++;
				tmp_slab->free_count--;
				slab_cache->total_using++;
				slab_cache->total_free--;
				if (slab_cache->constructor != NULL) {
					return slab_cache->constructor((char *)tmp_slab->virtual_address + slab_cache->size * j,arg);
				} else {			
					return (void *)((char *)tmp_slab->virtual_address + slab_cache->size * j);
				}		
			}
		}
	} else { // 如果当前有足够的空间
		do {
			if (slab_p->free_count == 0) { // 如果当前的 slab 没有闲暇的地址空间，那么就根据链表，移动到下一个Slab结构体
				slab_p = CONTAINER_OF(list_next(&slab_p->list),struct Slab,list);
				continue;
			}
			for (j = 0;j < slab_p->color_count;j++) {
				if (*(slab_p->color_map + (j >> 6)) == 0xffffffffffffffffUL) { // 如果当前的color_map元素全是1，那么就直接跳过当前的64个元素，直接找下面的64个
					j += 63;
					continue;
                }
				if ( (*(slab_p->color_map + (j >> 6)) & (1UL << (j % 64))) == 0 ) { // 如果当前检测的位有空间的话，那么就返回相应的空间
					*(slab_p->color_map + (j >> 6)) |= 1UL << (j % 64); // 将找到的空间置为1

                    // 对计数进行更改
					slab_p->using_count++;
					slab_p->free_count--;
					slab_cache->total_using++;
					slab_cache->total_free--;

					if (slab_cache->constructor != NULL) { // 如果当前的slab_cache的构造函数不为空，那么就自动调用这个构造函数，并且返回结果
						return slab_cache->constructor((char *)slab_p->virtual_address + slab_cache->size * j,arg);
					} else { // 否则直接将结果返回
						return (void *)((char *)slab_p->virtual_address + slab_cache->size * j);
					}
				}
			}
		} while (slab_p != slab_cache->cache_pool);		
	} // 如果找到最后都没有找到的话，那么就说明实在找不到了，那么就报错吧
	printk("SlabMalloc() ERROR: can`t alloc\n");
	if (tmp_slab != NULL) { // 如果申请了内存，但是还是出现了错误，那么就释放相应内存并且退出
		list_del(&tmp_slab->list);
		kfree(tmp_slab->color_map);
		page_clean(tmp_slab->page);
		free_pages(tmp_slab->page,1);
		kfree(tmp_slab);
	}
	return NULL;
}

/**
 * 释放某一内存池子中特定虚拟地址的内存
 * @param slab_cache 被释放的内存池
 * @param address 被释放的地址
 * @param arg 释放时使用的参数
 */
unsigned long SlabFree(struct SlabCache * slab_cache,void * address,unsigned long arg) {
    struct Slab * slab_p = slab_cache->cache_pool;
    int index = 0;

    do {
        if (slab_p->virtual_address <= address && address < slab_p->virtual_address + PAGE_2M_SIZE) {
            index = (address - slab_p->virtual_address) / slab_cache->size;
            *(slab_p->color_map + (index >> 6)) ^= 1UL << index % 64;
            slab_p->free_count++;
            slab_p->using_count--;
            slab_cache->total_using--;
            slab_cache->total_free++;
            if (slab_cache->destructor != NULL) {
                slab_cache->destructor((char *)slab_p->virtual_address + slab_cache->size * index,arg);
            }
            if ((slab_p->using_count == 0) && (slab_cache->total_free >= slab_p->color_count * 3 / 2)) {
                list_del(&slab_p->list);
                slab_cache->total_free -= slab_p->color_count;
                kfree(slab_p->color_map);
                page_clean(slab_p->page);
                free_pages(slab_p->page,1);
                kfree(slab_p);
            }
            return 1;
        } else {
            slab_p = CONTAINER_OF(list_next(&slab_p->list),struct Slab,list);
            continue;
        }
    } while (slab_p != slab_cache->cache_pool);

    printk("slab_free() ERROR: address not in slab\n");
    return 0;
}


/**
 * 对内置的SlabCache进行Init操作
 */
unsigned long SlabInit() {
    
    unsigned long struct_from_page = (memory_management_struct.end_of_struct >> PAGE_2M_SHIFT);
    // 遍历，为cache_pool请求空间
    for(int i = 0; i < 16; ++i) {
        // 直接在memory_management_struct后面手动开空间， 分配给Slab
        kmalloc_slab_cache[i].cache_pool = (struct Slab*)memory_management_struct.end_of_struct;
        memory_management_struct.end_of_struct += sizeof(struct Slab) + 10 * sizeof(long); // 该空间被使用向后移动，防止冲突
        list_init(&kmalloc_slab_cache[i].cache_pool->list);
        // 为color_Map分配空间，每一个Slab初始的大小都为y一个页的大小，即为2M，那么colormap的位长度就是(PageSize/SlabSize)
        // 占用的long 类型数量为(PageSize/SlabSize) << 6
        *kmalloc_slab_cache[i].cache_pool = (struct Slab) {
            kmalloc_slab_cache[i].cache_pool->list,  // list
            NULL, 0, (PAGE_2M_SIZE / kmalloc_slab_cache[i].size),  // page_pointer, using_count, free_count
            NULL, (((PAGE_2M_SIZE / kmalloc_slab_cache[i].size + (1 << 6) - 1) >> 6) << 3), // virtual_address, color_length
            (PAGE_2M_SIZE / kmalloc_slab_cache[i].size), (unsigned long*)memory_management_struct.end_of_struct // color_count, color_map
        };
        // 后移end_of_struct，为color_Map腾出空间
        memory_management_struct.end_of_struct = memory_management_struct.end_of_struct +  //  加上当前colormap的地址长度，然后腾出空间防止冲突，最后进行对齐
                                                 (kmalloc_slab_cache[i].cache_pool->color_length + sizeof(long) * 10) & (~ (sizeof(long) - 1));
        // 分配cache_pool后，为当前Slab池初始化参数
        kmalloc_slab_cache[i] = (struct SlabCache) {
            kmalloc_slab_cache[i].size,  // size
            0, (PAGE_2M_SIZE / kmalloc_slab_cache[i].size), // total_using, total_free
            kmalloc_slab_cache[i].cache_pool, NULL,  // cache_pool, cache_dma_pool
            NULL, NULL  // constructor, destructor
        };

		// 清空申请的color_map
		memset(kmalloc_slab_cache[i].cache_pool->color_map,0xff,kmalloc_slab_cache[i].cache_pool->color_length);

		for(int j = 0;j < kmalloc_slab_cache[i].cache_pool->color_count;j++)
			*(kmalloc_slab_cache[i].cache_pool->color_map + (j >> 6)) ^= 1UL << j % 64;

    }
    // 将这些使用的空间进行标记
    unsigned long long struct_to_page = (memory_management_struct.end_of_struct >> PAGE_2M_SHIFT);
    for(int i = struct_from_page; i <= struct_to_page; ++i) { // 将使用到的Page进行标记
        struct Page* page =  memory_management_struct.pages_struct + i;
		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
		page->zone_struct->page_using_count++;
		page->zone_struct->page_free_count--;
		PageInit(page,PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);
    }
    // 为Slab分配实际控制的Page
    for(int i = 0 ;i < 16; ++i) {
        unsigned long*  virtual_address = (unsigned long *)((memory_management_struct.end_of_struct + PAGE_2M_SIZE * i + PAGE_2M_SIZE - 1) & PAGE_2M_MASK);
		struct Page* page = Virt_To_2M_Page(virtual_address);
		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
		page->zone_struct->page_using_count++;
		page->zone_struct->page_free_count--;
		PageInit(page,PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);
		kmalloc_slab_cache[i].cache_pool->page = page;
		kmalloc_slab_cache[i].cache_pool->virtual_address = virtual_address;
    }

    printk("SlabCaches Init Finished!");
    printk(
        "start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,end_brk:%#018lx,end_of_struct:%#018lx\n",
        memory_management_struct.start_code,memory_management_struct.end_code,memory_management_struct.end_data,
        memory_management_struct.end_brk, memory_management_struct.end_of_struct
    );
    return 1;
}

