#include <stdarg.h>
#include "printk.h"
#include "lib.h"
#include "linkage.h"


/**
 * 输出一个带有颜色与背景颜色的字符串
 * @param FRcolor 字的颜色
 * @param BKcolor 背景的颜色
 * @param fmt 字的格式
 * @param ...
 * @return 返回
 */
int colorPrintk(unsigned int FRcolor, unsigned int BKcolor, const char * fmt, ...){
    int i = 0;
    int count = 0;
    int line = 0;
    va_list args;
    va_start(args, fmt);
    i = vsprintf(buf,fmt, args);
    va_end(args);

}