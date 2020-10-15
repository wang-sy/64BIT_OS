org    10000h
    jmp    Label_Start

%include    "fat12.inc" ; 引用了一个文件
;这个文件就在同目录下的fat12.inc中，其中储存着fat12的一些基本信息


BaseOfKernelFile          equ    0x00
OffsetOfKernelFile        equ    0x100000

BaseTmpOfKernelAddr       equ    0x00
OffsetTmpOfKernelFile     equ    0x7E00

MemoryStructBufferAddr    equ    0x7E00

[SECTION gdt]

LABEL_GDT:		dd	0,0
LABEL_DESC_CODE32:	dd	0x0000FFFF,0x00CF9A00 ; 两个字拼接而成
LABEL_DESC_DATA32:	dd	0x0000FFFF,0x00CF9200 ; 双字

GdtLen	equ	$ - LABEL_GDT
GdtPtr	dw	GdtLen - 1
	dd	LABEL_GDT

SelectorCode32	equ	LABEL_DESC_CODE32 - LABEL_GDT
SelectorData32	equ	LABEL_DESC_DATA32 - LABEL_GDT

[SECTION gdt64]

LABEL_GDT64:		dq	0x0000000000000000
LABEL_DESC_CODE64:	dq	0x0020980000000000
LABEL_DESC_DATA64:	dq	0x0000920000000000

GdtLen64	equ	$ - LABEL_GDT64
GdtPtr64	dw	GdtLen64 - 1
		dd	LABEL_GDT64

SelectorCode64	equ	LABEL_DESC_CODE64 - LABEL_GDT64
SelectorData64	equ	LABEL_DESC_DATA64 - LABEL_GDT64



;=======	声明，在16位宽模式下实现
[SECTION .s16]
[BITS 16]

Label_Start:

	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ax,	0x00
	mov	ss,	ax
	mov	sp,	0x7c00

;=======	在屏幕上显示 : Start Loader......

	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0200h		;row 2
	mov	cx,	12
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartLoaderMessage
	int	10h

;=======	打开 A20， 进入Big Real Mode
	push	ax
	in	al,	92h ; 从92h端口读取一字节数据到al
	or	al,	00000010b ; 或一下
	out	92h,	al ; 将al输出到92h端口
	pop	ax 
	; 我猜测这里在做的就是不管92h是什么，都用92h这个端口将20A功能开启

	cli ; 禁止所有中断

	db	0x66 ; 声明在16位情况下使用32位宽数据指令
	lgdt	[GdtPtr]	; lgdt将段描述子读取到对应区域

	;  启动保护模式， 通过更改cr0的最低位
	mov	eax,	cr0
	or	eax,	1
	mov	cr0,	eax

	mov	ax,	SelectorData32 ; 将不知道啥玩意读取到ax
	mov	fs,	ax ; 再存到fs?
	mov	eax,	cr0 ; 重新更改cr0以进入实模式
	and	al,	11111110b
	mov	cr0,	eax

	sti ; 开始接收中断

	jmp $

;=======	display messages

StartLoaderMessage:	db	"Start Loader"


