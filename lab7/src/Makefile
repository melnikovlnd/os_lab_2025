all: tcpclient tcpserver udpclient udpserver

tcpclient: tcpclient.c
	gcc -o tcpclient tcpclient.c

tcpserver: tcpserver.c
	gcc -o tcpserver tcpserver.c

udpclient: udpclient.c
	gcc -o udpclient udpclient.c

udpserver: udpserver.c
	gcc -o udpserver udpserver.c

clean:
	rm -f tcpclient tcpserver udpclient udpserver

.PHONY: all clean