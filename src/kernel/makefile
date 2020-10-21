all: system
# 生成kernel.bin
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary ./build/system ./build/kernel.bin

system: head.o main.o position.o Kernel.lds
# 
	ld -b elf64-x86-64 ./build/head.o ./build/main.o position.o -T Kernel.lds -o ./build/system

head.o: head.S
# 生成kernel head binary file
	gcc -E head.S > ./build/head.s
	as --64 -o ./build/head.o ./build/head.s

main.o: main.c position.h position.o
# 编译main.c内核主程序(只编译不链接)
	gcc -mcmodel=large -fno-builtin -m64 -c main.c -o ./build/main.o

position.o: position.c position.h
	gcc  -mcmodel=large -fno-builtin -m64 -c position.c


clean: 
	rm -rf ./build/*