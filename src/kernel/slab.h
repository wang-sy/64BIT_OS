#ifndef _64BITOS_SRC_KERNEL_SLAB_H
#define _64BITOS_SRC_KERNEL_SLAB_H

#include "lib.h"

/**
 * 用于描述一个Slab基本管理单元
 * @param list              用于记录Slab的前后关系
 * @param page              本Slab管理的页
 * @param using_count       本Slab中已经被使用的块的数量
 * @param free_count        本Slab中可用的块的数量
 * @param virtual_address   本的起始线性地址
 * @param color_length      colormap的地址长度
 * @param color_count       colormap的单元个数
 * @param color_map         colormap的主体
 */
struct Slab {
    struct List list;  // 用于记录Slab的前后关系
    struct Page * page;  // 本Slab管理的页
    unsigned long using_count;  // 本Slab中已经被使用的块的数量
    unsigned long free_count;  // 本Slab中可用的块的数量
    void * virtual_address;  // 本的起始线性地址
    unsigned long color_length;  // colormap的地址长度
    unsigned long color_count;  // colormap的单元个数
    unsigned long * color_map;  // colormap的主体
};

/** 
 * 描述Slab池的结构体
 * @param size              当前Slab池中，基本块的大小
 * @param total_using       当前Slab池中，被使用的块的数量
 * @param total_free        当前Slab池中，空闲的块的数量
 * @param cache_pool        指向当前Slab池管理的第一个Slab块
 * @param cache_dma_pool    暂时保留
 * @param constructor       块的构造函数
 * @param destructor        块的析构函数
 */
struct SlabCache {
    unsigned long    size;  // 当前Slab池中，基本块的大小
    unsigned long    total_using;  // 当前Slab池中，被使用的块的数量
    unsigned long    total_free;  // 当前Slab池中，空闲的块的数量
    struct Slab *    cache_pool;  // 指向当前Slab池管理的第一个Slab块
    struct Slab *    cache_dma_pool;  // 暂时保留
    void *(* constructor)(void * visual_address, unsigned long arg);  // 块的构造函数
    void *(* destructor) (void * visual_address, unsigned long arg);  // 块的析构函数
};

extern struct SlabCache kmalloc_slab_cache[16];

struct Slab * kmalloc_create(unsigned long size);

/**
 * 释放一个由Slab管理的内存区域
 * @param address 想要释放的区域的首地址
 * @return 如果成功，那么返回1, 否则返回0
 */
unsigned long kfree(void * address);

/**
 * 使用Slab资源请求内存区域的方法
 * @param size 请求的内存资源大小， 这个大小不能超过我们最大的 slab 块的大小
 * @param gfp_flags 暂时保留
 * 
 * @return 返回一个地址，表示请求到的地址，如果返回的地址是NULL那么就说明没有请求到有效地址，大概率是因为内存中已经没有可用的空间了
 */
void * kmalloc(unsigned long size,unsigned long gfp_flages);

/**
 * 用于删除一个SlabCache
 * @param slab_cache 被删除的SlabCache
 */
unsigned long SlabDestroy(struct SlabCache * slab_cache);

/**
 * 新建一个slab，需要给出新建的slab的大小、以及用于输出错误位置的字符串
 * @param slab_size 一个无符号长整数，表示新建slab的大小
 * @param origin_stirng 用于定位错误的字符串，用于标记谁调用了本函数
 */
struct Slab* CreateSlab(unsigned long slab_size, const char origin_stirng[]);

/**
 * 给出一个SlabCache管理的内存单元的大小，以及其构造函数、析构函数，创建并初始化一个SlabCache
 * @param size          新建一个Slab内存池
 * @param constructor   一个函数指针，指向一个构造函数
 * @param destructor    一个函数指针，指向一个析构函数
 * @param arg           给出的参数
 */
struct SlabCache * CreateSlabCache( unsigned long size, void *(* constructor)(void * virtual_address,unsigned long arg), void *(* destructor) (void * virtual_address,unsigned long arg), unsigned long arg);

/**
 * 向某一个SlabCache内存池请求一个内存区域（请求区域大小由slab_cache->size确定）
 * @param slab_cache 请求的内存池
 * @param arg 请求的参数
 */
void * SlabMalloc(struct SlabCache* slab_cache, unsigned long arg) ;

/**
 * 释放某一内存池子中特定虚拟地址的内存
 * @param slab_cache 被释放的内存池
 * @param address 被释放的地址
 * @param arg 释放时使用的参数
 */
unsigned long SlabFree(struct SlabCache * slab_cache,void * address,unsigned long arg);

/**
 * 对内置的SlabCache进行Init操作
 */
unsigned long SlabInit();


#endif