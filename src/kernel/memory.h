#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"


/**
 * 用于描述一个内存块的结构体
 * baseAddrHigh : baseAddrLow 拼接起来就是基地址
 * lengthHigh : lengthLow 拼接起来就是内存块长度
 * type 内存类型， 1 ：可被os使用， 2 ： 正在使用或不可使用 其他：没有意义
 */
struct Memory_Block_E820{
    unsigned int baseAddrLow;
    unsigned int baseAddrHigh;
    unsigned int lengthLow;
    unsigned int lengthHigh;
    unsigned int type;
};

// 从loader写入内存的内存块信息中读取内存信息，存储到结构体中
void init_memory();

#endif