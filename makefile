all: client.c packetProcessor.h socketConnection.h
	gcc-6 client.c -o client
clean:
	rm -f client
