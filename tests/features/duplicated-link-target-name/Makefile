all:
	make -C a
	make -C b
	make -C c
	gcc -o a.out a/a.lib b/a.lib b/b.lib c/a.lib c/b.lib c/c.lib main.c

clean:
	rm -rf a.out *.o **/*.o **/*.lib