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
    mov cx, 0x20
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


StartBootMessage:	db	"01234567891011121314151617181920"

;=======	fill zero until whole sector

	times	510 - ($ - $$)	db	0
	dw	0xaa55
