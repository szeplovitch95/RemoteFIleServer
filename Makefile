all:
	gcc netfileserver.c libnetfiles.c -o server -lpthread
netfileserver.o:
	gcc -o server netfileserver.c -lpthread
libnetfiles.o:
	gcc libnetfiles.c
clean:
	rm server
