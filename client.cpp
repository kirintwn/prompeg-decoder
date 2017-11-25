#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "packetQueue.h"
#define RECVBUFLEN 1500

using namespace std;

packetBuffer *myPacketBuffer = new packetBuffer(2048);
monitor *myMonitor = new monitor();

void *threadproc(void *arg) {
    while(1) {
        sleep(2);
        myPacketBuffer -> bufferMonitor();
        myMonitor -> printMonitor();
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char *multicastIP = (char*)malloc(20 * sizeof(char));
    char *mediaPort = (char*)malloc(10 * sizeof(char));
    unsigned char *sockRecvBuf = (unsigned char*)malloc(RECVBUFLEN * sizeof(unsigned char));

    if(argc != 3) {
        strcpy(multicastIP , "239.0.0.1");
        strcpy(mediaPort , "20000");
    }
    else {
        multicastIP = argv[1];
        mediaPort = argv[2];
    }

    socketUtility *mySocketUtility = new socketUtility(multicastIP , mediaPort);

    fd_set master;
    fd_set read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(mySocketUtility -> media_Sockfd , &master);
    FD_SET(mySocketUtility -> fecRow_Sockfd , &master);
    FD_SET(mySocketUtility -> fecCol_Sockfd , &master);

    read_fds = master;

    pthread_t tid;
    pthread_create(&tid, NULL, &threadproc, NULL);

    for(;;) {
        myPacketBuffer -> updateFecQueue();
        myPacketBuffer -> fecRecovery(4);
        myPacketBuffer -> mediaSender(mySocketUtility -> output_Sockfd , 20000);
        myPacketBuffer -> updateMediaQueue(20000);
        myPacketBuffer -> updateMinSN();

        myMonitor -> recovered = myPacketBuffer -> recovered;

        read_fds = master;
        struct timeval tv = {0 , 50};
        select( (mySocketUtility -> fdmax)+1 , &read_fds , NULL , NULL , &tv);

        if(FD_ISSET(mySocketUtility -> media_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(mySocketUtility -> media_Sockfd , sockRecvBuf , RECVBUFLEN , 0 , NULL , 0)) < 0)
                mySocketUtility -> ~socketUtility();

            myMonitor -> media -> updateStatus(sockRecvBuf);
            myPacketBuffer -> newMediaPacket(sockRecvBuf , bytes);
        }

        if(FD_ISSET(mySocketUtility -> fecRow_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(mySocketUtility -> fecRow_Sockfd , sockRecvBuf , RECVBUFLEN , 0 , NULL , 0)) < 0)
                mySocketUtility -> ~socketUtility();

            myMonitor -> fecRow -> updateStatus(sockRecvBuf);
            myPacketBuffer -> newFecPacket(sockRecvBuf , bytes);
        }

        if(FD_ISSET(mySocketUtility -> fecCol_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(mySocketUtility -> fecCol_Sockfd , sockRecvBuf , RECVBUFLEN , 0 , NULL , 0)) < 0)
                mySocketUtility -> ~socketUtility();

            myMonitor -> fecCol -> updateStatus(sockRecvBuf);
            myPacketBuffer -> newFecPacket(sockRecvBuf , bytes);
        }
    }

    return 0; //never reached
}
