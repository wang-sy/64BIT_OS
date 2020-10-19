all: head.o

head.o: head.S
	gcc -E head.S > ./build/head.s
	as --64 -o ./build/head.o ./build/head.s

clean: 
	rm -rf ./build/*