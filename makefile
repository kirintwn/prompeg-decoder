all: client.cpp packetProcessor.h socketConnection.h packetQueue.h monitor.h
	g++ -pthread client.cpp -o client
clean:
	rm -f client
