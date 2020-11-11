/**
 * MIT License
 *
 * Copyright (c) 2020 王赛宇

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @author wangsaiyu@cqu.edu.cn
 * 本文件内容实现了printk.h中定义的方法，主要负责在控制台上的输出
 */

/* =========================================================================
 =                                 头的引入                                  =
 =========================================================================*/

#include <stdarg.h>
#include "printk.h"
#include "lib.h"
#include "position.h"
#include "font.h"

/* =========================================================================
 =                                 宏的定义                                  =
 =========================================================================*/

// pad with zero
#define ZERO_PAD	1
// unsigned/signed long
#define SIGN	2
// show plus
#define PLUS	4
// space if plus
#define SPACE	8
// left justified
#define LEFT	16
// 0x
#define SPECIAL	32
// use 'abcdef' instead of 'ABCDEF'
#define SMALL	64

// ================== 方法宏 ==================

/**
 * 计算 n / base， 用于格式化字符串时计算格式转换
 * @param n: 被除数
 * @param base: 除数
 */
#define DO_DIV(n,base) ({ \
int __res; \
__asm__("divq %%rcx":"=a" (n),"=d" (__res):"0" (n),"1" (0),"c" (base)); \
__res; })

/* =========================================================================
 =                          函数的定义与实现                                  =
 =========================================================================*/

/**
 * 给出一段字符串，将字符串转换为数字
 * @param s 输入的字符串
 * @return 返回的数字， 整数类型
 */
int SkipAtoi(const char **s){
	int i=0;
	while (IS_DIGIT(**s)){
        i = i*10 + *((*s)++) - '0';
	}
	return i;
}

/**
 * 根据给出的参数，格式化输出一个数字
 * @param str 进行操作的字符串
 * @param num 被格式化的数字
 * @param base 转换成 base 进制
 * @param size 大小
 * @param precision 数据的宽度
 * @param type 表示格式
 * @return 一个字符串，格式化完成后的该数字
 */
static char * ConvertNumber(char * str, long num, int base, int size, int precision, int type) {
	char c,sign,tmp[50];
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	if (type & SMALL) { // 判断是否是 SMALL
	    digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	}
	if (type & LEFT) {
	    type &= ~ZERO_PAD;
	}
	if (base < 2 || base > 36) {
        return 0;
	}
	c = (type & ZERO_PAD) ? '0' : ' ' ;
	sign = 0;
	if (type&SIGN && num < 0) {
		sign='-';
		num = -num;
	} else {
        sign=(type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
	}

	if (sign) {
	    size--;
	}
	if (type & SPECIAL) {
	    if (base == 16) {
	        size -= 2;
	    } else if (base == 8) {
            size--;
        }
	}

	i = 0;
	if (num == 0) {
	    tmp[i++]='0';
	}
	else {
	    while (num!=0) {
            tmp[i++]=digits[DO_DIV(num,base)];
	    }
	}

	if (i > precision) {
	    precision=i;
	}
	size -= precision;
	if (!(type & (ZERO_PAD + LEFT))) {
	    while(size-- > 0) {
            *str++ = ' ';
	    }
	}
	if (sign) {
	    *str++ = sign;
	}
	if (type & SPECIAL) {
        if (base == 8) {
            *str++ = '0';
        } else if (base==16) {
            *str++ = '0';
            *str++ = digits[33];
        }
	}
	if (!(type & LEFT)) {
        while(size-- > 0) {
            *str++ = c;
        }
	}
	while(i < precision--) {
	    *str++ = '0';
	}
	while(i-- > 0) {
	    *str++ = tmp[i];
	}
	while(size-- > 0) {
	    *str++ = ' ';
	}
	return str;
}



/**
 * 对给出的字符串进行初始化（识别占位符并且根据需求进行填入）
 * @param saving_buffer 格式化后的字符串将返回到buf数组中
 * @param format_string 一个字符串，表示需要进行格式化的字符串如："numa : %06d !\n"
 * @param args 用户输入的参数，用于对前面的字符串进行填充
 * @return 格式化后字符串的结束位置
 */
int vsprintf(char * saving_buffer,const char *format_string, va_list args){
	char * str,*s;
	int flags;
	int field_width;
	int precision;
	int len,i;

	int qualifier;		/* 'h', 'l', 'L' or 'Z' for integer fields */

	for(str = saving_buffer; *format_string; format_string++){
		if (*format_string != '%') {
			*str++ = *format_string;
			continue;
		}
		flags = 0;
		repeat:
			format_string++;
			switch (*format_string) {
				case '-':{
					flags |= LEFT;	
					goto repeat;
				}
				case '+':{
					flags |= PLUS;	
					goto repeat;
				}
				case ' ':{
					flags |= SPACE;	
					goto repeat;
				}
				case '#':{
					flags |= SPECIAL;	
					goto repeat;
				}
				case '0':{
					flags |= ZERO_PAD;	
					goto repeat;
				}
                default: break;
			}

			/* 获取数字 */
			field_width = -1;
			if (IS_DIGIT(*format_string)){
				field_width = SkipAtoi(&format_string);
			} else if (*format_string == '*') {
				format_string++;
				field_width = va_arg(args, int);
				if (field_width < 0) {
					field_width = -field_width;
					flags |= LEFT;
				}
			}

			precision = -1;
			if (*format_string == '.') {
				format_string++;
				if (IS_DIGIT(*format_string)){
                    precision = SkipAtoi(&format_string);
				} else if (*format_string == '*') {
					format_string++;
					precision = va_arg(args, int);
				}
				if (precision < 0) {
                    precision = 0;
                }
			}
			
			qualifier = -1;
			if (*format_string == 'h' || *format_string == 'l' ||
			    *format_string == 'L' || *format_string == 'Z') {
				qualifier = *format_string;
				format_string++;
			}
			switch(*format_string) {
				case 'c':{
                    if (!(flags & LEFT)){
                        while(--field_width > 0) {
                            *str++ = ' ';
                        }
                    }
                    *str++ = (unsigned char)va_arg(args, int);
                    while(--field_width > 0){
                        *str++ = ' ';
                    }
                    break;
				}
				case 's':{
                    s = va_arg(args,char *);
                    if (!s){
                        s = '\0';
                    }
                    len = strlen(s);
                    if (precision < 0){
                        precision = len;
                    } else if (len > precision){
                        len = precision;
                    }
                    if (!(flags & LEFT)){
                        while(len < field_width--){
                            *str++ = ' ';
                        }
                    }

                    for(i = 0;i < len ;i++){
                        *str++ = *s++;
                    }
                    while(len < field_width--){
                        *str++ = ' ';
                    }
                    break;
				}
				case 'o':
					
					if (qualifier == 'l') {
                        str = ConvertNumber(str,va_arg(args,unsigned long),8,field_width,precision,flags);
					} else {
                        str = ConvertNumber(str,va_arg(args,unsigned int),8,field_width,precision,flags);
					}
					break;

				case 'p':{
                    if (field_width == -1) {
                        field_width = 2 * sizeof(void *);
                        flags |= ZERO_PAD;
                    }
                    str = ConvertNumber(str,(unsigned long)va_arg(args,void *),16,field_width,precision,flags);
                    break;
				}

				case 'x':{
                    flags |= SMALL;
				}
				case 'X':{
                    if (qualifier == 'l') {
                        str = ConvertNumber(str,va_arg(args,unsigned long),16,field_width,precision,flags);
                    } else {
                        str = ConvertNumber(str,va_arg(args,unsigned int),16,field_width,precision,flags);
                    }
                    break;
				}
				case 'd':
				case 'i':{
                    flags |= SIGN;
				}
				case 'u':{
                    if (qualifier == 'l') {
                        str = ConvertNumber(str,va_arg(args,unsigned long),10,field_width,precision,flags);
                    } else {
                        str = ConvertNumber(str,va_arg(args,unsigned int),10,field_width,precision,flags);
                    }
                    break;
				}
				case 'n':{
                    if (qualifier == 'l') {
                        long *ip = va_arg(args,long *);
                        *ip = (str - saving_buffer);
                    } else {
                        int *ip = va_arg(args,int *);
                        *ip = (str - saving_buffer);
                    }
                    break;
				}
				case '%':{
                    *str++ = '%';
                    break;
				}
				default:{
                    *str++ = '%';
                    if (*format_string)
                        *str++ = *format_string;
                    else
                        format_string--;
                    break;
				}
			}

	}
	*str = '\0';
	return str - saving_buffer;
}

/* =========================================================================
 =                        printk.h中函数的实现                               =
 =========================================================================*/

/*给出颜色与需要输出的东西，进行输出
其实现借助了 vsprintf 对字符串进行格式化后借用 Position 进行屏幕输出*/
int color_printk(unsigned int front_color,unsigned int back_ground_color,const char * format_string,...) {
    char saving_buffer[500];
	va_list args;
	va_start(args, format_string);

	int end_position = vsprintf(saving_buffer, format_string, args);

	va_end(args);

	for(int count = 0;count < end_position;count++) {
		if (saving_buffer[count] == '\n') DoEnter(&global_position);
		else if (saving_buffer[count] == '\b') DoBackspace(&global_position);
		else if (saving_buffer[count] == '\t') DoTab(&global_position);
		else DoPrint(&global_position, front_color, back_ground_color, font_ascii[saving_buffer[count]]);
        DoNext(&global_position);
	}
	return end_position;
}

/* 可以直接拿来当printf来用
其实现借助了 vsprintf 对字符串进行格式化后借用 Position 进行屏幕输出*/
int printk(const char * format_string,...){
    char saving_buffer[500];
	va_list args;
	va_start(args, format_string);
	int end_position = vsprintf(saving_buffer,format_string, args);
	va_end(args);

	for(int count = 0;count < end_position;count++) {
		if (saving_buffer[count] == '\n') DoEnter(&global_position);
		else if (saving_buffer[count] == '\b') DoBackspace(&global_position);
		else if (saving_buffer[count] == '\t') DoTab(&global_position);
		else DoPrint(&global_position, BLACK, WHITE, font_ascii[saving_buffer[count]]);
        DoNext(&global_position);
	}
	return end_position;
}