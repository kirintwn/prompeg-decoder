all: client.c packetParser.h socketConnection.h
	gcc client.c -o client
clean:
	rm -f client
