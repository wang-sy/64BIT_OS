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

