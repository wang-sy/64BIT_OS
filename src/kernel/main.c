#include "position.h"
#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"

#define COLOR_OUTPUT_ADDR (int *)0xffff800000a00000
#define SCREEN_ROW_LEN 900 // 一共有多少行
#define SCREEN_COL_LEN 1440 // 一共有多少列
#define CHAR_ROW_LEN 16 // 一个字符一共有多少行
#define CHAR_COL_LEN 8 // 一个字符一共有多少列

extern unsigned char font_ascii[256][16];
// 画点的方法，给出坐标与颜色，将其覆盖
void plot_color_point(int x, int y, char r, char g, char b);

// 内核主程序，执行完内核执行头程序后会跳转到Start_Kernel函数中
void Start_Kernel() {
    int row, col;
    // 根据行列进行绘图，先画20行红色
    for(row = 0; row < 20; row++){
        for(col = 0; col < SCREEN_COL_LEN; col++){
            plot_color_point(row, col, 0xff, 0x00, 0x00);
        }
    }

    for(row = 20; row < 60; row++){
        for(col = 0; col < SCREEN_COL_LEN; col++){
            plot_color_point(row, col, 0x00, 0xff, 0x00);
        }
    }

    for(row = 60; row < 140; row++){
        for(col = 0; col < SCREEN_COL_LEN; col++){
            plot_color_point(row, col, 0x00, 0x00, 0xff);
        }
    }

    for(row = 140; row < 300; row++){
        for(col = 0; col < SCREEN_COL_LEN; col++){
            plot_color_point(row, col, 0xff, 0xff, 0xff);
        }
    }


    // 初始化与屏幕有关的全局变量
    struct position* myPos = &globalPosition;
    (*myPos) = (struct position){
        SCREEN_ROW_LEN, SCREEN_COL_LEN,  // 屏幕行列
        0, 0, // 当前光标位置
        CHAR_ROW_LEN, CHAR_COL_LEN, // 字符行列
        COLOR_OUTPUT_ADDR, sizeof(int) * SCREEN_COL_LEN * SCREEN_ROW_LEN
    };
    
    doClear(myPos);

    int curChar; 
    for(curChar = 40; curChar < 130; curChar ++){ // 输出给出的表格中的每一个字符
        doPrint(myPos, 0xffffff00, 0x00000000, font_ascii[curChar]);
        doNext(myPos);
        if(curChar % 10 == 0) {
            if(curChar % 20 == 0) doBackspace(myPos);
            doEnter(myPos);
            if(curChar % 30 == 0) doTab(myPos);
        }
    }
    doEnter(&globalPosition);
    color_printk(YELLOW,BLACK,"Hello World!");
    doEnter(&globalPosition);

    // TSS段描述符的段选择子加载到TR寄存器
    load_TR(8);

    // 初始化
	set_tss64(
        0xffff800000007c00, 0xffff800000007c00, 
        0xffff800000007c00, 0xffff800000007c00, 
        0xffff800000007c00, 0xffff800000007c00,
        0xffff800000007c00, 0xffff800000007c00, 
        0xffff800000007c00, 0xffff800000007c00
    );

	sys_vector_init(); // 初始化IDT表，确定各种异常的处理函数

    init_memory(); // 输出所有内存信息

	while(1){
        ;
    }
}

/**
 * 在屏幕上画一个点
 * @author wangsaiyu@cqu.edu.cn
 * @param x 在第x行上面画点
 * @param y 在第y列上面画点
 * @param r 点的红色色彩值
 * @param g 点的绿色色彩值
 * @param b 点的蓝色色彩值
 */
void plot_color_point(int x,int y, char r, char g, char b){

    int* addr = COLOR_OUTPUT_ADDR + x * SCREEN_COL_LEN +y;

    *((char *)addr+0)=r;
    *((char *)addr+1)=g;
    *((char *)addr+2)=b;
    *((char *)addr+3)=(char)0x00;
}