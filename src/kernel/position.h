
#ifndef __POSITION_H__
#define __POSITION_H__


struct position
{
    int XResolution; // 屏幕行数
    int YResolution; // 屏幕列数

    int XPosition; // 当前光标左上角所在位置
    int YPosition; // 当前光标右上角所在位置

    int XCharSize; // 每个字符所占位置的行数
    int YCharSize; // 每个字符所占位置的列数

    unsigned int * FB_addr; // 显示缓冲区的首地址
    unsigned long FB_length; // 显示缓冲区的长度
};

/*引入本头文件时自动引入本变量*/
extern struct position globalPosition;


// 函数，给出一个描述屏幕的结构体，将光标后移一位
void doNext(struct position * curPos);

// 函数，给出一个描述屏幕的结构体，在当前位置模拟换行操作
void doEnter(struct position * curPos);

// 函数，给出一个描述屏幕的结构体，在当前位置模拟Backspace退格操作
void doBackspace(struct position * curPos);

// 函数，给出一个描述屏幕的结构体，在当前位置模拟Tab制表符操作（制表符大小为8个空格）
void doTab(struct position * curPos);

// 函数，给出一个描述屏幕的结构体，清空屏幕上所有的东西，并且将光标置为(0, 0)
void doClear(struct position * curPos);

// 函数，给出一个描述屏幕的结构体，以及想要在当前位置输出的文字的背景颜色、文字颜色、文字格式在当前位置进行输出
void doPrint(struct position * curPos,const int backColor, const int fontColor, const char* charFormat);


#endif