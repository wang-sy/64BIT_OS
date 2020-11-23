#ifndef _64BITOS_SRC_KERNEL_PRINTK_H
#define _64BITOS_SRC_KERNEL_PRINTK_H

#include <stdarg.h>

/* =========================================================================
 =                                宏的定义                                  =
 =========================================================================*/

// ================== 颜色相关的宏 ==================

// 白
#define WHITE 	0x00ffffff
// 黑
#define BLACK 	0x00000000
// 红
#define RED	0x00ff0000
// 橙
#define ORANGE	0x00ff8000
// 黄
#define YELLOW	0x00ffff00
// 绿
#define GREEN	0x0000ff00
// 蓝
#define BLUE	0x000000ff
// 靛
#define INDIGO	0x0000ffff
// 紫
#define PURPLE	0x008000ff

// ================== 方法宏 ==================

/**
 * 判断给出的字符是否是一个数字
 * @param c 被判断的字符
 */
#define IS_DIGIT(c)	((c) >= '0' && (c) <= '9')


/* =========================================================================
 =                                函数定义                                  =
 =========================================================================*/

/**
 * 给出颜色与需要输出的东西，进行输出
 * @param FRcolor 一个整形数字，表示想要输出的字体颜色
 * @param BKcolor 一个整形数字，表示想要输出的背景颜色
 * @param format_string 一个字符串，表示用户输出的字符串
 * @param ... 可变长的参数列表，表示想要填充到字符串中的参数
 */
int color_printk(unsigned int front_color,unsigned int back_ground_color,const char * format_string,...);

/**
 * 可以直接拿来当printf来用
 */
int printk(const char * format_string,...);

#endif

