CC=gcc
#CC=arm-linux-gcc

all:
	$(CC) -g main.c fx-serial.c  socket_client.c ./proto/message_header.pb-c.c ./proto/getdata.pb-c.c -Ipriqueue -Iproto -o Serial -lpthread

clean:
	rm -rf Serial a.out
