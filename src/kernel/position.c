#include "position.h"
#include "lib.h"

// 实现position中的一些函数
char* defaultFill = "                                                                ";

/**
 * 给出当前的位置结构体，将光标移动到下一区域
 * @param curPos 一个指针，指向被操作的位置结构体
 */ 
void doNext(struct position* curPos){
    curPos->YPosition = curPos->YPosition + curPos->YCharSize;
    if(curPos->YPosition >= curPos->YResolution) doEnter(curPos); // 试探，如果错误，就直接重置
}

/**
 * 给出当前的位置结构体，模拟回车时的操作
 * @param curPos 一个指针，指向被操作的位置结构体
 */ 
void doEnter(struct position * curPos){
    curPos->YPosition = 0;
    curPos->XPosition = curPos->XPosition + curPos->XCharSize; // 平移
    if(curPos->XPosition >= curPos->XResolution) doClear(curPos); // 试探，如果错误，就直接重置
}

/**
 * 给出当前的位置结构体，将光标置为(0, 0)[暂时不实现清空屏幕的功能]
 * @param curPos 一个指针，指向被操作的位置结构体
 */ 
void doClear(struct position * curPos){
    curPos->XPosition = 0;
    curPos->YPosition = 0;
    memset(curPos->FB_addr, 0xff, curPos->FB_length);
}

/**
 * 给出当前的位置结构体，以及想要在当前位置输出的文字的背景颜色、文字颜色、文字格式在当前位置进行输出
 * @param curPos 一个指针，指向被操作的位置结构体
 * @param backColor 背景颜色
 * @param fontColor 字体颜色
 * @param charFormat  字体样式会根据字体样式进行颜色填充
 */ 
void doPrint(struct position * curPos,const int backColor, const int fontColor, const char* charFormat){
    int row, col;

    // 遍历每一个像素块，进行输出
   for(row = curPos->XPosition; row < curPos->XPosition + curPos->XCharSize; row ++){
        for(col = curPos->YPosition; col < curPos->YPosition + curPos->YCharSize; col ++){
            int* pltAddr = curPos->FB_addr + curPos->YResolution * row + col; // 当前方块需要覆盖像素块的缓冲区地址
            int groupId =  row - curPos->XPosition; //  算出来在第几个char中
            int memberNum = col - curPos->YPosition; // 算出来在该char中是第几位
            char isFont = charFormat[groupId] & (1 << (curPos->YCharSize - memberNum)); // 判断该位是否为1

            (*pltAddr) = isFont ? fontColor : backColor; 
        }
    }
    
    
}

/**
 * 给出当前的位置结构体，在当前位置模拟退格键[退格键不会将回车删除（也就是说，无论怎么退格，Y值都不会变）]
 * @param curPos 一个指针，指向被操作的位置结构体
 */ 
void doBackspace(struct position* curPos){
    // 先在当前位置画一个空格（把之前的字符覆盖掉） 画的时候背景是黑色
    doPrint(curPos, 0x00000000, 0x00000000, defaultFill);

    // 如果不是行的第一个，那么就减一个空位
    curPos->YPosition = (curPos->YPosition - curPos->YCharSize <= 0) ? 0 : 
                        curPos->YPosition - curPos->YCharSize; 
}

/**
 * 给出当前的位置结构体，在当前位置模拟输入一次制表符
 * 这里无论如何都要做一次，然后直到对齐4位为止
 * @param curPos 一个指针，指向被操作的位置结构体
 */ 
void doTab(struct position * curPos){
    do{
        doNext(curPos);
    } while(((curPos->YPosition / curPos->YCharSize) & 4) == 0);
}