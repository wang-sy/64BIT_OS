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

<img src="pics/lab2/05.d04z.001.png" alt="img" style="zoom:33%;" />



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

<img src="pics/lab2/05.d04z.002.png" alt="img" style="zoom:33%;" />

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

<img src="./pics/lab2/image-20201020163850427.png" alt="image-20201020163850427" style="zoom: 50%;" />



## 显示字符串

在这一小节中，我们来实现向屏幕输出单个字符以及如何向屏幕输出一个字符串。我们的目标是实现一个`printk`函数，能够向屏幕输出一个字符串。



### 一个字符如何组成

我们都知道，电脑上看到的画面是由一个一个像素点组成的，同理，我们看到的字符也是由很多的像素点组成的，在我们的系统中，一个字符占`16行8列`的空间，如下：

<img src="pics/lab2/05.d04z.004.png" alt="img" style="zoom:33%;" />

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



### 如何移动光标并且打印文字

在上一段中，我们讲了单个字符是如何构成的，有了这个基础，就可以来学习怎么移动光标，并且打印单个文字了。



#### 使用一个结构体来描述这些功能

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

// 函数，给出一个描述屏幕的结构体，清空屏幕上所有的东西，并且将光标置为(0, 0)
void doClear(struct position * curPos);

// 函数，给出一个描述屏幕的结构体，以及想要在当前位置输出的文字的背景颜色、文字颜色、文字格式在当前位置进行输出
void doPrint(struct position * curPos,const int backColor, const int fontColor, const char* charFormat);


#endif
```

#### 各种方法的实现



接下来我们在`position.c`中，实现这些功能即可：

```C
#include "position.h"

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
```



这里按照惯例，来讲解一下代码，我终于摆脱了copy作者代码的阴影，开始自己创作了，下面我们来简单的讲一下这些函数：

- `doNext`：将位置向后移动一个方格，如果当前位置已经是行末，那么就调用`doEnter()`

- `doEnter`：将位置移动到下一行的行初，如果移动后超出了屏幕，就调用`doClear`

- `doClear`：将位置移动到左上角（暂时没有让他带有清空屏幕的功能）

- `doPrint`：输出一个字符，在执行这个函数的过程中，有几个需要讲解的点：

  - `int* pltAddr = curPos->FB_addr + curPos->YResolution * row + col;`：计算我们要覆盖的屏幕显示缓冲区内存地址。

  - `charFormat[groupId] & (1 << (curPos->YCharSize - memberNum))`：作者给出的字符表是反着的，他实际的作用如下：

    ![](./pics/lab2/figure3.png) 

    实际上就是对比当前位是否为0。

- `doBackspace`将当前的位置用黑色覆盖，然后将位置向前移动一个即可。

- `doTab`：无论何时使用`TAB`的时候，都会向后至少移动一格，基于这个理论，我们使用一个`do-while()`就可以非常容易解决了。后面写的判断，就是一个简易的膜4，对于CPU来说，按位与运算只需要一个时钟周期即可完成，而取余数运算是基于除法运算的，对于64位的cpu而言，最简单的除法操作需要64个时钟周期，就算用华莱士数+both来做优化，也需要让流水线停止等待很久，所以说，对于`2^n`形式的数字进行取余数，可以使用按位与的方式来判断其是否为0，这样可以极大地加快其运算速度。



#### 对实现结果的验证

我们想要输出字符串，就直接连续调用这个函数即可

我们在main函数中，实例化一个结构体，并且调用一些打印的方法来验证一下我们的程序是否正确：

```C
// 初始化屏幕
struct position* myPos = &(struct position){
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
```

每输出10个字符输出一个换行符，每输出20个字符输出一个退格，每输出30个输出一个TAB，执行结果如下，完全无误：

<img src="./pics/lab2/image-20201022001902987.png" alt="image-20201022001902987" style="zoom:50%;" />



### 输出带有格式化信息的字符串



- 首先，我们需要来复现一下使用场景，这里使用我们常用的`printf`函数来做类比：

  ```c
  printf("numa:%d\n", numa);
  ```

  我们来解读一下这个句子，首先，这个句子包括两个大的部分，前面是一个字符串，他表示我们最终需要输出的字符串，在这个字符串中，可能会有一些**占位符**，具体来说，这些占位符可能是基础的：`%d, %s, %c`等等，也有可能是`%06d`这种的，当然，除此之外，这个字符串中可能还有一些较为特殊的字符，比如：`\n, \t, \b`这三个，他们分别表示回车、制表符、回退，这三个都无法直接输出，所以我们需要进行特殊的判断。

  综上所述，我们这个输出一行字的函数可以被拆分为两个阶段：

  - **预处理**：给出一个格式字符串如`"numa:%d\n"`以及跟随的参数`numa`（这个参数的数量未知），对字符串进行解析，得到一个能够直接进行输出的字符串（相当于直接把后面的参数融合到字符串中）。
  - **输出**：读取预处理完毕的字符串，逐个字符进行输出，输出到屏幕上，这里的输出非常简单，只需要调用上面封装好的操作即可



#### 格式化字符串处理

这里就属于`dirty work`了，我直接就是一手copy（这里主要在做的工作就是解析字符串，然后来区分其中的`%d,%.3f,%*d,`这种的，然后再用后面的参数填充，这里我就不讲了，直接把作者的代码贴过来就可以了），当然，贴过来之前，我还是改了一下的，因为还需要适配我们写的显示模块：

```C
#include <stdarg.h>
#include "printk.h"
#include "lib.h"
#include "linkage.h"
#include "position.h"

extern inline int strlen(char* String);

/**
 * 给出一段字符串，将字符串转换为数字
 * @param s 输入的字符串
 * @return 返回的数字， 整数类型
 */
int skip_atoi(const char **s)
{
	int i=0;

	while (is_digit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}

/**
 * 根据给出的参数，格式化输出一个数字
 * @param str
 * @param num
 * @param base
 * @param size
 * @param precision
 * @param type
 * @return 一个字符串，格式化完成后的该数字
 */
static char * number(char * str, long num, int base, int size, int precision,	int type)
{
	char c,sign,tmp[50];
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	if (type&SMALL) digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if (type&LEFT) type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ' ;
	sign = 0;
	if (type&SIGN && num < 0) {
		sign='-';
		num = -num;
	} else
		sign=(type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
	if (sign) size--;
	if (type & SPECIAL)
		if (base == 16) size -= 2;
		else if (base == 8) size--;
	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num!=0)
		tmp[i++]=digits[do_div(num,base)];
	if (i > precision) precision=i;
	size -= precision;
	if (!(type & (ZEROPAD + LEFT)))
		while(size-- > 0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type & SPECIAL)
		if (base == 8)
			*str++ = '0';
		else if (base==16) 
		{
			*str++ = '0';
			*str++ = digits[33];
		}
	if (!(type & LEFT))
		while(size-- > 0)
			*str++ = c;

	while(i < precision--)
		*str++ = '0';
	while(i-- > 0)
		*str++ = tmp[i];
	while(size-- > 0)
		*str++ = ' ';
	return str;
}



/**
 * 对给出的字符串进行初始化（识别占位符并且根据需求进行填入）
 * @param buf 格式化后的字符串将返回到buf数组中
 * @param fmt 一个字符串，表示需要进行格式化的字符串如："numa : %06d !\n"
 * @param args 用户输入的参数，用于对前面的字符串进行填充
 * @return 格式化后字符串的结束位置
 */
int vsprintf(char * buf,const char *fmt, va_list args)
{
	char * str,*s;
	int flags;
	int field_width;
	int precision;
	int len,i;

	int qualifier;		/* 'h', 'l', 'L' or 'Z' for integer fields */

	for(str = buf; *fmt; fmt++)
	{

		if(*fmt != '%')
		{
			*str++ = *fmt;
			continue;
		}
		flags = 0;
		repeat:
			fmt++;
			switch(*fmt)
			{
				case '-':flags |= LEFT;	
				goto repeat;
				case '+':flags |= PLUS;	
				goto repeat;
				case ' ':flags |= SPACE;	
				goto repeat;
				case '#':flags |= SPECIAL;	
				goto repeat;
				case '0':flags |= ZEROPAD;	
				goto repeat;
			}

			/* get field width */

			field_width = -1;
			if(is_digit(*fmt))
				field_width = skip_atoi(&fmt);
			else if(*fmt == '*')
			{
				fmt++;
				field_width = va_arg(args, int);
				if(field_width < 0)
				{
					field_width = -field_width;
					flags |= LEFT;
				}
			}
			
			/* get the precision */

			precision = -1;
			if(*fmt == '.')
			{
				fmt++;
				if(is_digit(*fmt))
					precision = skip_atoi(&fmt);
				else if(*fmt == '*')
				{	
					fmt++;
					precision = va_arg(args, int);
				}
				if(precision < 0)
					precision = 0;
			}
			
			qualifier = -1;
			if(*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z')
			{	
				qualifier = *fmt;
				fmt++;
			}
							
			switch(*fmt)
			{
				case 'c':

					if(!(flags & LEFT))
						while(--field_width > 0)
							*str++ = ' ';
					*str++ = (unsigned char)va_arg(args, int);
					while(--field_width > 0)
						*str++ = ' ';
					break;

				case 's':
				
					s = va_arg(args,char *);
					if(!s)
						s = '\0';
					len = strlen(s);
					if(precision < 0)
						precision = len;
					else if(len > precision)
						len = precision;
					
					if(!(flags & LEFT))
						while(len < field_width--)
							*str++ = ' ';
					for(i = 0;i < len ;i++)
						*str++ = *s++;
					while(len < field_width--)
						*str++ = ' ';
					break;

				case 'o':
					
					if(qualifier == 'l')
						str = number(str,va_arg(args,unsigned long),8,field_width,precision,flags);
					else
						str = number(str,va_arg(args,unsigned int),8,field_width,precision,flags);
					break;

				case 'p':

					if(field_width == -1)
					{
						field_width = 2 * sizeof(void *);
						flags |= ZEROPAD;
					}

					str = number(str,(unsigned long)va_arg(args,void *),16,field_width,precision,flags);
					break;

				case 'x':

					flags |= SMALL;

				case 'X':

					if(qualifier == 'l')
						str = number(str,va_arg(args,unsigned long),16,field_width,precision,flags);
					else
						str = number(str,va_arg(args,unsigned int),16,field_width,precision,flags);
					break;

				case 'd':
				case 'i':

					flags |= SIGN;
				case 'u':

					if(qualifier == 'l')
						str = number(str,va_arg(args,unsigned long),10,field_width,precision,flags);
					else
						str = number(str,va_arg(args,unsigned int),10,field_width,precision,flags);
					break;

				case 'n':
					
					if(qualifier == 'l')
					{
						long *ip = va_arg(args,long *);
						*ip = (str - buf);
					}
					else
					{
						int *ip = va_arg(args,int *);
						*ip = (str - buf);
					}
					break;

				case '%':
					
					*str++ = '%';
					break;

				default:

					*str++ = '%';	
					if(*fmt)
						*str++ = *fmt;
					else
						fmt--;
					break;
			}

	}
	*str = '\0';
	return str - buf;
}

char buf[500];

/**
 * 给出颜色与需要输出的东西，进行输出
 * @param FRcolor 一个整形数字，表示想要输出的字体颜色
 * @param BKcolor 一个整形数字，表示想要输出的背景颜色
 * @param fmt 一个字符串，表示用户输出的字符串
 * @param ... 可变长的参数列表，表示想要填充到字符串中的参数
 */
int color_printk(unsigned int FRcolor,unsigned int BKcolor,const char * fmt,...)
{
	int i = 0;
	int count = 0;
	int line = 0;
	va_list args;
	va_start(args, fmt);

	i = vsprintf(buf,fmt, args);

	va_end(args);

	for(count = 0;count < i;count++)
	{
		if(buf[count] == '\n') doEnter(&globalPosition);
		else if(buf[count] == '\b') doBackspace(&globalPosition);
		else if(buf[count] == '\t') doTab(&globalPosition);
		else doPrint(&globalPosition, BKcolor, FRcolor, font_ascii[buf[count]]);
        doNext(&globalPosition);
	}
	return i;
}

```



我们调用这个`color_printk`：

```C
doEnter(&globalPosition);
color_printk(YELLOW,BLACK,"Hello World!");
doEnter(&globalPosition);
```

效果如下：

<img src="./pics/lab2/image-20201022035459566.png" alt="image-20201022035459566" style="zoom:50%;" />



#### 关键点1：多文件共享全局变量（讲给和我一样没学好C语言的人）

作者给出的文件的写法上存在一些错误（或者说是不规范的地方），比如我们在多个地方`#include"font.h"`之后，就会有`multiple define`的错误，经过上面的讲解，相信大家也能分析出来出现这样错误的原因：

- 文件A调用了`font.h`
- 文件B也调用了`font.h`
- 预编译、汇编、二进制 文件A，在这个过程中，没有任何错误，并且预编译文件A时已经复制了`font.h`中的内容
- 同理，预编译B的时候也复制了`font.h`中的内容
- 最后一步：链接，这个时候就不对了，A/B都定义了`font_ascii`这个字符数组，那怎么办呢？报错吧

好了，这样就启示我们，不要在`.h`文件中定义数据。那问题就来了，不在`.h`中定义，怎么做到`#include`后开箱即用呢？其实也很简单，我们需要在一个`.c`文件中定义想要全局使用的变量（如`font_ascii`），然后再在`.h`文件中使用关键字`extern `引入该变量即可，我们来看一下在这个项目中，是如何使用这个方法的：

`font.c`

```C
unsigned char font_ascii[256][16] = ......; // 定义数据
```

`font.h`

```C
#ifndef __FONT_H__
#define __FONT_H__

extern unsigned char font_ascii[256][16];

#endif
```

其实如果用户`#include"font.h"`，实际上就是使用了关键词`extern unsigned char font_ascii[256][16];`，来声明要使用这个全局变量。编译的时候，实际上编译器通过`extern`关键词知道我们要使用 `font_ascii`这个字符数组，但是实际上编译器是不知道`font_ascii`在哪里的。但是到了链接的时候，由于我们把`font.c`这个文件一起加进来编译了，所以程序执行的时候能够知道`font_ascii`的值。



#### 关键点2：可变长参数

作者的程序中，`#include <stdarg.h>`是引入了一个库，这个库的主要作用是：他可以让函数能够接收未知数量的变量，那么什么是未知数量的变量呢？

```C
printf("%d,%d,%c\n", numa, numb, charc);
```

我们来分析一下`printf`这个函数的构成：首先是一个字符串`"%d,%d,%c\n"`，然后后面会根据字符串中的占位符的数量跟上很多变量，也就是说：在写程序的时候不知道后面会跟多少参数，那么就可用这个库来解决这个问题，在这个程序中我们将`color_printk`声明成下面的样子：

```C
int color_printk(unsigned int FRcolor,unsigned int BKcolor,const char * fmt,...)
```

这里的`fmt`就相当于`printf`中的字符串，后面的`...`就是未知的参数了，我们来看下如何使用这个库：

- `va_list args;`:创建一个 **va_list** 类型变量
- `va_start(args, fmt)`：给出上一个参数的位置，将args指向第一个可变参数
- `va_arg(args, int)`：取出下一个`int`类型的参数
- `va_end(vl);`：结束标志

##### 可变长参数的原理

这里我们还是来阅读下源码：

```C++
#define va_start(AP, LASTARG)                         \
 (AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))//ap指向下一个参数，lastarg不变
```

这个地方，我们给出的va_list就对应这里的ap，给出的上一个参数`fmt`就对应着这里的`LASTARG`，这里的含义是：`va_list`的第一个元素的内存地址是上一个参数的内存地址+`__va_rounded_size (LASTARG)`。其中`__va_rounded_size (LASTARG)`：

```C
#define __va_rounded_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))
```

那么为什么这样就能指向`va_list`的首地址了呢？其实非常简单，我们不妨想一下函数入栈的操作：从右到左依次入栈；也就是说这些参数在内存中是线性排列的。那就是说：只要我们知道`va_list`前的第一个元素的地址，那么我们就不难知道`va_list`的地址了。

同理，我们想找到`va_list`下一个元素的位置也是非常简单，我们只需要知道这次想要读取的元素占了内存中多少个字节即可。





## 系统异常

首先通过以往的经验来谈一下什么是异常：(谈这些的时候都是基于对mips32架构的理解，而不是x86，所以和这个项目可能有些不同)：

 异常可能来源于中断、陷阱、系统调用、无效指令、溢出等等等等，简而言之就是如果执行到当前出现了cpu无法解决的问题的时候就会抛出异常；这里需要特别注意的是：**异常是由`cpu`抛出的**，**操作系统进行处理的**；在mips中，cpu检测到异常后，会将流水线停掉，将异常信息记录到`cp0`寄存器中，然后跳转到异常处理例程。异常处理例程是由操作系统来决定的，该例程所在内存地址是约定俗成的（固定的，必须装载在固定位置）。



### 异常的分类

作者给出了一些描述：

> - **错误（fault）**。错误是一种可被修正的异常。只要错误被修正，处理器可将程序或任务的运行环境还原至异常发生前（已在栈中保存CS和EIP寄存器值），并重新执行产生异常的指令，也就是说异常的返回地址指向产生错误的指令，而不是其后的位置。
> - **陷阱（trap）**。陷阱异常同样允许处理器继续执行程序或任务，只不过处理器会跳过产生异常的指令，即陷阱异常的返回地址指向诱发陷阱指令之后的地址。
> - **终止（abort）**。终止异常用于报告非常严重的错误，它往往无法准确提供产生异常的位置，同时也不允许程序或任务继续执行，典型的终止异常有硬件错误或系统表存在不合逻辑、非法值。
>
> 当终止异常产生后，程序现场不可恢复，也无法继续执行。当错误异常和陷阱异常产生后，程序现场可以恢复并继续执行，只不过错误异常会重新执行产生异常的指令，而陷阱异常会跳过产生异常的指令。

除此之外，还有一个异常的分类表：

| 向量号 | 助记符 | 异常/中断描述           | 异常/中断类型 | 错误码      | 触发源                          |
| :----- | :----- | :---------------------- | :------------ | :---------- | :------------------------------ |
| 0      | #DE    | 除法错误                | 错误          | No          | DIV或IDIV指令                   |
| 1      | #DB    | 调试异常                | 错误/陷阱     | No          | 仅供Intel处理器使用             |
| 2      | —      | NMI中断                 | 中断          | No          | 不可屏蔽中断                    |
| 3      | #BP    | 断点异常                | 陷阱          | No          | INT 3指令                       |
| 4      | #OF    | 溢出异常                | 陷阱          | No          | INTO指令                        |
| 5      | #BR    | 越界异常                | 错误          | No          | BOUND指令                       |
| 6      | #UD    | 无效/未定义的机器码     | 错误          | No          | UD2指令或保留的机器码           |
| 7      | #NM    | 设备异常（FPU不存在）   | 错误          | No          | 浮点指令WAIT/FWAIT指令          |
| 8      | #DF    | 双重错误                | 终止          | Yes（Zero） | 任何异常、NMI中断或INTR中断     |
| 9      | —      | 协处理器段越界（保留）  | 错误          | No          | 浮点指令                        |
| 10     | #TS    | 无效的TSS段             | 错误          | Yes         | 访问TSS段或任务切换             |
| 11     | #NP    | 段不存在                | 错误          | Yes         | 加载段寄存器或访问系统段        |
| 12     | #SS    | SS段错误                | 错误          | Yes         | 栈操作或加载栈段寄存器SS        |
| 13     | #GP    | 通用保护性异常          | 错误          | Yes         | 任何内存引用和保护检测          |
| 14     | #PF    | 页错误                  | 错误          | Yes         | 任何内存引用                    |
| 15     | —      | Intel保留，请勿使用     | —             | No          | —                               |
| 16     | #MF    | x87 FPU错误（计算错误） | 错误          | No          | x87 FPU浮点指令或WAIT/FWAIT指令 |
| 17     | #AC    | 对齐检测                | 错误          | Yes(Zero)   | 引用内存中的任何数据            |
| 18     | #MC    | 机器检测                | 终止          | No          | 如果有错误码，其与CPU类型有关   |
| 19     | #XM    | SIMD浮点异常            | 错误          | No          | SSE/SSE2/SSE3浮点指令           |
| 20     | #VE    | 虚拟化异常              | 错误          | No          | 违反EPT                         |
| 21-31  | —      | Intel保留，请勿使用     | —             | —           | —                               |
| 32-255 | —      | 用户自定义中断          | 中断          | —           | 外部中断或执行INT n指令         |

当发生异常的时候，我们就可以通过`向量号`来知道异常的类型，同时通过`错误码`来检测异常的原因。







#### 预备知识1：IDT表&中断门

参考：[**《编写操作系统之路》**](https://www.bilibili.com/video/BV127411K72M)

![屏幕截图 2020-10-23 17:38:18](pics/lab2/%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE%202020-10-23%20173818-1603446020768.png)

在前面的学习中，IDT表已经多次出现了，我们每次都在对IDT表进行定义，以及使用`lidt`指令对IDT表进行更新、读取，但我们根本不知道它是干什么的。它的学名叫做**中段描述符表**。在IDT表中保存的是一个个“门”，那么每一个门中都保存着很多信息，在这些信息中最为关键的（我们需要理解的）就是：

- 选择子：就是段选择子
- 偏移：在相应段中的偏移地址

这个门可以被分为：中断门、调用门、陷阱门、任务门，为了方便理解，我们从调用门开始学起。

- 调用门：调用门非常简单，他的作用就是被调用，方法是`call 调用门`，在执行这样的语句的时候实际上就是在做`call 选择子：偏移`，这个过程就等同于调用一个函数，但是于直接调用函数不同，使用调用门可以实现从低特权级到高特权级的转移。比如说：想要从用户模式的代码B调用搞特权级的A，如果直接`call`，那么就会出现错误（保护模式特性），这个时候使用`调用门`就可以解决这个问题。
- **中断门**：中断门和调用门基本相同，唯一的不同点在于在使用中断门时，系统会自动屏蔽掉其他中断



#### 预备知识2：特权级转换实现

参考：

- [**《编写操作系统之路》**](https://www.bilibili.com/video/BV127411K72M) 
- **[总结：特权级之间的转换](https://blog.csdn.net/bfboys/article/details/52420211)**

##### 段内跳转

首先我们来看一下最简单的函数跳转（段内跳转）：

```assembly
; 调用函数
push eax ; 压入参数2
push ebx ; 压入参数1
call function ; 调用

; 被调用的函数
function:
	mov ebx, [esp + 4] ; 取出参数1
	mov ebx, [esp + 8] ; 取出参数2
	...
	ret ; 返回
```

这个过程应当是大家熟悉的，我们将参数压栈的顺序和取出参数的顺序应该是相反的，这是栈的基本特征。但是同时产生了一个问题：为什么取出最后一个`push`进去的参数需要`+4`呢？

- 这是因为我们在使用`call`的时候，处理器默认将下一条需要执行的指令的地址也压入了栈中，这样在执行`ret`结束函数时，可以直接使用栈中的地址进行跳转，来达到继续执行的目的。而这个过程对于用户来说是不可见的。

段内跳转前后的栈内存如下：

![](./pics/lab2/figure4.png)



##### 段间跳转（特权级相同）

上面所讲述的跳转方法是段内跳转的方法。下面我们来看一下段间跳转，首先来看一张图，这张图表示的是长调用且不发生特权级转移时堆栈的变化：

![](pics/lab2/figure5.png)

可以看到，唯一的区别在于长跳转的时候，不仅保存了偏移，还保存了段选择符号，以便回到不同段的代码处继续执行。



##### 段间转换（特权级不同）

首先需要说明的是，如果需要在不同特权级的段之间进行跳转的话，那么是不可以直接使用call +地址的形式来进行跳转的。而是需要使用我们上面说的各种门描述符来进行跳转，使用的方法就是：`call  gate`，这样的方法就能在不同特权级的代码段之间切换了。那么我们来看，如何切换：

在不同特权级下的堆栈段不同，所以每一个任务最多可能在4个特权级间转移，所以，每个任务实际上需要4个堆栈。那么我们就遇到了第一个问题：**如何保存不同特权级下的堆栈？**

其实解决方法非常简单，因为intel已经给我们提供好了，我们使用一个叫做`TSS`的表格来存储cpu当前运行时的很多状态，其中的一项就是我们这里需要存储的：不同栈的`ss和esp`，需要注意的是，特权级`0,1`的都保存在`tss`段 ，是只读的。只有在访问更高特权级的时候，才会创建新的堆栈，同时在调用结束的时候，相应的堆栈也会被销毁，以供下一次调用。



我们来看一下调用前后栈的变化：

<img src="pics/lab2/image-20201023192449112.png" alt="image-20201023192449112" style="zoom: 80%;" />

返回时：

<img src="pics/lab2/image-20201023192522395.png" alt="image-20201023192522395" style="zoom:80%;" />

现在，我们面临着最后一个问题：**中间需要的很多信息从哪里来？**

其实这个问题也很简单。。我们上面不是讲了一个门嘛，门里面含有两个Byte专门用于储存目标代码段的一些属性，比如参数数量等等。



最后我们总结一下在进行特权级切换时的流程：

1. 从门中获得目标的特权级，来让TSS知道该切换到哪个ss和esp
2. 从TSS中读取新的ss和esp
3. 对ss进行校验（如果不存在就会爆出异常）
4. 暂时保存这个时候的ss:esp，也就是调用者的ss，esp
5. 加载新的ss、esp
6. 将刚才保存的ss、esp加载到栈中
7. 将调用者堆栈中的参数拷贝到新的栈中，其中参数的数量由门中属性区域的Param Count来决定，最多只有31个参数
8. 将当前调用者的cs:eip压栈。
9. 加载门中指定的`cs:eip`，开始执行新的代码，这样就完成了特权级变化的跳转



虽然这个过程很复杂， 但是它的原理很简单，如果你没有看懂也没有关系，你可以去网上搜索其他人的讲解。这个过程实际上是被intel公司封装好了的，我们只需要懂它的大概原理就可以进行使用了。下面一节的内容可能与现在的内容有部分重复。



#### 预备知识3：中断处理过程（摘自《一个六十四位操作系统的设计与实现》有部分改动）



我们观察到上面的每一种异常/中断，都对应这样一个向量，发生中断时会用到之前定义过的`IDT中段描述符表`，来检索相应的门描述符，然后再通过地址映射的方法，找到中断处理例程的代码段的地址，最后转移到目标地址进行执行，整个过程如下图所示：

<img src="http://www.ituring.com.cn/figures/2019/OperatingSystemx64/05.d04z.006.png" style="zoom: 50%;" />

当发生中断时，会检测产生中断的程序的特权级，并且与代码寄存器的特权级进行比较：

- 如果中断/异常处理程序的特权级更高，则会在中断/异常处理程序执行前切换栈空间，以下是栈空间的切换过程：

  (1) 处理器会从任务状态段TSS中取出对应特权级的栈段选择子和栈指针，并将它们作为中断/异常处理程序的栈空间进行切换。在栈空间切换的过程中，处理器将自动把切换前的SS和ESP寄存器值压入中断/异常处理程序栈。

  (2) 在栈空间切换的过程中，处理器还会保存被中断程序的EFLAGS、CS和EIP寄存器值到中断/异常处理程序栈。

  (3) 如果异常会产生错误码，则将其保存在异常栈内，位于EIP寄存器之后。

- 如果中断/异常处理程序的特权级与代码段寄存器的特权级相等。

  (1) 处理器将保存被中断程序的EFLAGS、CS和EIP寄存器值到栈中（如图4-7所示）。

  (2) 如果异常会产生错误码，则将其保存在异常栈内，位于EIP寄存器之后。

  <img src="http://www.ituring.com.cn/figures/2019/OperatingSystemx64/05.d04z.007.png" style="zoom:50%;" /> 

  处理器必须借助`IRET`指令才能从异常/中断处理程序返回。

- **异常/中断处理的标志位使用**。当处理器穿过中断门或陷阱门执行异常/中断处理程序时，处理器会在标志寄存器EFLAGS入栈后复位TF标志位，以关闭单步调试功能。（处理器还会复位VM、RF和NT标志位。）在执行`IRET`指令的过程中，处理器会还原被中断程序的标志寄存器EFLAGS，进而相继还原TF、VM、RF和NT等标志位。

  中断门与陷阱门的不同之处在于执行处理程序时对IF标志位（位于标志寄存器EFLAGS中）的操作。

  - 当处理器穿过中断门执行异常/中断处理程序时，处理器将复位IF标志位，以防止其他中断请求干扰异常/中断处理程序。处理器会在随后执行`IRET`指令时，将栈中保存的EFLAGS寄存器值还原，进而置位IF标志位。
  - 当处理器穿过陷阱门执行异常/中断处理程序时，处理器却不会复位IF标志位。

  其实，中断和异常向量同在一张IDT内，只是它们的向量号不同罢了。IDT表的前32个向量号被异常占用，而且每个异常的向量号固定不能更改，从向量号32开始被中断处理程序所用。



### 检测除法错误

通过上面的学习，我们已经掌握了这部分的基本原理，接下来我们通过看作者代码+自己写的方法来进行应用。



#### 初始化IDT表格

回顾下总体的过程，在发生异常的时候，cpu会拿着异常向量在`IDT`表格中进行查找，将代码跳转到相应的表项记录的地址。所以我们就需要对`IDT`表格进行初始化，给每一个异常号分配一个异常处理例程。这个过程非常好理解。

```assembly
setup_IDT:							
	leaq	ignore_int(%rip),	%rdx
	movq	$(0x08 << 16),	%rax
	movw	%dx,	%ax
	movq	$(0x8E00 << 32),	%rcx		
	addq	%rcx,	%rax
	movl	%edx,	%ecx
	shrl	$16,	%ecx
	shlq	$48,	%rcx
	addq	%rcx,	%rax
	shrq	$32,	%rdx
	leaq	IDT_Table(%rip),	%rdi
	mov	$256,	%rcx
rp_sidt:
	movq	%rax,	(%rdi)
	movq	%rdx,	8(%rdi)
	addq	$0x10,	%rdi
	dec	%rcx
	jne	rp_sidt
```

我认为，我们不需要再去单句单句的去理解这段汇编干了什么了，因为说实话。。不是很有意义，我们的重点应该放在这段代码达到了什么目的上面。简单说，这段代码改变了IDT表的值，对IDT表进行了初始化，将idt表的每一项都填上了相同的值，这个值的意思是，当前门会跳转到这个`ignore_int`函数中进行处理，那么他是怎么实现的呢？（我还是会讲，就是不要太在意单个语句的作用，当前阶段学x86投入产出有点低）：

- 首先，在`setup_IDT`中，我们定义了一个标准的门（这个门会跳转到`ignore_int`函数）
- 然后，获取了IDT表的地址`leaq	IDT_Table(%rip),	%rdi`
- 最后，使用循环，每次覆盖IDT表的一项，覆盖256次（这是因为IDT表格一共有256个条目【为什么有256条目？没为什么！】）
- 每执行完一次循环，就会自减`dec	%rcx`，直到覆盖完成后结束：`jne	rp_sidt`。

#### 初始化TSS

我们接着回忆过程，在跳转的时候，我们需要使用TSS来保存、切换状态，那么我们在这里需要给TSS一个初始的状态（TSS也需要初始化）



```assembly
setup_TSS64:
    leaq    TSS64_Table(%rip),    %rdx
    xorq    %rax,    %rax
    xorq    %rcx,    %rcx
    movq    $0x89,   %rax
    shlq    $40,     %rax
    movl    %edx,    %ecx
    shrl    $24,     %ecx
    shlq    $56,     %rcx
    addq    %rcx,    %rax
    xorq    %rcx,    %rcx
    movl    %edx,    %ecx
    andl    $0xffffff,    %ecx
    shlq    $16,     %rcx
    addq    %rcx,    %rax
    addq    $103,    %rax
    leaq    GDT_Table(%rip),    %rdi
    movq    %rax,    64(%rdi)
    shrq    $32,     %rdx
    movq    %rdx,    72(%rdi)

    mov     $0x40,   %ax
    ltr     %ax

    movq    go_to_kernel(%rip),    %rax        /* movq address */
    pushq   $0x08
    pushq   %rax
    lretq
```



这部分程序负责初始化GDT（IA-32e模式）内的TSS Descriptor，并通过LTR汇编指令把TSS Descriptor的选择子加载到TR寄存器中。



#### 异常处理模块

其实我觉得，前面两个部分，知道有这么个东西就可以了，这里才是重要的地方。

```assembly
//=======    ignore_int

ignore_int:
    cld
    pushq    %rax
    pushq    %rbx
    pushq    %rcx
    pushq    %rdx
    pushq    %rbp
    pushq    %rdi
    pushq    %rsi

    pushq    %r8
    pushq    %r9
    pushq    %r10
    pushq    %r11
    pushq    %r12
    pushq    %r13
    pushq    %r14
    pushq    %r15

    movq     %es,     %rax
    pushq    %rax
    movq     %ds,     %rax
    pushq    %rax

    movq     $0x10,   %rax
    movq     %rax,    %ds
    movq     %rax,    %es

    leaq     int_msg(%rip),    %rax            /* leaq get address */
    pushq    %rax
    movq     %rax,    %rdx
    movq     $0x00000000,    %rsi
    movq     $0x00ff0000,    %rdi
    movq     $0,      %rax
    callq    color_printk
    addq     $0x8,    %rsp

Loop:
    jmp      Loop

    popq     %rax
    movq     %rax,    %ds
    popq     %rax
    movq     %rax,    %es

    popq     %r15
    popq     %r14
    popq     %r13
    popq     %r12
    popq     %r11
    popq     %r10
    popq     %r9
    popq     %r8

    popq     %rsi
    popq     %rdi
    popq     %rbp
    popq     %rdx
    popq     %rcx
    popq     %rbx
    popq     %rax
    iretq

int_msg:
    .asciz "Unknown interrupt or fault at RIP\n"
```

到此为止，我们就完成了对异常的监听，并且进行简单的处理（过于简单，直接输出提示，并且宕机），向`main`函数中加入`int a  = 1 / 0`，获得以下结果：

<img src="pics/lab2/image-20201023202828221.png" alt="image-20201023202828221" style="zoom: 50%;" />



### 完善异常处理



#### 添加门

现在给出一段代码，这段代码可以添加一个门：

```assembly
#define _set_gate(gate_selector_addr,attr,ist,code_addr)             \
do                                                                   \
{    unsigned long __d0,__d1;                                        \
    __asm__ __volatile__    (    "movw    %%dx,    %%ax     \n\t"    \
                                 "andq    $0x7,    %%rcx    \n\t"    \
                                 "addq    %4,      %%rcx    \n\t"    \
                                 "shlq    $32,     %%rcx    \n\t"    \
                                 "addq    %%rcx,   %%rax    \n\t"    \
                                 "xorq    %%rcx,   %%rcx    \n\t"    \
                                 "movl    %%edx,   %%ecx    \n\t"    \
                                 "shrq    $16,     %%rcx    \n\t"    \
                                 "shlq    $48,     %%rcx    \n\t"    \
                                 "addq    %%rcx,   %%rax    \n\t"    \
                                 "movq    %%rax,   %0       \n\t"                \
                                 "shrq    $32,     %%rdx    \n\t"                \
                                 "movq    %%rdx,   %1       \n\t"                \
                                 :"=m"(*((unsigned long)(gate_selector_addr))),  \
                                 "=m"(*(1 + (unsigned long *)(gate_selector_addr
                                     ))),"=&a"(__d0), "=&d"(__d1)                \
                                 :"i"(attr << 8),                                \
                                 "3"((unsigned long *)(code_addr)),"2"(0x8 <<
                                 16),"c"(ist)                                    \
                                 :"memory"                                       \
                                       );                                        \
}while(0)
```

实际上我们不需要理解这段代码到底是怎么运作的，我们需要知道的重点是：

- 定义的方法：`define`，当我们输入` _set_gate`的时候，实际上就是将下面的代码全都复制进来了。
- 执行的参数：
  - gate_selector_addr：对应的IDT表的向量号
  - attr： 添加到IDT中的项目的类型（陷阱、中断、系统调用）
  - ist： 用于填充TSS中的内容
  - code_addr：跳转到的目标地址
- 执行的结果：系统遇到中断的时候，会使用中断向量进行检索IDT表，向我们已经定义好的目标地址进行带权跳转。



#### 对添加门的代码进行二次封装

我们在这里对添加门的代码进行二次封装：

```assembly
/**
 * 创建陷阱门
 * @param n 对应的IDT表的条目号
 * @param ist 用于填充TSS
 * @param addr 跳转的目标地址，需要传递一个函数指针
 */ 
inline void set_intr_gate(unsigned int n,unsigned char ist,void * addr) 
{
	_set_gate(IDT_Table + n , 0x8E , ist , addr);	//P,DPL=0,TYPE=E
}

/**
 * 创建一个陷阱门
 * @param n 对应的IDT表的条目号
 * @param ist 用于填充TSS
 * @param addr 跳转的目标地址，需要传递一个函数指针
 */ 
inline void set_trap_gate(unsigned int n,unsigned char ist,void * addr)
{
	_set_gate(IDT_Table + n , 0x8F , ist , addr);	//P,DPL=0,TYPE=F
}

/**
 * 创建一个DPL为3的陷阱门
 * @param n 对应的IDT表的条目号
 * @param ist 用于填充TSS
 * @param addr 跳转的目标地址，需要传递一个函数指针
 */ 
inline void set_system_gate(unsigned int n,unsigned char ist,void * addr)
{
	_set_gate(IDT_Table + n , 0xEF , ist , addr);	//P,DPL=3,TYPE=F
}

/**
 * 创建一个DPL是0的中断门
 * @param n 对应的IDT表的条目号
 * @param ist 用于填充TSS
 * @param addr 跳转的目标地址，需要传递一个函数指针
 */ 
inline void set_system_intr_gate(unsigned int n,unsigned char ist,void * addr)	//int3
{
	_set_gate(IDT_Table + n , 0xEE , ist , addr);	//P,DPL=3,TYPE=E
}

```

这样就可以更加方便的添加各种门了。对应的门的类型已经在代码中注明。

在这里有一个小的细节，我们还是需要讲一下的，我们在当前这个`gate.h`文件中写了这样的代码，尽心了一些变量类型的定义与声明：

```c
struct desc_struct 
{
	unsigned char x[8];
};

struct gate_struct
{
	unsigned char x[16];
};

extern struct desc_struct GDT_Table[];
extern struct gate_struct IDT_Table[];
extern unsigned int TSS64_Table[26];
```

可以看到，这里我们使用了一个`GDT_Table`，但是我们并没有在`.c`文件中进行定义，这是因为我们之前在`head`文件中的`.global`之中进行了定义，定义时的代码段如下：

```assembly
.section .data

.globl GDT_Table

GDT_Table:
	.quad	0x0000000000000000			/*0	NULL descriptor		       	00*/
	.quad	0x0020980000000000			/*1	KERNEL	Code	64-bit	Segment	08*/
	.quad	0x0000920000000000			/*2	KERNEL	Data	64-bit	Segment	10*/
	.quad	0x0020f80000000000			/*3	USER	Code	64-bit	Segment	18*/
	.quad	0x0000f20000000000			/*4	USER	Data	64-bit	Segment	20*/
	.quad	0x00cf9a000000ffff			/*5	KERNEL	Code	32-bit	Segment	28*/
	.quad	0x00cf92000000ffff			/*6	KERNEL	Data	32-bit	Segment	30*/
	.fill	10,8,0					/*8 ~ 9	TSS (jmp one segment <7>) in long-mode 128-bit 40*/
GDT_END:

```

这样的定义方法，实际上和我们上一节中说的在`.c`文件中声明全局变量，然后再在`.h`文件中使用`extern`关键词进行引入相同。



#### 添加各种门

在这一节中，我们会看一下作者是如何添加各种门的

作者给了我们一种实现的方法：

```C
#include "trap.h"

void sys_vector_init()
{
    set_trap_gate(0,1,divide_error);
    set_trap_gate(1,1,debug);
    set_intr_gate(2,1,nmi);
    set_system_gate(3,1,int3);
    set_system_gate(4,1,overflow);
    set_system_gate(5,1,bounds);
    set_trap_gate(6,1,undefined_opcode);
    set_trap_gate(7,1,dev_not_available);
    set_trap_gate(8,1,double_fault);
    set_trap_gate(9,1,coprocessor_segment_overrun);
    set_trap_gate(10,1,invalid_TSS);
    set_trap_gate(11,1,segment_not_present);
    set_trap_gate(12,1,stack_segment_fault);
    set_trap_gate(13,1,general_protection);
    set_trap_gate(14,1,page_fault);
    //15 Intel reserved. Do not use.
    set_trap_gate(16,1,x87_FPU_error);
    set_trap_gate(17,1,alignment_check);
    set_trap_gate(18,1,machine_check);
    set_trap_gate(19,1,SIMD_exception);
    set_trap_gate(20,1,virtualization_exception);

    //set_system_gate(SYSTEM_CALL_VECTOR,7,system_call);
}
```

同理，需要在`trap.h`之中预定义各种各样的处理函数:

```C
#ifndef __TRAP_H__

#define __TRAP_H__

#include "linkage.h"
#include "printk.h"
#include "lib.h"

/*

*/

 void divide_error();
 void debug();
 void nmi();
 void int3();
 void overflow();
 void bounds();
 void undefined_opcode();
 void dev_not_available();
 void double_fault();
 void coprocessor_segment_overrun();
 void invalid_TSS();
 void segment_not_present();
 void stack_segment_fault();
 void general_protection();
 void page_fault();
 void x87_FPU_error();
 void alignment_check();
 void machine_check();
 void SIMD_exception();
 void virtualization_exception();



/*

*/

void sys_vector_init();


#endif

```



在这里，我们定义了一些参数的偏移量：

`entry.S`

```assembly
#include "linkage.h"

R15 =    0x00
R14 =    0x08
R13 =    0x10
R12 =    0x18
R11 =    0x20
R10 =    0x28
R9  =    0x30
R8  =    0x38
RBX =    0x40
RCX =    0x48
RDX =    0x50
RSI =    0x58
RDI =    0x60
RBP =    0x68
DS  =    0x70
ES  =    0x78
RAX =    0x80
FUNC    = 0x88
ERRCODE = 0x90
RIP =    0x98
CS  =    0xa0
RFLAGS =    0xa8
OLDRSP =    0xb0
OLDSS  =    0xb8
```



这里定义的函数会在汇编中实现，实现方法如下：

```assembly
ENTRY(debug)
	pushq	$0
	pushq	%rax
	leaq	do_debug(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
```

这里的`ENTRY`是一个宏定义，是在`linkage.h`之中定义的，定义方法如下：

```C

#define SYMBOL_NAME(X)	X

#define SYMBOL_NAME_STR(X)	#X

#define SYMBOL_NAME_LABEL(X) X##:


#define ENTRY(name)		\
.global	SYMBOL_NAME(name);	\
SYMBOL_NAME_LABEL(name)
```

这个`ENTERY`实际上就是一个声明全局函数的方法。



在这个叫做`debug`的函数的最后，会进入到`error_code`这个代码段，改代码段如下：

```assembly
error_code:
	pushq	%rax
	movq	%es,	%rax
	pushq	%rax
	movq	%ds,	%rax
	pushq	%rax
	xorq	%rax,	%rax

	pushq	%rbp
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15	
	
	cld
	movq	ERRCODE(%rsp),	%rsi
	movq	FUNC(%rsp),	%rdx	

	movq	$0x10,	%rdi
	movq	%rdi,	%ds
	movq	%rdi,	%es

	movq	%rsp,	%rdi
	////GET_CURRENT(%ebx)

	callq 	*%rdx

	jmp	ret_from_exception	
```

可以看到，这个代码段实际上就是把各种寄存器的状态push到了栈中，然后调用了相应的方法，调用方法结束后跳转到`ret_from_exception`代码段进行复原，复原的代码段定义如下：

```assembly
ret_from_exception:
	/*GET_CURRENT(%ebx)	need rewrite*/
ENTRY(ret_from_intr)
	jmp	RESTORE_ALL	/*need rewrite*/
```



而RESTORE_ALL就像下面一样：它的作用也非常简单，就是把各种push进来的都pop出去即可

```assembly
RESTORE_ALL:
    popq    %r15;
    popq    %r14;
    popq    %r13;
    popq    %r12;
    popq    %r11;
    popq    %r10;
    popq    %r9;
    popq    %r8;
    popq    %rbx;
    popq    %rcx;
    popq    %rdx;
    popq    %rsi;
    popq    %rdi;
    popq    %rbp;
    popq    %rax;
    movq    %rax,    %ds;
    popq    %rax;
    movq    %rax,    %es;
    popq    %rax;
    addq    $0x10,   %rsp;
    iretq;
```





所以说，在整个过程中，汇编语言执行的是：将现场压入栈中，调用C语言函数进行处理、等待函数执行完毕、恢复现场的作用。我们来看一下div的异常处理：

```C
void do_divide_error(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_divide_error(0),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}
```

这个处理方式也是非常简单的，就是直接将错误的信息输出了。





同理，我们再来看一个：

```assembly
ENTRY(invalid_TSS)
    pushq    %rax
    leaq     do_invalid_TSS(%rip),    %rax
    xchgq    %rax,    (%rsp)
    jmp      error_code
```

这个与前面的debug有一些区别，区别在于：`invalid_TSS`是一个有返回错误码的中断，intel的cpu直接隐式的帮助我们将错误码push到栈中了，所以我们就不需要重复push了，我们之前的时候push 0 实际上就是将`error_code`置为了0.



最后，我们来看一个特殊的（唯一一个特殊的）：

```assembly

ENTRY(nmi)
	pushq	%rax
	cld;			
	pushq	%rax;	
	
	pushq	%rax
	movq	%es,	%rax
	pushq	%rax
	movq	%ds,	%rax
	pushq	%rax
	xorq	%rax,	%rax
	
	pushq	%rbp;		
	pushq	%rdi;		
	pushq	%rsi;		
	pushq	%rdx;		
	pushq	%rcx;		
	pushq	%rbx;		
	pushq	%r8;		
	pushq	%r9;		
	pushq	%r10;		
	pushq	%r11;		
	pushq	%r12;		
	pushq	%r13;		
	pushq	%r14;		
	pushq	%r15;
	
	movq	$0x10,	%rdx;	
	movq	%rdx,	%ds;	
	movq	%rdx,	%es;
	
	movq	$0,	%rsi
	movq	%rsp,	%rdi

	callq	do_nmi

	jmp	RESTORE_ALL
```



`#NMI`不可屏蔽中断不是异常，而是一个外部中断，从而不会生成错误码。`#NMI`应该执行中断处理过程，这段程序最后会跳转到`do_nmi`函数进行处理。了解了我们处理函数的入口后，我们接下来看一下几种错误码的组成。

在继续前进之前，我们先来梳理一下现在学习的内容，我画了一张图方便大家理解：

<img src="pics/lab2/%E5%BC%82%E5%B8%B8%E5%A4%84%E7%90%86%E8%BF%87%E7%A8%8B.png" style="zoom: 33%;" />





#### \#TS异常错误码格式

<img src="pics/lab2/05.d04z.009.png" alt="img" style="zoom:50%;" />

- 段选择子:索引IDT,GDT,LDT等描述符表内的描述符
- EXT: 如果EXT是1,就说明当前异常是在程序向外投递外部事件的过程中触发,比如说想抛出异常结果抛出异常的过程的时候异常了.
- IDT: 如果是1,就说明段选择子记录的是IDT表中的描述符;为0说明是GDT或LDT的内容.
- TI: 当IDT是0的时候生效, 当TI为1则说明段选择子指向LDT内的描述符,否则指向GDT的描述符.

为了将这些错误进行报告,我们这样封装即可:

```C
/**
 * 无效的TSS段, 可能发生在:访问TSS段或者任务切换时(这个时候也访问TSS)
 * 包含一个错误码, 该错误码由五个部分组成:
 * - 段选择子\ TI\ IDT\ EXT _ 保留位
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 * 
 */ 
void do_invalid_TSS(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk("do_invalid_TSS(10),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	switch (error_code & 6) {
        case 2: printk("Ref to IDT");break; // IDT
        case 4: printk("Ref to LDT");break; // LDT
        case 0: printk("Ref to GDT");break; // GDT
        default: printk("Ref ERROR!!!");break; // 错误码出错了
    }

	printk("Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1);
}
```

为了输出方便,我封装了`printk`宏,该宏自动将背景颜色设置为黑色,将字体设置为白色(他的实现与color_printk)完全相同,只不过默认了颜色参数.

#### \#PF异常错误码格式

PF错误是页错误,在进行内存引用时可能被触发：

<img src="pics/lab2/05.d04z.010.png" alt="img" style="zoom: 50%;" />

不同位置所代表的含义已经在上面被列举出来了，我们当前阶段只需要根据不同的情况进行输出即可。下面是处理程序：

```C
/**
 * 页错误，在访问内存出错时出现, 输出相应的错误类型
 * @param rsp 段基址
 * @param error_code 错误码,由段选择子\ TI\ IDT\ EXT _ 保留位五个部分组成
 */
void do_page_fault(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	unsigned long cr2 = 0;

	__asm__	__volatile__("movq	%%cr2,	%0":"=r"(cr2)::"memory");

	p = (unsigned long *)(rsp + 0x98);
	printk("do_page_fault(14),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	// 如果是0，就说明是缺页异常， 否则就说明是页级保护异常
	if(!(error_code & 0x01)) printk("Page Not-Present,\t");
	else printk("Page is Protected!\t"); 

	// 检查W/R位，如果为1就说明写入错误，否则读取错误
	if(error_code & 0x02) printk("Write Cause Fault,\t");
	else printk("Read Cause Fault,\t");

	// 检查U/S位，1代表普通用户访问时出现错误，否则超级用户访问时错误
	if(error_code & 0x04) printk("Fault in user(3)\t");
	else printk("Fault in supervisor(0,1,2)\t");

	// RSVD位，如果为1则说明页表的保留项引发异常
	if(error_code & 0x08) printk(",Reserved Bit Cause Fault\t");

	// I/D位，如果为1则说明获取指令时发生异常
	if(error_code & 0x10) printk(",Instruction fetch Cause Fault");

	printk("\n");
	printk("CR2:%#018lx\n",cr2);

	while(1);
}
```

相信到此为止，大家一定对异常处理的入口有了一定的理解了。我们的异常处理部分也到此为止了，如果大家有兴趣的话，可以在本次提交的源代码仓库中查看如何`printk`各种异常。当然，这样的做法是没有什么意义的，它只能让我们看到发生了什么样的异常，然后宕机，而不能进行真正的处理，我们可能需要在后面再进行真正的处理。



#### 编译验证

##### 编译

相信大家也有相同的体会，kernel的makefile脚本，随着文件数量的增多，变得越来越长了，为了解决这个问题，这里更改了makefile脚本，如下：

```makefile
C_FILE_LIST=main position printk font gate trap
C_FILE_BUILD_GOALS=$(foreach file, $(C_FILE_LIST), ./build/$(file).o)

ASM_FILE_LIST=head entry
ASM_FILE_BUILD_GOALS=$(foreach file, $(ASM_FILE_LIST), ./build/$(file).o)

define \n


endef

all: system
# 生成kernel.bin
        objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary ./build/system ./build/kernel.bin

system: Kernel.lds build_all_c_code build_all_asm_code
# 
        ld -b elf64-x86-64 $(ASM_FILE_BUILD_GOALS) $(C_FILE_BUILD_GOALS) -T Kernel.lds -o ./build/system

build_all_c_code:
        $(foreach file, $(C_FILE_LIST), gcc -mcmodel=large -fno-builtin -fno-stack-protector -m64 -c $(file).c -o ./build/$(file).o  ${\n})

build_all_asm_code:
        $(foreach file, $(ASM_FILE_LIST), gcc -E $(file).S > ./build/$(file).s  ${\n} as --64 -o ./build/$(file).o ./build/$(file).s ${\n})

clean: 
        rm -rf ./build/*

```

这里的实现方式就是循环编译没一个文件，然后最后在一起链接。

##### 验证

这里对main.c进行了更改，这里对异常处理函数进行了限定， 对TSS进行了读取与初始化，相应的，我们需要将head.S中读取TSS的代码删除，以避免重复读取，产生错误：

```C
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

    int a = 1 / 0;

	while(1){
        ;
    }
```



最后执行结果如下：

<img src="pics/lab2/image-20201026024056882.png" alt="image-20201026024056882" style="zoom:67%;" />

可以看到，屏幕上已经打印出了相应的错误。



## 初级内存管理单元

在这一节中，我们讨论的是入门级的内存管理内容主要包括下面三个部分：

- 获取物理内容信息
- 统计可用物理内存页数量
- 分配物理页



### 简单回顾

在开始这一节的学习之前，我们来回顾一下上一章中，操作系统启动的过程中都做了些什么：

<img src="pics/lab2/操作系统启动流程.png" style="zoom: 33%;" />

注意我标记为黑色的部分，在loader中，我们已经对计算机的内存信息进行了检查，并且将检测的信息进行了存储，相应的代码如下：

```assembly
;=======	获取内存的类型及其对应的地址范围

	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0400h		;row 4
	mov	cx,	44
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetMemStructMessage
	int	10h

	mov	ebx,	0
	mov	ax,	0x00
	mov	es,	ax
	mov	di,	MemoryStructBufferAddr	

Label_Get_Mem_Struct:

	mov	eax,	0x0E820
	mov	ecx,	20
	mov	edx,	0x534D4150
	int	15h
	jc	Label_Get_Mem_Fail
	add	di,	20
    inc	dword	[MemStructNumber]

	cmp	ebx,	0
	jne	Label_Get_Mem_Struct
	jmp	Label_Get_Mem_OK

Label_Get_Mem_Fail:

    mov	dword	[MemStructNumber],	0

	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0500h		;row 5
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetMemStructErrMessage
	int	10h

Label_Get_Mem_OK:
	
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0600h		;row 6
	mov	cx,	29
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetMemStructOKMessage
	int	10h	

```

这里我们通过`int 15h`中断，将一个个内存块的信息进行保存，我们不用完整的复习`int 15h`的相关内容，只需知道三个关键点：

- 一、 `int 15h`将内存信息保存到了 `es:di`，这里每次做完一次之后`di`都会增加20

- 二、存储的内存信息格式如下：

  | 偏移 | 名称         | 意义                 |
  | ---- | ------------ | :------------------- |
  | 0    | BaseAddrLow  | 基地址的低 32 位     |
  | 4    | BaseAddrHigh | 基地址的高 32 位     |
  | 8    | LengthLow    | ⻓度(字节)的低 32 位 |
  | 12   | LengthHigh   | ⻓度(字节)的高 32 位 |
  | 16   | Type         | 这个内存区域的类型   |

  我们可以看到，这里的信息实际上就是在用`起点 + 长度`的方法来描述一块内存，其中，内存类型如下：

  | 取值 | 名称                 | 意义                                                         |
  | ---- | -------------------- | ------------------------------------------------------------ |
  | 1    | AddressRangeMemory   | 可以被OS使用的内存                                           |
  | 2    | AddressRangeReserved | 正在使用的区域或者不能系统保留不能使用的区域                 |
  | 其他 | 未定义               | 各个具体机器会有不同的意义，在这里我们暂时不用关心，将它视为AddressRangeReserved即可 |

- 三、这些信息被保存在`0x7e00`地址下



那剩下的事情就变得简单且枯燥了，我们只需要从`0x7e00`开始读取，一次读取20字节，20字节中包含5个整形，每个整型变量都表示一个参数。剩下的事情都会变得非常自然，我们只需要读取即可。



### 读取内存分布

在这里，我们新建一个文件`memory.h/memory.c`来描述内存相关的操作，为了描述每个内存块，我们建立一个和上面表格结构相同的结构体：

```C
/**
 * 用于描述一个内存块的结构体
 * baseAddrHigh : baseAddrLow 拼接起来就是基地址
 * lengthHigh : lengthLow 拼接起来就是内存块长度
 * type 内存类型， 1 ：可被os使用， 2 ： 正在使用或不可使用 其他：没有意义
 */
struct Memory_Block_E820{
    unsigned int baseAddrLow;
    unsigned int baseAddrHigh;
    unsigned int lengthLow;
    unsigned int lengthHigh;
    unsigned int type;
};
```

除此之外，我们还需要一个函数来对内存信息进行初始化，以读取存储在内存`0x7e00`为首的内存信息,就叫他`init_memory`,我们在`memory.h`中对其进行声明，在`memory.c`中对其进行实现即可：

```C
/**
 * 读取内存中存储的内存块信息
 * 默认读取基地址为0x7e00，这里填写线性地址
 * 会输出每一块内存的信息，并且输出总可用内存
 */ 
void init_memory(){
	unsigned long TotalMem = 0 ;
	struct Memory_Block_E820 *p = NULL;	
	
	printk("Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
	p = (struct Memory_Block_E820 *)0xffff800000007e00; // 将基地址对齐

    // 循环遍历，输出内存信息
	for(int i = 0;i < 32;i++){

        // 输出内存信息
		printk("Address:%#010x,%08x\tLength:%#010x,%08x\tType:%#010x\n", \
            p->baseAddrHigh,p->baseAddrLow,p->lengthHigh,p->lengthLow,p->type);

        // 统计可用内存
		unsigned long tmp = 0;
		if(p->type == 1){ // 如果内存可用
			tmp = p->lengthHigh;
			TotalMem +=  p->lengthLow;
			TotalMem +=  tmp  << 32;
		}

		if(p->type > 4)
			break;		
	}

    // 输出总可用内存
	printk("OS Can Used Total RAM:%#018lx\n",TotalMem);
}

```

这个函数并没有进行存储，而直接将信息进行了输出，我们在`main`中添加相应的输出，进行查看（需要注意的是，你需要把除0错误去掉，要不然会宕机），更改后的程序片段如下：

```C
sys_vector_init(); // 初始化IDT表，确定各种异常的处理函数

init_memory(); // 输出所有内存信息

while(1){
    ;
}
```

由于我们新建了一个`memory`，所以我们需要向`makefile`文件中添加相应的描述：

```makefile
C_FILE_LIST=main position printk font gate trap memory
C_FILE_BUILD_GOALS=$(foreach file, $(C_FILE_LIST), ./build/$(file).o)
```

添加的方法非常简单，我们只需要将memory添加到`C_FILE_LIST`的尾部即可，可见我们之前的写法带来了极大的便利。

运行结果如下：

<img src="pics/lab2/image-20201027005727113.png" alt="image-20201027005727113" style="zoom:50%;" />

我们的这个虚拟机一共有`0x7ff8f000`的内存，换算一下大约是2G，这与我们之前做配置的时候分配的值是相近的，不完全相同是因为我们其他部分还占用了一部分的内存。

