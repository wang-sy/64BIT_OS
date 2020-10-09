BOOT_DIR=./boot
BUILD_DIR=./build

boot.bin: 
# 生成boot的bin文件
	nasm $(BOOT_DIR)/boot.asm -o $(BOOT_DIR)/boot.bin 

install: 
# 将boot的bin写入到引导扇区内 
	dd if=$(BOOT_DIR)/boot.bin of=$(BUILD_DIR)/boot.img bs=512 count=1 conv=notrunc

clean: 
# 清空生成的文件的方法, 不清空本来就有的img文件
	rm -rf $(BOOT_DIR)/boot.bin 
