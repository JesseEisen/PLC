#CC=gcc
CC=arm-linux-gcc

all:
	$(CC) -g main.c fx-serial.c  -Ipriqueue -o Serial -lpthread

clean:
	rm -rf Serial a.out
