all: client.cpp packetProcessor.h socketConnection.h packetQueue.h
	g++ client.cpp -o client
clean:
	rm -f client
