BOOT_SRC_DIR=./src/boot
KERNEL_SRC_DIR=./src/kernel
SUBDIRS=$(BOOT_SRC_DIR) $(KERNEL_SRC_DIR)
# 定义换行符，下面要用
define \n


endef

define \@
@
endef
BOOT_BUILD_DIR=$(BOOT_SRC_DIR)/build
KERNEL_BUILD_DIR=$(KERNEL_SRC_DIR)/build
PROJECT_BUILD_DIR=./build

all: clean
# 遍历每一个文件夹，进行编译
	@$(foreach dir, $(SUBDIRS), ${\@}cd $(dir) && $(MAKE) ${\n})


install: all
# 将boot的bin写入到引导扇区内 

	@echo 特别声明：不要删除boot.img，如果删除了， 请到64位操作系统书中36页寻找复原方法
	@dd if=$(BOOT_BUILD_DIR)/boot.bin of=$(PROJECT_BUILD_DIR)/boot.img bs=512 count=1 conv=notrunc
	@echo wangsy1990085 | sudo -S mount $(PROJECT_BUILD_DIR)/boot.img /home/wangsy/mount/ -t vfat -o loop
	@sudo cp $(BOOT_BUILD_DIR)/loader.bin /home/wangsy/mount
	@sudo cp $(KERNEL_BUILD_DIR)/kernel.bin /home/wangsy/mount
	@sync
	@sudo umount /home/wangsy/mount/
	@echo 挂载完成，请进入build文件夹后输入"bochs"以启动虚拟机

install_physical: all

	echo wangsy1990085 | sudo -S cp $(BOOT_BUILD_DIR)/loader.bin /media/wangsy/94AE-F0E9/
	sudo cp $(KERNEL_BUILD_DIR)/kernel.bin /media/wangsy/94AE-F0E9/
	sudo dd if=$(BOOT_BUILD_DIR)/boot.bin of=/dev/sdc bs=512 count=1 conv=notrunc
	echo 挂载完成，请将U盘插入并启动

clean:
# 遍历每一个文件夹，进行删除
	@$(foreach dir, $(SUBDIRS), ${\@}cd $(dir) && $(MAKE) clean ${\n})
