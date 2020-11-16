#include "position.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"
#include "interrupt.h"
#include "task.h"

#define COLOR_OUTPUT_ADDR (int *)0xffff800000a00000
#define SCREEN_ROW_LEN 768 // 一共有多少行
#define SCREEN_COL_LEN 1024 // 一共有多少列
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
    struct Position* myPos = &global_position;
    (*myPos) = (struct Position){
        SCREEN_ROW_LEN, SCREEN_COL_LEN,  // 屏幕行列
        0, 0, // 当前光标位置
        CHAR_ROW_LEN, CHAR_COL_LEN, // 字符行列
        COLOR_OUTPUT_ADDR, (SCREEN_ROW_LEN * SCREEN_COL_LEN * 4 + PAGE_4K_SIZE - 1) & PAGE_4K_MASK
    };
    
    /*
    DoClear(myPos);
    int curChar; 
    for(curChar = 40; curChar < 130; curChar ++){ // 输出给出的表格中的每一个字符
        DoPrint(myPos, 0xffffff00, 0x00000000, font_ascii[curChar]);
        DoNext(myPos);
        if (curChar % 10 == 0) {
            if (curChar % 20 == 0) DoBackspace(myPos);
            DoEnter(myPos);
            if (curChar % 30 == 0) DoTab(myPos);
        }
    }
    DoEnter(&global_position);
    color_printk(YELLOW,BLACK,"Hello World!");
    DoEnter(&global_position);
    */
    // TSS段描述符的段选择子加载到TR寄存器
    /*LOAD_TR(10);

    // 初始化
	SetTss64(
        0xffff800000007c00, 0xffff800000007c00, 
        0xffff800000007c00, 0xffff800000007c00, 
        0xffff800000007c00, 0xffff800000007c00,
        0xffff800000007c00, 0xffff800000007c00, 
        0xffff800000007c00, 0xffff800000007c00
    );

	SystemInterruptVectorInit(); // 初始化IDT表，确定各种异常的处理函数

    // 该变量来源于memory.c， 对其中的关键地址信息进行填充
    memory_management_struct.start_code = (unsigned long)& _text;
	memory_management_struct.end_code   = (unsigned long)& _etext;
	memory_management_struct.end_data   = (unsigned long)& _edata;
	memory_management_struct.end_brk    = (unsigned long)& _end;

    InitMemory(); // 输出所有内存信息

    InitInterrupt();

    TaskInit();*/

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