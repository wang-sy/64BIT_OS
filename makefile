BOOT_DIR=./src/boot
BUILD_DIR=./build
BUILD_BIN_DIR=./build/bin

all: boot.bin loader.bin
	echo 执行完成

boot.bin: 
# 生成boot的bin文件
	nasm $(BOOT_DIR)/boot.asm -o $(BUILD_BIN_DIR)/boot.bin 

loader.bin:
# 生成loader的bin文件
	nasm $(BOOT_DIR)/loader.asm -o $(BUILD_BIN_DIR)/loader.bin 


install: 
# 将boot的bin写入到引导扇区内 

	echo "特别声明：不要删除boot.img，如果删除了， 请到64位操作系统书中36页寻找复原方法"
	dd if=$(BUILD_BIN_DIR)/boot.bin of=$(BUILD_DIR)/boot.img bs=512 count=1 conv=notrunc 
	sudo mount $(BUILD_DIR)/boot.img /media/ -t vfat -o loop
	sudo cp $(BUILD_BIN_DIR)/loader.bin /media
	sync
	sudo umount /media/
	echo 挂载完成，请进入build文件夹后输入"bochs -f ./bochsrc"以启动虚拟机

clean: 
# 清空生成的文件的方法, 不清空本来就有的img文件
	rm -rf $(BUILD_BIN_DIR)/boot.bin $(BUILD_BIN_DIR)/loader.bin

