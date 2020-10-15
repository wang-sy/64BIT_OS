BOOT_SRC_DIR=./src/boot
SUBDIRS=$(BOOT_SRC_DIR)

BOOT_BUILD_DIR=$(BOOT_SRC_DIR)/build
PROJECT_BUILD_DIR=./build

all: 
	cd $(BOOT_SRC_DIR) && $(MAKE)

install: 
# 将boot的bin写入到引导扇区内 

	echo "特别声明：不要删除boot.img，如果删除了， 请到64位操作系统书中36页寻找复原方法"
	dd if=$(BOOT_BUILD_DIR)/boot.bin of=$(PROJECT_BUILD_DIR)/boot.img bs=512 count=1 conv=notrunc 
	sudo mount $(PROJECT_BUILD_DIR)/boot.img /media/ -t vfat -o loop
	sudo cp $(BOOT_BUILD_DIR)/loader.bin /media
	sync
	sudo umount /media/
	echo 挂载完成，请进入build文件夹后输入"bochs -f ./bochsrc"以启动虚拟机

clean:
	cd $(SUBDIRS) && $(MAKE) clean
