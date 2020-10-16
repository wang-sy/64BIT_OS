
all: boot.bin loader.bin kernel.bin

boot.bin: 
# 生成boot的bin文件
	nasm boot.asm -o ./build/boot.bin 

loader.bin: fat12.inc
# 生成loader的bin文件
	nasm loader.asm -o ./build/loader.bin

kernel.bin: loader.bin
	cp ./build/loader.bin ./build/kernel.bin

clean:
# 清除
	rm -rf ./build/*