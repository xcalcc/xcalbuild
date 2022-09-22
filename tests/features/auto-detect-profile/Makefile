target_gnu:
	gcc -x c a.c
target_clang:
	./clang -x c a.c
target_arm:
	./arm-none-eabi-gcc -x c a.c
target_partial:
	ar r a.lib a.c
	./clang -x c a.c
	gcc -x c a.c
