RootDirSectors	equ	14 ; 根目录区占用的扇区数
SectorNumOfRootDirStart	equ	19 ; 根目录区起始扇区号
SectorNumOfFAT1Start	equ	1 ; 起始扇区号
SectorBalance	equ	17	

    BS_OEMName          db  'WSYboot'  ; 分区的名字
    BPB_BytesPerSec     dw  0x200         ; 每个扇区的字节数
    BPB_SecPerClus      db  0x8           ; 每个簇的扇区数
    BPB_RsvdSecCnt	    dw	1           ; 保留扇区数
	BPB_NumFATs	        db	2           ; FAT表的数量
	BPB_RootEntCnt	    dw	0xe0         ; 根目录可容纳的目录项数
	BPB_TotSec16	    dw	0x7d82        ; 总扇区数
	BPB_Media	        db	0xf0        ; 介质描述符
	BPB_FATSz16	        dw	0xc           ; 每FAT扇区数
	BPB_SecPerTrk	    dw	0x3f          ; 每磁道扇区数
	BPB_NumHeads	    dw	0xff           ; 磁头数
	BPB_HiddSec	        dd	0           ; 隐藏扇区数
	BPB_TotSec32	    dd	0           
	BS_DrvNum	        db	0           ; int 13h 的驱动器号
	BS_Reserved1	    db	0           ; 未使用
	BS_BootSig	        db	0x29        ; 扩展引导标记
	BS_VolID	        dd	0           ; 卷序列号
	BS_VolLab	        db	'boot loader' ; 卷标
	BS_FileSysType	    db	'FAT12   '  ; 文件系统类型
