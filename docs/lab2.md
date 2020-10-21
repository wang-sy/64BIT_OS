# 64位操作系统——（二）kernel



---

作者：王赛宇

参考列表：

- 主要参考：《一个六十四位操作系统的设计与实现》——田雨
- 《Using as》 ——Dean Elsner &  Jay Fenlason  & friends
- nasm 用戶手册
- [处理器startup.s 常见汇编指令...](https://blog.csdn.net/uunubt/article/details/84838260)

---



## 前情提要



在第一章节中，我们学习、研读了`bootloader`的代码，`bootloader`可以被分为两个过程：

- `boot`：计算机上电，自检完成后自动执行`0x7c00`处的`boot`程序，该程序被限制了大小`512KB`，所以它仅用于进行计算机磁盘设定以及加载、跳转到`loader`
- `loader`：从`boot`处跳转而来，进入`loader`时计算机仍处于`实模式`，尽可操作`1MB`内存，但我们进入了`Big Real Mode`让计算机能够操作`4GB`的内存，并通过这种方法将检索到的`kernel`程序放置在了内存的`1MB`处，并且通过两步(`实模式` -- `保护模式` -- `长模式`)的方法进入了长模式，在长模式下cpu可以操作非常大的内存(`16T`)，每一次切换模式，我们都使用一条`长跳转语句`来切换当前代码的运行状态，在跳转到长模式时，使用长跳转跳转到了`内核程序`所在的地址，到此为止我们完成了从`bootloader`模块到操作系统内核的切换，电脑的控制权交到了`操作系统内核`的手中。

在这一章中，我们将继续完成64位操作系统，值得一提的是，在上一章中，由于主要语言是汇编，我们（我太菜了dbq）只能以解读、copy为主，但是在这一章中，主要语言从汇编变成了C语言，我希望我们的主要任务从解读变为`理解 --> 复现`；下面，我们开始这一章的学习。



## 内核执行头程序`head.S`

我们还是需要写一小会汇编。

### 什么是内核执行头程序

我们知道，在`loader`的最后进行了一次长跳转，跳转到了`kernel.bin`的起始地址，这个起始地址实际上就是物理地址`0x100000`，内核头程序就是被装载到`0x100000`上的程序，它是内核程序的一部分，但内核执行头程序是一个汇编程序，它主要负责为操作系统创建段结构与页结构，设置某些结构的默认处理函数以及配置关键寄存器等工作。

接下来，还有一个非常重要的问题：**如何将内核执行头程序装载到我们想要的地址？**

这里我们需要自己去写一个链接脚本，在连接脚本中，会对不同程序的空间进行布局。这个链接脚本不是我们当前阶段需要关心的内容，我们直接使用作者提供的即可。但我们需要知道：

> 内核层的起始线性地址`0xffff800000000000`对应着物理地址`0`处，内核程序的起始线性地址位于`0xffff800000000000 + 0x100000`。

<img src="http://www.ituring.com.cn/figures/2019/OperatingSystemx64/05.d04z.001.png" alt="img" style="zoom:33%;" />



### 定义各种表以及段结构（数据段）

这里我们还是以解读为主了：

```gas
#//=======    GDT_Table

.section .data // 数据段

.globl GDT_Table // 声明全局符号，可以被其他程序调用

GDT_Table:
    // 一次写8个Byte
    .quad    0x0000000000000000    /*0 NULL descriptor 00*/
    .quad    0x0020980000000000    /*1 KERNEL Code 64-bit Segment 08*/
    .quad    0x0000920000000000    /*2 KERNEL Data 64-bit Segment 10*/
    .quad    0x0020f80000000000    /*3 USER    Code 64-bit Segment 18*/
    .quad    0x0000f20000000000    /*4 USER    Data 64-bit Segment 20*/
    .quad    0x00cf9a000000ffff    /*5 KERNEL Code 32-bit Segment 28*/
    .quad    0x00cf92000000ffff    /*6 KERNEL Data 32-bit Segment 30*/
    .fill    10,8,0                    /*8 ~ 9    TSS (jmp one segment <7>) in long-mode 128-bit 40*/
    // 重复十次，每次覆盖八个字节，每个位置都填充为0
GDT_END:

GDT_POINTER:
GDT_LIMIT:    .word    GDT_END - GDT_Table - 1
GDT_BASE:     .quad    GDT_Table

//=======     IDT_Table

.globl IDT_Table

IDT_Table:
    .fill 512,8,0
IDT_END:

IDT_POINTER:
IDT_LIMIT:    .word    IDT_END - IDT_Table - 1
IDT_BASE:     .quad    IDT_Table

//=======     TSS64_Table

.globl        TSS64_Table

TSS64_Table:
    .fill 13,8,0
TSS64_END:

TSS64_POINTER:
TSS64_LIMIT:    .word    TSS64_END - TSS64_Table - 1
TSS64_BASE:     .quad    TSS64_Table

```

在解读之前给大家推荐一个网站，大家可以使用这个网站来搜索相应的汇编关键词：https://sourceware.org/binutils/docs/as/，本文档中的汇编知识全部来自于这个网站（需要注意的是，我们这里不再使用上一章节的nasm了，使用的是GNU提供的汇编器`GAS`）

这里在做的事情还是和上一章最后在做的事情一样：定义了`GDT表`、`IDT表`、`TSS64`，这里的`TSS64`是任务状态段。他们都会被存储在内核程序的数据段中。



我们来熟悉一下里面的一些语句：

- `.section .data`：这个语句的前半部分`.section`表示把代码分成若干段，程序被加载时会加载到不同的地址，`.data`代表这部分代码书写的是程序的数据段，数据段是可读可写的。
- `.globl GDT_Table`：首先这个语句的`.globl`是一个声明，这个声明是告诉汇编器，后面的符号需要被连接器用到，所以要在目标文件的符号表中标记它是一个全局符号，这里是告诉连接器GDT_Table是一个全局符号，GDT_Table是一个标签，他的内容在下面进行了定义
- `.quad    0x0000000000000000`：`.quad`指令类似于上一章中的`db`指令，都是用于填充二进制的，但是`.quad`指令是用于填充一个`8 Byte`的二进制数字的，如果你仔细一数，就发现上面的十六进制数字由16个0构成，占空间`8 Byte`。
- `.fill   10,8,0`：这个`.fill`的标准用法如下`fill repeat, size, value`，这里就表示重复十次，每次写8个字节，填充的内容是0。
- `GDT_LIMIT:    .word    GDT_END - GDT_Table - 1`：这里的`.word`实际上和`.quad `是相似的，只不过它是存储了`4Byte`的数据。

这里出现的所有汇编语句我们都解释一遍了，大体上来说这里是一个数据段，定义了两个表个一个段，同时使用`.globl`使定义的数据段成为全局符号，可以被其他程序调用。





### 创建并初始化页表及页表项



```gas
//=======    init page
.align 8

.org    0x1000

__PML4E:
    .quad    0x102007
    .fill    255,8,0
    .quad    0x102007
    .fill    255,8,0

.org    0x2000

__PDPTE:
    .quad    0x103003
    .fill    511,8,0

.org    0x3000

__PDE:
    .quad    0x000083
    .quad    0x200083
    .quad    0x400083
    .quad    0x600083
    .quad    0x800083
    .quad    0xe0000083        /*0x a00000*/
    .quad    0xe0200083
    .quad    0xe0400083
    .quad    0xe0600083       /*0x1000000*/
    .quad    0xe0800083 
    .quad    0xe0a00083
    .quad    0xe0c00083
    .quad    0xe0e00083
    .fill    499,8,0


```



这段代码中有两个我们不大熟悉的地方：

- `.align 8`：这个地方代表我们将下一条语句进行对齐，对齐的单位是8，比如说这下语句本来会被放在结尾是5的内存中，那么加入这条语句后，就会将下条语句的地址进行对齐，对齐的方法是后移。官方给出的描述是：`'.align 8'将位置计数器递增到它的8的倍数。如果位置计数器已经是8的倍数，则无需更改。`

- `.org    0x3000`：`.org`表示当前节的位置计数器前进到给定的位置，如果这个位置有错，那么就忽略这条语句。官方的指南中有这样的描述：`.org`只能增加或不改变位置计数器；您不能使用`.org`将位置计数器向后移动。

  以本程序为例，这个`.org 0x3000`会试图将当前的代码放置在程序头部偏移`0x3000`的位置，那么程序头部的位置我们已经在最开始的时候说过了，就是：`0xffff800000000000 + 0x100000`，那么`__PDE`会被存放在线性地址为`0xffff800000000000 + 0x100000 + 0x3000`的位置，其他的也是同理。



这里定义的是页表相关的数据。此页表将线性地址0和`0xffff800000000000`映射为同一物理页以方便页表切换，即程序在配置页表前运行于线性地址`0x100000`附近，经过跳转后运行于线性地址`0xffff800000000000`附近。这里将前10 MB物理内存分别映射到线性地址`0`处和`0xffff800000000000`处，接着把物理地址`0xe0000000`开始的16 MB内存映射到线性地址`0xa00000`处和`0xffff800000a00000`处，最后使用伪指令`.fill`将数值0填充到页表的剩余499个页表项里。



### 再次进行IA-32e模式初始化

```gas
.section .text

.globl _start

_start:

    mov    $0x10,    %ax
    mov    %ax,      %ds
    mov    %ax,      %es
    mov    %ax,      %fs
    mov    %ax,      %ss
    mov    $0x7E00,  %esp

//=======    load GDTR

    lgdt    GDT_POINTER(%rip)

//=======  load      IDTR

    lidt   IDT_POINTER(%rip)

    mov    $0x10,    %ax
    mov    %ax,      %ds
    mov    %ax,      %es
    mov    %ax,      %fs
    mov    %ax,      %gs
    mov    %ax,      %ss

    movq   $0x7E00,    %rsp

//=======  load      cr3

    movq   $0x101000,   %rax
    movq   %rax,        %cr3
    movq   switch_seg(%rip),    %rax
    pushq  $0x08
    pushq  %rax
    lretq

//=======  64-bit mode code

switch_seg:
    .quad  entry64

entry64:
    movq   $0x10,    %rax
    movq   %rax,     %ds
    movq   %rax,     %es
    movq   %rax,     %gs
    movq   %rax,     %ss
    movq   $0xffff800000007E00,    %rsp        /* rsp address */

    movq   go_to_kernel(%rip),     %rax        /* movq address */
    pushq  $0x08
    pushq  %rax
    lretq

go_to_kernel:
    .quad    Start_Kernel
```



这里的流程和前面完全一致，我们主要来解读汇编代码：



- `.section .text`表示一个新的段，这个`.text`表示这里是代码段

- `_start`相当于c语言的`main`函数，他必须被声明为`globl`，程序从这里开始执行

- `mov    $0x10,    %ax`将立即数`0x10`赋值给寄存器`%ax`

- `mov    %ax,      %ds`将`%ax`寄存器中的值赋值给`%ds`

- `lgdt GDT_POINTER(%rip)`这里仍然是在装载GDT表，但是寻址方式有所变化：

  |                  | Intel汇编语言格式      | AT&T汇编语言格式     |
  | :--------------- | :--------------------- | :------------------- |
  | RIP-Relative寻址 | `[rip + displacement]` | `displacement(%rip)` |

- 函数跳转：这里作者用了一个非常有意思的方法，总体来说：作者伪造了函数调用的现场，假装自己被调用了，然后再假装自己调用结束，返回了。这样cpu就会进入我们刚才push到栈中的地址，或者说是“返回到”刚才push到栈中的地址。



做了这些讲解之后，这段代码就非常好懂了：

- 首先：初始化了一系列寄存器的值
- 第二步：读取了GDT、IDT表，更改了cr3
- 第三步：使用特殊的跳转方法跳转到了`entry 64`函数
- 第四步：在`entry 64`函数中，完成进入64位操作系统的操作后，跳转到`Start_Kernel`函数

由于这里没有定义`Start_Kernel`函数，所以是无法完成跳转的



### makefile

kernel的makefile，这个非常简单，我们就不多讲解了。

```gas
all: head.o

head.o: head.S
	gcc -E head.S > ./build/head.s
	as --64 -o ./build/head.o ./build/head.s

clean: 
	rm -rf ./build/*
```



对于主文件夹下的makefile这里我们用到了makefile中的for循环：

```makefile
BOOT_SRC_DIR=./src/boot
KERNEL_SRC_DIR=./src/kernel
SUBDIRS=$(BOOT_SRC_DIR) $(KERNEL_SRC_DIR)

define \n # 定义换行符，下面要用


endef
BOOT_BUILD_DIR=$(BOOT_SRC_DIR)/build
KERNEL_BUILD_DIR=$(KERNEL_SRC_DIR)/build
PROJECT_BUILD_DIR=./build

all: 
# 遍历每一个文件夹，进行编译
	$(foreach dir, $(SUBDIRS), cd $(dir) && $(MAKE) ${\n})


install: 
# 将boot的bin写入到引导扇区内 

	echo "特别声明：不要删除boot.img，如果删除了， 请到64位操作系统书中36页寻找复原方法"
	dd if=$(BOOT_BUILD_DIR)/boot.bin of=$(PROJECT_BUILD_DIR)/boot.img bs=512 count=1 conv=notrunc 
	sudo mount $(PROJECT_BUILD_DIR)/boot.img /media/ -t vfat -o loop
	sudo cp $(BOOT_BUILD_DIR)/loader.bin /media
	sudo cp $(BOOT_BUILD_DIR)/kernel.bin /media
	sync
	sudo umount /media/
	echo 挂载完成，请进入build文件夹后输入"bochs"以启动虚拟机

clean:
# 遍历每一个文件夹，进行删除
	$(foreach dir, $(SUBDIRS), cd $(dir) && $(MAKE) clean ${\n})

```

这里方法也很简单，就是遍历每一个`SUBDIRS`中的文件夹，进行make操作，操作完再回来，再进入下一个文件夹。

这里使用了`define `定义了换行符，然后使用`foreach`方法，对于每一个`$(SUBDIRS)`中的文件夹`dir`，我们输出的内容是：

```shell
cd $(dir) && $(MAKE) ${\n}
```

意思就是进入文件夹、执行make、输出换行，之所以输出换行，是因为makefile中的cd仅在当前行有效，也就是说，我们换行就相当于回到主文件夹下重新执行了，这样就可以完成循环进入所有子文件夹，执行make的操作。





## 内核主程序

简单回顾一下，我们在上个小节中完成了三个表的声明与数据填充，并且重新进行了`IA-32e`模式的初始化，在内核执行头程序的最后，跳转到了`Start_Kernel`函数，这个`Start_Kernel`函数就是这一节我们想要介绍的内核主程序的主体。一般情况下，它将负责调各个系统模块的初始化函数，在初始化结束后，他会创建出系统的第一个进程`init`，并且将控制权交给`init`进程。



当然了，在这一个小节中，我们只是写一个“假的”内核主程序，在之后的其他小节中，我们会继续完善它。这一节主要讲解怎么去编译、链接它。



### 写一个“假的”内核主程序



这个就很脑瘫了，我们只要写一个含有`Start_Kernel`函数的程序即可，这里是`main.c`：(终于见到亲切的高级语言了)

```C
// 内核主程序，执行完内核执行头程序后会跳转到Start_Kernel函数中
void Start_Kernel() {
	while(1){
        ;
    }
}
```

这里我们就让他原地空转（死循环），接下来我们讨论如何编译它的问题。



### 编译内核主程序

这里我们继续完善我们的`makefile`文件，在此之前我们来完整的看一下当前的文件夹结构（之前从来没介绍过，大家可能看我的makefile的时候会很懵逼）：

```
64BIT_OS 
├─ README.md : 项目描述文件
├─ build ：整个项目生成的可执行空间， 在这个文件夹下执行bochs就可以直接打开生成好的虚拟机
│  ├─ .bochsrc ：虚拟机的描述文件
│  └─ boot.img ：虚拟机的镜像文件
├─ docs ：这个文档所在的文件夹，用于存储文档以及文档中用到的图片
├─ makefile ： 整个项目的make脚本
└─ src ：源码目录
   ├─ boot：bootloader模块
   │  ├─ boot.asm ：boot
   │  ├─ build ：bootloader模块编译生成的文件都存储在这里
   │  ├─ fat12.inc ：被%include的东西
   │  ├─ loader.asm ：loader
   │  └─ makefile ：bootloader模块的makefile文件，该脚本生成的文件都放在boot\biuild文件夹下
   └─ kernel：内核模块
      ├─ build ：kernel模块编译生成的文件都存储在这里
      ├─ head.S
      ├─ main.c
      └─ makefile ： 内核的make脚本
```



这里我们继续完善的是`kernel/makefile`。在自己写之前，我们先来了解一下一个编译器在编译一个文件的时候执行的操作，为了完整的演示整个过程，我们再写一套专门用来演示的程序：（当然，你可以不看着一部分，这一部分只是为了让你深刻理解gcc编译器在编译一段程序的完整过程中会干些什么）

#### 演示程序

`temp.h`

```c
int a = 1;
int b = 2;
int c = 3;

```

`temp.c`

```C





#include "temp.h"


int main() {
        a = b + 1;
}
```



这里我们使用`gcc temp.c`就会生成一个可执行文件`a.out`，执行`a.out`时什么都不会输出，因为我们没有让这个程序输出。



#### 编译过程

- 预编译：我们在控制台执行指令`gcc -E temp.c -o temp.i`表示执行预编译，并且指定输出文件为`temp.i`，完成后打开`temp.i`查看：

  ```C
  # 1 "temp.c"
  # 1 "<built-in>"
  # 1 "<command-line>"
  # 1 "/usr/include/stdc-predef.h" 1 3 4
  # 1 "<command-line>" 2
  # 1 "temp.c"
  
  
  
  
  
  # 1 "temp.h" 1
  int a = 1;
  int b = 2;
  int c = 3;
  # 7 "temp.c" 2
  
  
  int main() {
   a = b + 1;
  }
  
  ```

  可以看到，这里生成了一个文件，这个文件的实质就是把`temp.h`中的内容复制了进来。

- 汇编：继续执行`gcc -S temp.i -o temp.s`：

  ```gas
          .file   "temp.c"
          .text
          .globl  a
          .data
          .align 4
          .type   a, @object
          .size   a, 4
  a:
          .long   1
          .globl  b
          .align 4
          .type   b, @object
          .size   b, 4
  b:
          .long   2
          .globl  c
          .align 4
          .type   c, @object
          .size   c, 4
  c:
          .long   3
          .text
          .globl  main
                            
  main:
  .LFB0:
          .cfi_startproc
          endbr64
          pushq   %rbp
          .cfi_def_cfa_offset 16
          .cfi_offset 6, -16
          movq    %rsp, %rbp
          .cfi_def_cfa_register 6
          movl    b(%rip), %eax
          addl    $1, %eax
          movl    %eax, a(%rip)
          movl    $0, %eax
          popq    %rbp
          .cfi_def_cfa 7, 8
          ret
          .cfi_endproc
  .LFE0:
          .size   main, .-main
          .ident  "GCC: (Ubuntu 9.3.0-17ubuntu1~20.04) 9.3.0"
          .section        .note.GNU-stack,"",@progbits
          .section        .note.gnu.property,"a"
          .align 8
          .long    1f - 0f
          .long    4f - 1f
          .long    5
  0:
          .string  "GNU"
  1:
          .align 8
          .long    0xc0000002
          .long    3f - 2f
  2:
          .long    0x3
  3:
          .align 8
  4:
  
  ```

  这里生成了我们非常眼熟的`gas`汇编语言。

- 生成编译好但没有连接的文件：`gcc - c temp.s -o temp.o`

  这里生成的是一个二进制文件，打开它来看没啥意义

- 链接：`gcc temp.o -o temp`：

  ```shell
  $ ./temp
  
  ```

  当然，执行他还是啥都不会发生，但是能够执行

到此为止，我们就把整个流程走了一遍了，相信大家都大致理解了编译的过程。



#### 应用到项目中

回想一下我们要做的事情：先编译`main.c`，然后再让他按照我们的想法链接，于是我们就要先编译一下`main.c`，即在`makefile`中添加：

```makefile
main.o: main.c
# 编译main.c内核主程序
	gcc -mcmodel=large -fno-builtin -m64 -c main.c -o ./build/main.o
```

接下来我们指定链接脚本，并且进行链接：

```makefile
system: head.o main.o
	ld -b elf64-x86-64 -o ./build/system ./build/head.o ./build/main.o -T Kernel.lds
```

这句的意思就是链接`head.o\main.o`两个文件，指定输入的文件类型是`efl64-x86-64`，并且指定了链接脚本是`Kernel.lds`。

我们这里也新建一个链接脚本：

```shell
OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS
{

        . = 0xffff800000000000 + 0x100000;
        .text :
        {
                _text = .;
                *(.text)

                _etext = .;
        }
        . = ALIGN(8);
        .data :
        {
                _data = .;
                *(.data)

                _edata = .;
        }
        .bss :
        {
                _bss = .;
                *(.bss)
                _ebss = .;
        }

        _end = .;
}
```

这段脚本非常简单：

- `OUTPUT_FORMAT`：指定输出的文件类型（和输入一样）
- `OUTPUT_ARCH`：指定输出的体系结构
- `ENTRY`：指定程序的入口（开始执行的地方）
- `SECTIONS`：指定了程序被加载到哪里，每个段的相对关系以及对齐位置



相应的，`all`也不再依赖`head.o/main.o`了，而是转而依赖`system`即可，之后还需要添加：

```makefile
all: system
# 生成kernel.bin
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary ./build/system ./build/kernel.bin
```

这里使用了`objcopy`生成了`kernel.bin`，这里我们就生成了`真·内核`，所以我们就把上一章中的假内核删掉就可以了。我们要做出于以下改变：

- 将boot文件夹下的makefile中生成kernel.bin的语句和依赖删除（自己删，或者看我的源码，这里就不啰嗦了，非常简单）

- 更改主文件夹下拷贝`kernel.bin`的语句（说白了就是把copy的源改一下就可以了）：

  ```makefile
  sudo cp $(KERNEL_BUILD_DIR)/kernel.bin /media
  ```

  

#### 简单验证

我们要验证一下到此为止是否正确执行，这里我们使用反汇编指令：

```shell
objdump -D system
```

找到`Start Kernel`部分

```gas
ffff80000010400c:       48 8d 05 f5 ff ff ff    lea    -0xb(%rip),%rax        # ffff800000104008 <Start_Kernel+0x8>
ffff800000104013:       49 bb 68 11 00 00 00    movabs $0x1168,%r11
ffff80000010401a:       00 00 00 
ffff80000010401d:       4c 01 d8                add    %r11,%rax
ffff800000104020:       eb fe                   jmp    ffff800000104020 <Start_Kernel+0x20>
```

这里就标记出了while语句的线性地址`ffff800000104020`

```shell
^C00156063365i[      ] Ctrl-C detected in signal handler.
Next at t=156063366
(0) [0x000000104020] 0008:ffff800000104020 (unk. ctxt): jmp .-2 (0xffff800000104020) ; ebfe
<bochs:2> r
CPU0:
rax: ffff8000_00105170 rcx: 00000000_c0000080
rdx: 00000000_00000000 rbx: 00000000_00000000
rsp: ffff8000_00007df8 rbp: ffff8000_00007df8
rsi: 00000000_00008098 rdi: 00000000_0000bd00
r8 : 00000000_00000000 r9 : 00000000_00000000
r10: 00000000_00000000 r11: 00000000_00001168
r12: 00000000_00000000 r13: 00000000_00000000
r14: 00000000_00000000 r15: 00000000_00000000
rip: ffff8000_00104020
eflags 0x00000092: id vip vif ac vm rf nt IOPL=0 of df if tf SF zf AF pf cf
```

RIP寄存器存放着当前指令的地址，这里指示的正是当前的跳转指令，也就是说程序已经进入了死循环，验证结果为正确。

### clion

接下来我们的项目会用clion来接管，主要原因是：好看，提示比较全，当然，由于我的git配置的原因（我故意的），大家克隆下来的项目不会有任何迹象，这里我就简单介绍一下主要的方法，这里我是用的脚本来做的一键执行，因为我们的代码中需要赋权，赋权需要密码，所以我就没把握的脚本放到github上面，这里我就把我的脚本分享一下：

```shell
#!/usr/bin/env zsh
make clean
make
echo [passowrd] | sudo -S make install
make clean

cd build
bochs
```

在clion配置中，配置`run.sh`作为项目运行的脚本即可。上面的password直接填写自己的密码即可。





## 屏幕显示

之前在`loader`中，我们已经设置了显示模式：`模式号：0x180、分辨率：1440×900、颜色深度：32 bit`。我们在屏幕上显示东西的原理就是：将现实的画面存储到内存中的`帧缓存`区域中，`帧缓存`中的每一个存储单元对应着屏幕上的一个像素点。帧缓存的起始地址是：`0xe0000000`，我们之前在定义页表的过程中，已经进行了两组映射：

- `0xe0000000` 到 `0xffff800000a00000`
- `0xe0000000` 到 `0xa00000`



### 在屏幕上显示色彩



#### 屏幕布局（画布）

首先，我们需要了解屏幕的布局：

<img src="http://www.ituring.com.cn/figures/2019/OperatingSystemx64/05.d04z.002.png" alt="img" style="zoom:33%;" />

我们的坐标系是从左上角为原点的。



#### 单点像素构成

我们刚才说了，这个用的是`32bit`的颜色，这个描述方式只有前`24`位是有效位，他们分别表示`rgb`的值（其实就是标准的rgb描述）。

```C
#define COLOR_OUTPUT_ADDR (int *)0xffff800000a00000
#define LINE_SIZE 1440


// 画点的方法，给出坐标与颜色，将其覆盖
void plot_color_point(int x, int y, char r, char g, char b);

// 内核主程序，执行完内核执行头程序后会跳转到Start_Kernel函数中
void Start_Kernel() {
    int *addr = (int *)0xffff800000a00000;
    int row, col;
    // 根据行列进行绘图，先画20行红色
    for(row = 0; row < 20; row++){
        for(col = 0; col < LINE_SIZE; col++){
            plot_color_point(row, col, 0xff, 0x00, 0x00);
        }
    }

    for(row = 20; row < 60; row++){
        for(col = 0; col < LINE_SIZE; col++){
            plot_color_point(row, col, 0x00, 0xff, 0x00);
        }
    }

    for(row = 60; row < 140; row++){
        for(col = 0; col < LINE_SIZE; col++){
            plot_color_point(row, col, 0x00, 0x00, 0xff);
        }
    }

    for(row = 140; row < 300; row++){
        for(col = 0; col < LINE_SIZE; col++){
            plot_color_point(row, col, 0xff, 0xff, 0xff);
        }
    }

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

    int* addr = COLOR_OUTPUT_ADDR + x * LINE_SIZE +y;

    *((char *)addr+0)=r;
    *((char *)addr+1)=g;
    *((char *)addr+2)=b;
    *((char *)addr+3)=(char)0x00;
}
```



我们将在屏幕上画一个点的方法进行封装，我们只需要给出需要绘画的点的横纵坐标以及rgb值即可进行绘图。绘图的方法也非常简单且暴力，就是直接覆盖对应的内存区域。在主程序中，我们多次调用这个方法进行绘图，绘出了四个不同颜色的矩形，效果如下：

<img src="/home/wangsy/Code/64BIT_OS/docs/pics/lab2/image-20201020163850427.png" alt="image-20201020163850427" style="zoom: 50%;" />



## 显示字符串

在这一小节中，我们来实现向屏幕输出单个字符以及如何向屏幕输出一个字符串。我们的目标是实现一个`printk`函数，能够向屏幕输出一个字符串。



### 一个字符如何组成

我们都知道，电脑上看到的画面是由一个一个像素点组成的，同理，我们看到的字符也是由很多的像素点组成的，在我们的系统中，一个字符占`16行8列`的空间，如下：

<img src="http://www.ituring.com.cn/figures/2019/OperatingSystemx64/05.d04z.004.png" alt="img" style="zoom:33%;" />

我们使用一个数组来记录每个字符的方块的内容：

```C
unsigned char font_ascii[256][16]=
{
    ……
    /*    0040    */
    {0x02,0x04,0x08,0x08,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x08,0x08,0x04,0x02, 0x00}, // '('
    {0x80,0x40,0x20,0x20,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x20,0x20,0x40,0x80, 0x00}, // ')'
    {0x00,0x00,0x00,0x00,0x00,0x10,0x92,0x54,0x38,0x54,0x92,0x10,0x00,0x00,0x00, 0x00}, // '*'
    {0x00,0x00,0x00,0x00,0x00,0x10,0x10,0x10,0xfe,0x10,0x10,0x10,0x00,0x00,0x00, 0x00}, // '+'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x08,0x08, 0x10}, // ','
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe,0x00,0x00,0x00,0x00,0x00,0x00, 0x00}, // '-'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00, 0x00}, // '.'
    {0x02,0x02,0x04,0x04,0x08,0x08,0x08,0x10,0x10,0x20,0x20,0x40,0x40,0x40,0x80, 0x80}, // '/'
    {0x00,0x18,0x24,0x24,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x24,0x24,0x18,0x00, 0x00}, //48    '0'
    {0x00,0x08,0x18,0x28,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x3e,0x00, 0x00}, // '1'
    ……
};
```

这里是这个数组的局部，这是一个二维数组，他的size是`[256][16]`，`font_ascii[3]`就代表`ASCII`码为3的字符的方块内容，其中一个数字代表一列，该数字的每一位代表一个像素是否进行填充。



### 如何打印一行文字

在上一段中我们讲了如何在屏幕指定的区域打印一行文字，在知道这样的信息后，我们这一段来关注如何在屏幕上打印一行文字。



- 首先，我们需要来复现一下使用场景，这里使用我们常用的`printf`函数来做类比：

  ```c
  printf("numa:%d\n", numa);
  ```

  我们来解读一下这个句子，首先，这个句子包括两个大的部分，前面是一个字符串，他表示我们最终需要输出的字符串，在这个字符串中，可能会有一些**占位符**，具体来说，这些占位符可能是基础的：`%d, %s, %c`等等，也有可能是`%06d`这种的，当然，除此之外，这个字符串中可能还有一些较为特殊的字符，比如：`\n, \t, \b`这三个，他们分别表示回车、制表符、回退，这三个都无法直接输出，所以我们需要进行特殊的判断。
  
  综上所述，我们这个输出一行字的函数可以被拆分为两个阶段：
  
  - **预处理**：给出一个格式字符串如`"numa:%d\n"`以及跟随的参数`numa`（这个参数的数量未知），对字符串进行解析，得到一个能够直接进行输出的字符串（相当于直接把后面的参数融合到字符串中）。
  - **输出**：读取预处理完毕的字符串，逐个字符进行输出，输出到屏幕上



#### 总体工作思路

这里我们的解决方案和作者的解决方案有很大的不同，我们采用一种类似于面向对象编程的思想来完成这些操作。我们的整体逻辑可以分为两个大的部分：

- 字符串处理
- 字符串输出

在输出字符串的时候，需要对屏幕进行操作（实际上就是对用于显示的缓冲区进行操作），那么这个时候我们就想到了去建立一个"类"来描述这个过程，我们来看一下这个`cursor类`需要描述哪些东西：

- 屏幕的大小
- 光标的位置
- 每个字符所占的方块的尺寸
- 操作的缓冲区的起始地址以及长度

上面列出来的就是我们的缓冲区需要描述的属性，那么下一个问题，我们的光标在当前阶段需要进行哪些操作：

- 向后移动一位
- 模拟制表符操作
- 模拟回退操作
- 模拟换行操作
- 清空屏幕
- 输出一个字符

这是我所想到的，我们的缓冲区需要进行的操作，经过上面的思考后，我们的"类"就有了一个雏形（当然了，C语言没法面向对象，所以我们只能用变量+函数的形式来爽一下了）：（这里我构建了一个程序`position.h`来描述相关的信息）

```C

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
}Pos;

// 函数，给出一个描述屏幕的结构体，将光标后移一位
void doNext(struct position * curPos);

// 函数，给出一个描述屏幕的结构体，在当前位置模拟换行操作
void doEnter(struct position * curPos);

// 函数，给出一个描述屏幕的结构体，在当前位置模拟Backspace退格操作
void doBackspace(struct position * curPos);

// 函数，给出一个描述屏幕的结构体，在当前位置模拟Tab制表符操作（制表符大小为8个空格）
void doTab(struct position * curPos);

// 函数，给出一个描述屏幕的结构体，以及想要在当前位置输出的文字的背景颜色、文字颜色、文字格式在当前位置进行输出
void doPrint(struct position * curPos,const int backColor, const int fontColor, const char* charFormat);

#endif
```



接下来我们在`position.c`中，实现这些功能即可：





在printk函数中，我们需要实现这一功能

