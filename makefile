all: client.cpp packetProcessor.h socketConnection.h packetQueue.h monitor.h
	g++ -pthread -std=c++11 client.cpp -o client
clean:
	rm -f client
