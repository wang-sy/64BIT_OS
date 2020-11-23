/***************************************************
*		版权声明
*
*	本操作系统名为：MINE
*	该操作系统未经授权不得以盈利或非盈利为目的进行开发，
*	只允许个人学习以及公开交流使用
*
*	代码最终所有权及解释权归田宇所有；
*
*	本模块作者：	田宇
*	EMail:		345538255@qq.com
*
*
***************************************************/

#ifndef _64BITOS_SRC_KERNEL_LIB_H
#define _64BITOS_SRC_KERNEL_LIB_H


#define NULL 0

/**
 * 通过结构体的成员来获取该结构体的首地址
 * @param ptr		结构体的成员的地址
 * @param type		结构体的类型
 * @param member	结构体的成员的名称
 * 
 * <h3>相关原理</h3>
 * <p>
 * 		<ul>
 * 			<li>使用：(unsigned long)&(((type *)0)->member计算出member相对于整个结构体的偏移量</li>
 * 			<li>使用当前成员的实际地址减掉当前成员的偏移量，得到当前结构体的首地址</li>
 * 		</ul>
 * </p>
 */
#define CONTAINER_OF(ptr,type,member)							\
({											\
	typeof(((type *)0)->member) * p = (ptr);					\
	(type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));		\
})


#define sti() 		__asm__ __volatile__ ("sti	\n\t":::"memory")
#define cli()	 	__asm__ __volatile__ ("cli	\n\t":::"memory")
#define nop() 		__asm__ __volatile__ ("nop	\n\t")
#define io_mfence() 	__asm__ __volatile__ ("mfence	\n\t":::"memory")


struct List
{
	struct List * prev;
	struct List * next;
};

void list_init(struct List * list);

void list_add_to_behind(struct List * entry,struct List * new);
void list_add_to_before(struct List * entry,struct List * new);

void list_del(struct List * entry);

long list_is_empty(struct List * entry);

struct List * list_prev(struct List * entry);

struct List * list_next(struct List * entry);

/*
		From => To memory copy Num bytes
*/

void * memcpy(void *From,void * To,long Num);

/*
		FirstPart = SecondPart		=>	 0
		FirstPart > SecondPart		=>	 1
		FirstPart < SecondPart		=>	-1
*/

int memcmp(void * FirstPart,void * SecondPart,long Count);
/*
		set memory at Address with C ,number is Count
*/

void * memset(void * Address,unsigned char C,long Count);

/*
		string copy
*/

char * strcpy(char * Dest,char * Src);
/*
		string copy number bytes
*/

char * strncpy(char * Dest,char * Src,long Count);

/*
		string cat Dest + Src
*/

char * strcat(char * Dest,char * Src);

/*
		string compare FirstPart and SecondPart
		FirstPart = SecondPart =>  0
		FirstPart > SecondPart =>  1
		FirstPart < SecondPart => -1
*/

int strcmp(char * FirstPart,char * SecondPart);

/*
		string compare FirstPart and SecondPart with Count Bytes
		FirstPart = SecondPart =>  0
		FirstPart > SecondPart =>  1
		FirstPart < SecondPart => -1
*/

int strncmp(char * FirstPart,char * SecondPart,long Count);

/*

*/

int strlen(char * String);

/*

*/

unsigned long bit_set(unsigned long * addr,unsigned long nr);
/*

*/

unsigned long bit_get(unsigned long * addr,unsigned long nr);

/*

*/

unsigned long bit_clean(unsigned long * addr,unsigned long nr);
/*

*/

unsigned char io_in8(unsigned short port);

/*

*/

unsigned int io_in32(unsigned short port);

/*

*/

void io_out8(unsigned short port,unsigned char value);

/*

*/

void io_out32(unsigned short port,unsigned int value);

void wrmsr(unsigned long address,unsigned long value);

/*

*/

#define port_insw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;insw;mfence;"::"d"(port),"D"(buffer),"c"(nr):"memory")

#define port_outsw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;outsw;mfence;"::"d"(port),"S"(buffer),"c"(nr):"memory")

#endif
