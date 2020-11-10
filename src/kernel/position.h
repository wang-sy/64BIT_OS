#ifndef _64BITOS_SRC_KERNEL_POSITION_H_
#define _64BITOS_SRC_KERNEL_POSITION_H_

/* =========================================================================
 =                                结构体定义                                 =
 =========================================================================*/

/**
 * 结构体，用于描述当前指针位置
 * @param x_resolution 屏幕行数
 * @param y_resolution 屏幕列数
 * @param x_position 当前光标左上角所在位置
 * @param y_position 当前光标右上角所在位置
 * @param x_char_size 每个字符所占位置的行数
 * @param y_char_size 每个字符所占位置的列数
 * @param screen_buffer_base_address 显示缓冲区的首地址
 * @param screen_buffer_length 显示缓冲区的长度
 */
struct Position {
    int x_resolution; // 屏幕行数
    int y_resolution; // 屏幕列数

    int x_position; // 当前光标左上角所在位置
    int y_position; // 当前光标右上角所在位置

    int x_char_size; // 每个字符所占位置的行数
    int y_char_size; // 每个字符所占位置的列数

    unsigned int * screen_buffer_base_address; // 显示缓冲区的首地址
    unsigned long screen_buffer_length; // 显示缓冲区的长度
};
/*引入本头文件时自动引入本变量*/
extern struct Position global_position;

/* =========================================================================
 =                                函数声明                                   =
 =========================================================================*/

/**
 * 给出当前的位置结构体，将光标移动到下一区域
 * @param cur_position 一个指针，指向被操作的位置结构体
 */ 
void DoNext(struct Position * cur_position);

/**
 * 给出当前的位置结构体，模拟回车时的操作
 * @param cur_position 一个指针，指向被操作的位置结构体
 */ 
void DoEnter(struct Position * cur_position);

/**
 * 给出当前的位置结构体，在当前位置模拟退格键[退格键不会将回车删除（也就是说，无论怎么退格，Y值都不会变）]
 * @param cur_position 一个指针，指向被操作的位置结构体
 */ 
void DoBackspace(struct Position * cur_position);

/**
 * 给出当前的位置结构体，在当前位置模拟输入一次制表符
 * 这里无论如何都要做一次，然后直到对齐4位为止
 * @param cur_position 一个指针，指向被操作的位置结构体
 */ 
void DoTab(struct Position * cur_position);

/**
 * 给出当前的位置结构体，将光标置为(0, 0)[暂时不实现清空屏幕的功能]
 * @param cur_position 一个指针，指向被操作的位置结构体
 */ 
void DoClear(struct Position * cur_position);

/**
 * 给出当前的位置结构体，以及想要在当前位置输出的文字的背景颜色、文字颜色、文字格式在当前位置进行输出
 * @param cur_position 一个指针，指向被操作的位置结构体
 * @param back_ground_color 背景颜色
 * @param font_color 字体颜色
 * @param char_format  字体样式会根据字体样式进行颜色填充
 */ 
void DoPrint(struct Position * cur_position,const int back_ground_color, const int font_color, const char* char_format);
#endif