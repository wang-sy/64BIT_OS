C_FILE_LIST=main position printk font gate trap memory
C_FILE_BUILD_GOALS=$(foreach file, $(C_FILE_LIST), ./build/$(file).o)

ASM_FILE_LIST=head entry
ASM_FILE_BUILD_GOALS=$(foreach file, $(ASM_FILE_LIST), ./build/$(file).o)

define \n


endef

all: system
# 生成kernel.bin
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary ./build/system ./build/kernel.bin

system: Kernel.lds build_all_c_code build_all_asm_code
# 
	ld -b elf64-x86-64 $(ASM_FILE_BUILD_GOALS) $(C_FILE_BUILD_GOALS) -T Kernel.lds -o ./build/system

build_all_c_code:
	$(foreach file, $(C_FILE_LIST), gcc -mcmodel=large -fno-builtin -fno-stack-protector -m64 -c $(file).c -o ./build/$(file).o  ${\n})

build_all_asm_code:
	$(foreach file, $(ASM_FILE_LIST), gcc -E $(file).S > ./build/$(file).s  ${\n} as --64 -o ./build/$(file).o ./build/$(file).s ${\n})

clean: 
	rm -rf ./build/*