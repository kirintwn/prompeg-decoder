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

#include "socketConnection.h"
#include "packetProcessor.h"
#include "packetQueue.h"

#define MULTICAST_SO_RCVBUF 1048576
#define STREAM_OUTPUT_IP "127.0.0.1"
#define STREAM_OUTPUT_PORT "8000"

packetBuffer *myPacketBuffer = new packetBuffer(2048);

int media_Sockfd;
int fecRow_Sockfd;
int fecCol_Sockfd;
int output_Sockfd;
unsigned char *sockRecvBuf;

static void fuckUpSituation(const char *errorMsg) {
    fprintf(stderr , "%s\n" , errorMsg);
    if(media_Sockfd >= 0)
        close(media_Sockfd);
    if(fecRow_Sockfd >= 0)
        close(fecRow_Sockfd);
    if(fecCol_Sockfd >= 0)
        close(fecCol_Sockfd);
    if(output_Sockfd >= 0)
        close(output_Sockfd);
    if(sockRecvBuf)
        free(sockRecvBuf);
    exit(1);
}

int maximumOfThreeNum( int a, int b, int c ) {
   int max = ( a < b ) ? b : a;
   return ( ( max < c ) ? c : max );
}

int main(int argc, char *argv[]) {
    char *multicastIP;
    char *mediaPort;
    char *fecRowPort = (char*)malloc(20);
    char *fecColPort = (char*)malloc(20);
    int recvBufLen = 1500;

    if(argc != 3) {
        fprintf(stderr,"Usage: %s <Multicast IP> <Multicast Port>\n", argv[0]);
        exit(1);
    }

    multicastIP = argv[1];
    mediaPort = argv[2];
    sprintf(fecRowPort , "%d" , atoi(argv[2]) + 4);
    sprintf(fecColPort , "%d" , atoi(argv[2]) + 2);

    media_Sockfd = mCastClientConnectSocket(multicastIP , mediaPort , MULTICAST_SO_RCVBUF);
    fecRow_Sockfd = mCastClientConnectSocket(multicastIP , fecRowPort , MULTICAST_SO_RCVBUF);
    fecCol_Sockfd = mCastClientConnectSocket(multicastIP , fecColPort , MULTICAST_SO_RCVBUF);
    output_Sockfd = uCastConnectSocket(STREAM_OUTPUT_IP , STREAM_OUTPUT_PORT);
    if((media_Sockfd < 0) || (fecRow_Sockfd < 0) || (fecCol_Sockfd < 0) || (output_Sockfd < 0))
        fuckUpSituation("ConnectSocket failed");


    fd_set master;
    fd_set read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(media_Sockfd , &master);
    FD_SET(fecRow_Sockfd , &master);
    FD_SET(fecCol_Sockfd , &master);
    int fdmax = maximumOfThreeNum(media_Sockfd , fecRow_Sockfd , fecCol_Sockfd);
    read_fds = master;

    sockRecvBuf = (unsigned char*)malloc(recvBufLen * sizeof(unsigned char));

    for(;;) {
        //todo: 把太遲的media packet送出去
        //      更新mediaQueue (base on currentTS)
        //      更新minSN

        read_fds = master;
        struct timeval tv = {0 , 50};
        select(fdmax+1 , &read_fds , NULL , NULL , &tv);

        if(FD_ISSET(media_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(media_Sockfd , sockRecvBuf , recvBufLen , 0 , NULL , 0)) < 0)
                fuckUpSituation("recvfrom() failed");

            myPacketBuffer -> newMediaPacket(sockRecvBuf , recvBufLen);
            myPacketBuffer -> bufferMonitor();
        }

        if(FD_ISSET(fecRow_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(fecRow_Sockfd , sockRecvBuf , recvBufLen , 0 , NULL , 0)) < 0)
                fuckUpSituation("recvfrom() failed");

            myPacketBuffer -> newFecPacket(sockRecvBuf , recvBufLen);
            myPacketBuffer -> updateFecQueue();
            myPacketBuffer -> bufferMonitor();
        }

        if(FD_ISSET(fecCol_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(fecCol_Sockfd , sockRecvBuf , recvBufLen , 0 , NULL , 0)) < 0)
                fuckUpSituation("recvfrom() failed");

            myPacketBuffer -> newFecPacket(sockRecvBuf , recvBufLen);
            myPacketBuffer -> updateFecQueue();
            myPacketBuffer -> bufferMonitor();
        }
    }

    return 0; /*never reached*/
}
