# 64位操作系统——bootLoader





## 第一部分： 写一个简单的引导程序并且显示一些字符

```assembly
    org 0x7c00 ; 将程序加载到0x7c00位置，即：指定程序的起始地址

BaseOfStack equ 0x7c00


; 将CS寄存器的段基址设置到DS、ES、SS中
Label_Start:

    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BaseOfStack

; ======  清空屏幕

    mov ax, 0600h
    mov bx, 0700h
    mov cx, 0
    mov dx, 184fh
    int 10h

; ===== set focus ??? 没看懂这是啥意思

    mov ax, 0200h
    mov bx, 0000h
    mov dx, 0000h
    int 10h

; ===== 在屏幕上显示：Start Booting......

    mov ax, 1301h
    mov bx, 000fh
    mov dx, 0000h
    mov cx, 10
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, StartBootMessage
    int 10h

; ===== 软盘驱动器复位

    xor ah, ah
    xor dl, dl
    int 13h

    jmp $


StartBootMessage:	db	"Start Boot"

;=======	fill zero until whole sector

	times	510 - ($ - $$)	db	0
	dw	0xaa55

```



这个程序可以被分为几个部分：

- 初始化部分： 定义了程序被加载到的地方，以及程序中的常量，并且对所有寄存器进行了简单的初始化
- 显示部分：  通过`int 10h`的指令，进行了显示方面的一系列处理
- 软盘驱动器复位：这里我暂时也没搞明白是在干什么
- 填充部分



### 简单的汇编知识

在搞明白这些之前，我们需要先学一点简单的汇编：

我们会发现，上面的程序主要是在对一些寄存器进行简单的操作，这些寄存器是：`ax,bx,cx,dx`他们是nasm语言的通用寄存器。同时，这些寄存器可以根据高位、低位被划分成：`ah,al,bh,bl,ch,cl,dh,dl`，可以理解为：`ax = ah << 16 + al`也就是说`ax`是由`ah\al`两个部分拼接而成的。



理解了这一点之后，你往下看，暂时就没有太大的难度了。



### int 指令

我们首先来看一下这里的`int`指令，学过组成原理的同学都知道，这里的int就是软件中断，我们这里用到了两种int指令，他们实际上就是调用了BIOS里面的一些固有程序，进行了一些我们期望中的处理，我们来看一下这些`int`分别做了哪些操作：



#### 第一次

```assembly
; ======  清空屏幕
mov ax, 0600h
mov bx, 0700h
mov cx, 0
mov dx, 184fh
int 10h
```



这里设置了`ax = 0600h`,`bx=0700h`,`cx=0`,`dx=184fh`,然后调用了`int 10h`,`INT 10h`主要执行的是一些与显示相关的操作。我们来看一下这里的操作：

首先来看`ax`寄存器：我们经过划分可以知道：`ah = 0x06`，`al = 0x00`，这里的ah指示得是执行`INT 10h`相关程序中的哪种操作：当`ah = 0x06`时，执行的是**按指定范围滚动窗口的功能**。

具体的参数如下：

>- AL=滚动的列数，若为0则实现清空屏幕功能；
>- BH=滚动后空出位置放入的属性；
>- CH=滚动范围的左上角坐标列号；
>- CL=滚动范围的左上角坐标行号；
>- DH=滚动范围的右下角坐标列号；
>- DL=滚动范围的右下角坐标行号；
>- BH=颜色属性。
>  - bit 0~2：字体颜色（0：黑，1：蓝，2：绿，3：青，4：红，5：紫，6：综，7：白）。
>  - bit 3：字体亮度（0：字体正常，1：字体高亮度）。
>  - bit 4~6：背景颜色（0：黑，1：蓝，2：绿，3：青，4：红，5：紫，6：综，7：白）。
>  - bit 7：字体闪烁（0：不闪烁，1：字体闪烁）。

这里我们将`AL `设置为了0， 所以实现的是清空屏幕的功能。



#### 第二次

```assembly
; ===== set focus ??? 没看懂这是啥意思
    mov ax, 0200h
    mov bx, 0000h
    mov dx, 0000h
    int 10h
```

这里作者给出的代码中写的是`set focus`，我们还是来看一下`ah`的值：`ah = 0x02`，`ah = 0x02`时`INT 10h`执行**屏幕光标位置的设置功能**。

> - DH=游标的列数；
> - DL=游标的行数；
> - BH=页码。





#### 第三次

```assembly
; ===== 在屏幕上显示：Start Booting......

    mov ax, 1301h
    mov bx, 000fh
    mov dx, 0000h
    mov cx, 10
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, StartBootMessage
    int 10h
```

相信大家已经掌握了这套研究的方法了，我们直入主题： 这里执行的是显示字符串的方法，字符串存储在`StartBootMessage`位置，参数如下：

> - AL=写入模式。
>   - AL=00h：字符串的属性由BL寄存器提供，而CX寄存器提供字符串长度（以B为单位），显示后光标位置不变，即显示前的光标位置。
>   - AL=01h：同AL=00h，但光标会移动至字符串尾端位置。
>   - AL=02h：字符串属性由每个字符后面紧跟的字节提供，故CX寄存器提供的字符串长度改成以Word为单位，显示后光标位置不变。
>   - AL=03h：同AL=02h，但光标会移动至字符串尾端位置。
> - CX=字符串的长度。
> - DH=游标的坐标行号。
> - DL=游标的坐标列号。
> - ES:BP=>要显示字符串的内存地址。
> - BH=页码。
> - BL=字符属性/颜色属性。
>   - bit 0~2：字体颜色（0：黑，1：蓝，2：绿，3：青，4：红，5：紫，6：综，7：白）。
>   - bit 3 ：字体亮度（0：字体正常，1：字体高亮度）。
>   - bit 4~6：背景颜色（0：黑，1：蓝，2：绿，3：青，4：红，5：紫，6：综，7：白）。
>   - bit 7：字体闪烁（0：不闪烁，1：字体闪烁）

我们注意到这里执行的是光标移动到字符串尾端位置的写入模式，通过规定cx，规定了字符串的长度。我们尝试加长字符串的长度，但不更改cx会发生什么样的事情：我们将下面的`StartBootMessage`部分更改为：

```assembly
StartBootMessage:	db	"01234567891011121314151617181920"
```

随后进行查看，结果如下：

<img src="/home/wangsy/Code/os/docs/pics/尝试不更改长度.png" style="zoom: 50%;" />

我们对`cx`进行修改后即可显示完全：

```assembly
    mov cx, 0x20
```

<img src="/home/wangsy/Code/os/docs/pics/lab1/image-20200930162323357.png" alt="image-20200930162323357" style="zoom:50%;" />



### 其他值得一提的



####  `$` 与  `$$`

在nasm语言中，`$`代表当前行的地址，`$$`代表本段程序起始地址。那么我们来看：

```assembly
;=======	fill zero until whole sector

	times	510 - ($ - $$)	db	0
	dw	0xaa55
```

这里的`($ - $$)`就是当前行地址减掉程序开始的地址也就是当前程序占用的空间，我们的第一个扇区大小必须是`512Byte`，并且必须以`0xaa55`作为结尾，所以我们使用0来填充剩下的部分。





####     org 0x7c00

这里的意思，我们在注释里已经描述过了：

```assembly
    org 0x7c00 ; 将程序加载到0x7c00位置，即：指定程序的起始地址
```

相信大家看到这里都会想，为什么要加载到`0x7c00`呢？实际上，不是我们不想加载到更前面，而是因为更前面已经被`BIOS`的标准例程给占上了，所以我们只能使用可以使用的最前面的空间了。





## 第二部分：加载Loader到内存



### 先导知识：FAT12文件系统

为了使操作简单一些，作者在这里使用的是`FAT12`文件系统，我们这里主要来了解一下`FAT12`文件系统。



#### 引导扇区



首先来看引导扇区的结构：

> | 名称              | 偏移 | 长度 | 内容                                        | 本系统引导程序数据        |
> | :---------------- | :--- | :--- | :------------------------------------------ | :------------------------ |
> | `BS_jmpBoot`      | 0    | 3    | 跳转指令                                    | jmp short Label_Start nop |
> | `BS_OEMName`      | 3    | 8    | 生产厂商名                                  | 'MINEboot'                |
> | `BPB_BytesPerSec` | 11   | 2    | 每扇区字节数                                | 512                       |
> | `BPB_SecPerClus`  | 13   | 1    | 每簇扇区数                                  | 1                         |
> | `BPB_RsvdSecCnt`  | 14   | 2    | 保留扇区数                                  | 1                         |
> | `BPB_NumFATs`     | 16   | 1    | FAT表的份数                                 | 2                         |
> | `BPB_RootEntCnt`  | 17   | 2    | 根目录可容纳的目录项数                      | 224                       |
> | `BPB_TotSec16`    | 19   | 2    | 总扇区数                                    | 2880                      |
> | `BPB_Media`       | 21   | 1    | 介质描述符                                  | 0xF0                      |
> | `BPB_FATSz16`     | 22   | 2    | 每FAT扇区数                                 | 9                         |
> | `BPB_SecPerTrk`   | 24   | 2    | 每磁道扇区数                                | 18                        |
> | `BPB_NumHeads`    | 26   | 2    | 磁头数                                      | 2                         |
> | `BPB_HiddSec`     | 28   | 4    | 隐藏扇区数                                  | 0                         |
> | `BPB_TotSec32`    | 32   | 4    | 如果BPB_TotSec16值为0，则由这个值记录扇区数 | 0                         |
> | `BS_DrvNum`       | 36   | 1    | int 13h的驱动器号                           | 0                         |
> | `BS_Reserved1`    | 37   | 1    | 未使用                                      | 0                         |
> | `BS_BootSig`      | 38   | 1    | 扩展引导标记（29h）                         | 0x29                      |
> | `BS_VolID`        | 39   | 4    | 卷序列号                                    | 0                         |
> | `BS_VolLab`       | 43   | 11   | 卷标                                        | 'boot loader'             |
> | `BS_FileSysType`  | 54   | 8    | 文件系统类型                                | 'FAT12'                   |
> | 引导代码          | 62   | 448  | 引导代码、数据及其他信息                    |                           |
> | 结束标志          | 510  | 2    | 结束标志0xAA55                              | 0xAA55                    |

下图所展示的就是FAT12格式组织的软盘的文件系统分配图：

<img src="http://www.ituring.com.cn/figures/2019/OperatingSystemx64/04.d03z.005.png" style="zoom:50%;" />



#### FAT表

FAT12用簇作为基本单位来分配数据区的存储空间。我们来回顾一下上面所说到的几个变量：

- `BPB_SecPerClus`： 每个扇区的字节数
- `BPB_SecPerClus`：每个簇的扇区数

那么我们也可以知道：`簇中字节数 =  BPB_SecPerClus * BPB_SecPerClus`，数据区的簇号与FAT表的表项是一一对应的关系，因此文件在FAT系统总的存储单位是簇。即使文件的大小不足一个簇， 也会为该文件分配一个簇的空间进行存储。这种存储方式与操作系统中的页表非常相似，它的意义在于将磁盘存储空间按照固定存储片有效管理，从而可以按照文件偏移，分片段的访问文件内的数据，不必一次将文件内的数据全部取出。

下面我们来看FAT表的存储结构：

> | FAT项 | 实例值 | 描述                                        |
> | :---- | :----- | :------------------------------------------ |
> | 0     | FF0h   | 磁盘标示字，低字节与`BPB_Media`数值保持一致 |
> | 1     | FFFh   | 第一个簇已经被占用                          |
> | 2     | 003h   | 000h：可用簇                                |
> | 3     | 004h   | 002h~FEFh：已用簇，标识下一个簇的簇号       |
> | ……    | ……     | FF0h~FF6h：保留簇                           |
> | N     | FFFh   | FF7h：坏簇                                  |
> | N+1   | 000h   | FF8h~FFFh：文件的最后一个簇                 |
> | ……    | ……     |                                             |

可以看到，FAT表的存储结构与单向链表的存储方式非常相似。

#### 根目录区和数据区

根目录区和数据区是两个不同的分区，他们都保存着文件相关的数据，但是根目录区只能保存目录项信息，而数据区不仅保存目录项信息还保存文件内的数据。这里的目录项是一个32B的结构体，记录着名字、长度、数据起始簇号等信息。对于树状的目录结构而言，树的层级结构是由目录项结构建立，从根目录开始经过目录项的逐层嵌套渐渐形成了 树的结构（非常像TreeNode）。

| 名称         | 偏移 | 长度 | 描述                 |
| :----------- | :--- | :--- | :------------------- |
| DIR_Name     | 0x00 | 11   | 文件名8 B，扩展名3 B |
| DIR_Attr     | 0x0B | 1    | 文件属性             |
| 保留         | 0x0C | 10   | 保留位               |
| DIR_WrtTime  | 0x16 | 2    | 最后一次写入时间     |
| DIR_WrtDate  | 0x18 | 2    | 最后一次写入日期     |
| DIR_FstClus  | 0x1A | 2    | 起始簇号             |
| DIR_FileSize | 0x1C | 4    | 文件大小             |



### 开始书写引导区





#### 变量约束

首先在头部添加代码

```assembly
    org 0x7c00 ; 将程序加载到0x7c00位置，即：指定程序的起始地址

BaseOfStack             equ 0x7c00

BaseOfLoader            equ 0x1000
OffsetOfLoader          equ 0x00

RootDirSectors          equ 14
SectorNumOfRootDirStart equ 19
SectorNumOfFAT1Start    equ 1
SectorBalance           equ 17

    jmp short Label_Start ; 这里是BS_jmpBoot 
    ; 实现的是段内转移，如果转移范围超过128，那么就会出错
    nop
    BS_OEMName          db  'WSYuboot'  ; 表示的是生产商的名字，我不要脸一点直接写自己名字了
    BPB_BytesPerSec     dw  512         ; 每个扇区的字节数
    BPB_SecPerClus      db  1           ; 每个簇的扇区数
    BPB_RsvdSecCnt	    dw	1           ; 保留扇区数
	BPB_NumFATs	        db	2           ; FAT表的数量
	BPB_RootEntCnt	    dw	224         ; 根目录可容纳的目录项数
	BPB_TotSec16	    dw	2880        ; 总扇区数
	BPB_Media	        db	0xf0        ; 介质描述符
	BPB_FATSz16	        dw	9           ; 每FAT扇区数
	BPB_SecPerTrk	    dw	18          ; 每磁道扇区数
	BPB_NumHeads	    dw	2           ; 磁头数
	BPB_HiddSec	        dd	0           ; 隐藏扇区数
	BPB_TotSec32	    dd	0           
	BS_DrvNum	        db	0           ; int 13h 的驱动器号
	BS_Reserved1	    db	0           ; 未使用
	BS_BootSig	        db	0x29        ; 扩展引导标记
	BS_VolID	        dd	0           ; 卷序列号
	BS_VolLab	        db	'boot loader' ; 卷标
	BS_FileSysType	    db	'FAT12   '  ; 文件系统类型
```

这里各部分的信息，我们在注释里面已经写得非常详细了，其实这些就对应这上面的FAT12文件系统引导扇区的部分。同时，我们在上面进行了一些宏定义，宏定义的内容包括：`BaseOfLoader + OffsetOfLoader`这两个分别代表Loader的起始物理地址和偏移量，共同构成了Loader的起始物理地址，构成方法如下：
$$
BaseOfLoader << 4 + OffsetOfLoader = 0 \times 10000
$$
`RootDirSectors`定义的是**根目录占用的扇区数**，这个值是通过计算得到的：
$$
\begin{align}
(BPB\_RootEntCnt * 32 + BPB\_BytesPerSec - 1) / BPB\_BytesPerSec
\end{align}
$$
实质上就是：根目录容纳的目录项数*32（每个项目的大小）除以每个扇区的字节数，这样就可以得到占用的扇区数，后面加上了扇区的字节数 - 1，是为了对结果做上取整的。

`SectorNumOfRootDirStart`记录的是根目录起始的扇区号，他也是通过计算而得的，他的计算方法如下：
$$
保留扇区数 +FAT表扇区数 * FAT表份数
$$
`SectorNumOfFAT1Start`记录的是起始扇区号，在FAT1表前面只有一个保留扇区，而且它的扇区编号是0，那么FAT1标的起始扇区号是1.

`SectorBalance`用于平衡文件的起始簇号与数据区起始簇号的差值。由于数据区对应的有效簇号为2，为了正确计算出FAT表项对应的数据起始扇区号，则必须将FAT表项值减2，或者将数据区的起始簇号减2。这里用的是一种比较取巧的方法，就是将根目录区的起始扇区号减2，从而间接地把数据区的起始扇区号减2.





#### 软盘读取