	org	0x7c00	; 程序加载的位置

BaseOfStack	equ	0x7c00 ; 记录基地址

BaseOfLoader	equ	0x1000 ; 记录Loader的基地址
OffsetOfLoader	equ	0x00 ; 记录Loader的偏移量

RootDirSectors	equ	14 ; 根目录区占用的扇区数
SectorNumOfRootDirStart	equ	19 ; 根目录区起始扇区号
SectorNumOfFAT1Start	equ	1 ; 起始扇区号
SectorBalance	equ	17	

    jmp short Label_Start ; 这里是BS_jmpBoot 
    ; 实现的是段内转移，如果转移范围超过128，那么就会出错
    nop
    BS_OEMName          db  'MINEboot'  ; 表示的是生产商的名字，我不要脸一点直接写自己名字了
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

; 将CS寄存器的段基址设置到DS、ES、SS中
Label_Start:

	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	BaseOfStack

; ======  清空屏幕

    mov ax, 0600h
    mov bx, 0700h
    mov cx, 0
    mov dx, 0184fh
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


;=======	搜索 loader.bin
	mov	word	[SectorNo],	SectorNumOfRootDirStart

Lable_Search_In_Root_Dir_Begin:

	cmp	word	[RootDirSizeForLoop],	0
	jz	Label_No_LoaderBin
	dec	word	[RootDirSizeForLoop]	
	mov	ax,	00h
	mov	es,	ax
	mov	bx,	8000h
	mov	ax,	[SectorNo]
	mov	cl,	1
	call	Func_ReadOneSector
	mov	si,	LoaderFileName
	mov	di,	8000h
	cld
	mov	dx,	10h
	
Label_Search_For_LoaderBin:

	cmp	dx,	0
	jz	Label_Goto_Next_Sector_In_Root_Dir
	dec	dx
	mov	cx,	11

Label_Cmp_FileName:

	cmp	cx,	0
	jz	Label_FileName_Found
	dec	cx
	lodsb	
	cmp	al,	byte	[es:di]
	jz	Label_Go_On
	jmp	Label_Different

Label_Go_On:
	
	inc	di
	jmp	Label_Cmp_FileName

Label_Different:

	and	di,	0ffe0h
	add	di,	20h
	mov	si,	LoaderFileName
	jmp	Label_Search_For_LoaderBin

Label_Goto_Next_Sector_In_Root_Dir:
	
	add	word	[SectorNo],	1
	jmp	Lable_Search_In_Root_Dir_Begin
	
;=======	在屏幕上显示 : ERROR:No LOADER Found

Label_No_LoaderBin:

	mov	ax,	1301h
	mov	bx,	008ch
	mov	dx,	0100h
	mov	cx,	21
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderMessage
	int	10h
	jmp	$

;=======	在根目录寻找 loader.bin

Label_FileName_Found:

	mov	ax,	RootDirSectors
	and	di,	0ffe0h
	add	di,	01ah
	mov	cx,	word	[es:di]
	push	cx
	add	cx,	ax
	add	cx,	SectorBalance
	mov	ax,	BaseOfLoader
	mov	es,	ax
	mov	bx,	OffsetOfLoader
	mov	ax,	cx

Label_Go_On_Loading_File:
	push	ax
	push	bx
	mov	ah,	0eh
	mov	al,	'.'
	mov	bl,	0fh
	int	10h
	pop	bx
	pop	ax

	mov	cl,	1
	call	Func_ReadOneSector
	pop	ax
	call	Func_GetFATEntry
	cmp	ax,	0fffh
	jz	Label_File_Loaded
	push	ax
	mov	dx,	RootDirSectors
	add	ax,	dx
	add	ax,	SectorBalance
	add	bx,	[BPB_BytesPerSec]
	jmp	Label_Go_On_Loading_File

Label_File_Loaded: ; 找到了就跳转
	
	jmp	BaseOfLoader:OffsetOfLoader

;=======	从磁盘中读入一个扇区

Func_ReadOneSector:
	
	push	bp
	mov	bp,	sp
	sub	esp,	2
	mov	byte	[bp - 2],	cl
	push	bx
	mov	bl,	[BPB_SecPerTrk]
	div	bl
	inc	ah
	mov	cl,	ah
	mov	dh,	al
	shr	al,	1
	mov	ch,	al
	and	dh,	1
	pop	bx
	mov	dl,	[BS_DrvNum]
Label_Go_On_Reading:
	mov	ah,	2
	mov	al,	byte	[bp - 2]
	int	13h
	jc	Label_Go_On_Reading
	add	esp,	2
	pop	bp
	ret

;=======	FAT表解析

Func_GetFATEntry:

	push	es
	push	bx
	push	ax
	mov	ax,	00
	mov	es,	ax
	pop	ax
	mov	byte	[Odd],	0
	mov	bx,	3
	mul	bx
	mov	bx,	2
	div	bx
	cmp	dx,	0
	jz	Label_Even
	mov	byte	[Odd],	1

Label_Even:

	xor	dx,	dx
	mov	bx,	[BPB_BytesPerSec]
	div	bx
	push	dx
	mov	bx,	8000h
	add	ax,	SectorNumOfFAT1Start
	mov	cl,	2
	call	Func_ReadOneSector
	
	pop	dx
	add	bx,	dx
	mov	ax,	[es:bx]
	cmp	byte	[Odd],	1
	jnz	Label_Even_2
	shr	ax,	4

Label_Even_2:
	and	ax,	0fffh
	pop	bx
	pop	es
	ret

;=======	局部变量

RootDirSizeForLoop	dw	RootDirSectors
SectorNo		dw	0
Odd			db	0

;=======	输出的信息

StartBootMessage:	db	"Start Boot"
NoLoaderMessage:	db	"ERROR:No LOADER Found"
LoaderFileName:		db	"LOADER  BIN",0

;=======	补0

	times	510 - ($ - $$)	db	0
	dw	0xaa55
