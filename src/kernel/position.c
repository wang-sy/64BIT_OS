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
 * 本文件内容实现了position.h中定义的方法，完成了一套对屏幕上内容与光标位置操作的方法
 * 同时，声明了全局变量 global_position，用于管理目前存在的唯一控制台的显示内容与预输入位置
 */

/* =========================================================================
 =                                 头的引入                                  =
 =========================================================================*/

#include "position.h"
#include "lib.h"

/* =========================================================================
 =                                数据的声明                                 =
 =========================================================================*/

struct Position global_position;

// 实现position中的一些函数
char* defaultFill = "                                                                ";

/* =========================================================================
 =                        position.h中函数的实现                             =
 =========================================================================*/

/**
 * 给出当前的位置结构体，将光标移动到下一区域
 * @param cur_position 一个指针，指向被操作的位置结构体
 */ 
void DoNext(struct Position* cur_position){
    cur_position->y_position = cur_position->y_position + cur_position->y_char_size;
    if (cur_position->y_position >= cur_position->y_resolution) {
        DoEnter(cur_position); // 试探，如果错误，就直接重置
    } 
}

/**
 * 给出当前的位置结构体，模拟回车时的操作
 * @param cur_position 一个指针，指向被操作的位置结构体
 */ 
void DoEnter(struct Position * cur_position){
    cur_position->y_position = 0;
    cur_position->x_position = cur_position->x_position + cur_position->x_char_size; // 平移
    if (cur_position->x_position >= cur_position->x_resolution) DoClear(cur_position); // 试探，如果错误，就直接重置
}

/**
 * 给出当前的位置结构体，在当前位置模拟退格键[退格键不会将回车删除（也就是说，无论怎么退格，Y值都不会变）]
 * @param cur_position 一个指针，指向被操作的位置结构体
 */ 
void DoBackspace(struct Position* cur_position){
    // 先在当前位置画一个空格（把之前的字符覆盖掉） 画的时候背景是黑色
    DoPrint(cur_position, 0x00000000, 0x00000000, defaultFill);
    // 如果不是行的第一个，那么就减一个空位
    cur_position->y_position = (cur_position->y_position - cur_position->y_char_size <= 0) ? 0 : 
                        cur_position->y_position - cur_position->y_char_size; 
}

/**
 * 给出当前的位置结构体，在当前位置模拟输入一次制表符
 * 这里无论如何都要做一次，然后直到对齐4位为止
 * @param cur_position 一个指针，指向被操作的位置结构体
 */ 
void DoTab(struct Position * cur_position){
    do{
        DoNext(cur_position);
    } while(((cur_position->y_position / cur_position->y_char_size) & 4) == 0);
}

/**
 * 给出当前的位置结构体，将光标置为(0, 0)[暂时不实现清空屏幕的功能]
 * @param cur_position 一个指针，指向被操作的位置结构体
 */ 
void DoClear(struct Position * cur_position){
    cur_position->x_position = 0;
    cur_position->y_position = 0;
    memset(cur_position->screen_buffer_base_address, 0x00, cur_position->screen_buffer_length);
}

/**
 * 给出当前的位置结构体，以及想要在当前位置输出的文字的背景颜色、文字颜色、文字格式在当前位置进行输出
 * @param cur_position 一个指针，指向被操作的位置结构体
 * @param back_ground_color 背景颜色
 * @param font_color 字体颜色
 * @param char_format  字体样式会根据字体样式进行颜色填充
 */ 
void DoPrint(struct Position * cur_position,const unsigned int back_ground_color,
             const unsigned int font_color, const unsigned char* char_format){
    int row, col;
    // 遍历每一个像素块，进行输出
   for(row = cur_position->x_position; row < cur_position->x_position + cur_position->x_char_size; row ++){
        for(col = cur_position->y_position; col < cur_position->y_position + cur_position->y_char_size; col ++){
            int* pltAddr = cur_position->screen_buffer_base_address + cur_position->y_resolution * row + col; // 当前方块需要覆盖像素块的缓冲区地址
            int groupId =  row - cur_position->x_position; //  算出来在第几个char中
            int memberNum = col - cur_position->y_position; // 算出来在该char中是第几位
            char isFont = char_format[groupId] & (1 << (cur_position->y_char_size - memberNum)); // 判断该位是否为1
            (*pltAddr) = isFont ? font_color : back_ground_color; 
        }
    }
}