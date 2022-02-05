msh379: msh379.o linked_list.o
	gcc -o msh379 msh379.o linked_list.o

msh379.o: msh379.c msh379.h
	gcc -c msh379.c
linked_list.o: linked_list.c msh379.h
	gcc -c linked_list.c
tar:
	tar -cvf submit.tar msh379.c msh379.h linked_list.c Makefile README.pdf
clean:
	rm -f msh379
	rm -f msh379.o
	rm -f linked_list.o
