/*
  *Usage:
  *    client <Multicast IP> <Multicast Port>
  *Examples:
  *    >client 233.0.41.102 20000
 */
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

using namespace std;

#include "packetQueue.h"
#define RECVBUFLEN 1500

packetBuffer *myPacketBuffer = new packetBuffer(2048);

int main(int argc, char *argv[]) {
    char *multicastIP;
    char *mediaPort;
    unsigned char *sockRecvBuf;

    if(argc != 3) {
        fprintf(stderr,"Usage: %s <Multicast IP> <Multicast Port>\n", argv[0]);
        exit(1);
    }

    multicastIP = argv[1];
    mediaPort = argv[2];

    socketUtility *mySocketUtility = new socketUtility(multicastIP , mediaPort);

    fd_set master;
    fd_set read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(mySocketUtility -> media_Sockfd , &master);
    FD_SET(mySocketUtility -> fecRow_Sockfd , &master);
    FD_SET(mySocketUtility -> fecCol_Sockfd , &master);

    read_fds = master;

    sockRecvBuf = (unsigned char*)malloc(RECVBUFLEN * sizeof(unsigned char));

    for(;;) {
        myPacketBuffer -> mediaSender(mySocketUtility -> output_Sockfd , 10000);
        myPacketBuffer -> updateMediaQueue(10000);
        myPacketBuffer -> updateMinSN();

        read_fds = master;
        struct timeval tv = {0 , 50};
        select( (mySocketUtility -> fdmax)+1 , &read_fds , NULL , NULL , &tv);

        if(FD_ISSET(mySocketUtility -> media_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(mySocketUtility -> media_Sockfd , sockRecvBuf , RECVBUFLEN , 0 , NULL , 0)) < 0)
                mySocketUtility -> ~socketUtility();

            myPacketBuffer -> newMediaPacket(sockRecvBuf , bytes);
            myPacketBuffer -> bufferMonitor();
        }

        if(FD_ISSET(mySocketUtility -> fecRow_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(mySocketUtility -> fecRow_Sockfd , sockRecvBuf , RECVBUFLEN , 0 , NULL , 0)) < 0)
                mySocketUtility -> ~socketUtility();

            myPacketBuffer -> newFecPacket(sockRecvBuf , bytes);
            myPacketBuffer -> updateFecQueue();
            myPacketBuffer -> bufferMonitor();
        }

        if(FD_ISSET(mySocketUtility -> fecCol_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(mySocketUtility -> fecCol_Sockfd , sockRecvBuf , RECVBUFLEN , 0 , NULL , 0)) < 0)
                mySocketUtility -> ~socketUtility();

            myPacketBuffer -> newFecPacket(sockRecvBuf , bytes);
            myPacketBuffer -> updateFecQueue();
            myPacketBuffer -> bufferMonitor();
        }
    }

    return 0; //never reached
}
