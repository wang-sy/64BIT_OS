all: system
# 生成kernel.bin
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary ./build/system ./build/kernel.bin

system: head.o main.o
	ld -b elf64-x86-64 -o ./build/system ./build/head.o ./build/main.o -T Kernel.lds

head.o: head.S
	gcc -E head.S > ./build/head.s
	as --64 -o ./build/head.o ./build/head.s

main.o: main.c
# 编译main.c内核主程序(只编译不链接)
	gcc -mcmodel=large -fno-builtin -m64 -c main.c -o ./build/main.o

clean: 
	rm -rf ./build/*