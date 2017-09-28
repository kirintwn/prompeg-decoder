/*
 * Usage:
 *     client <Multicast IP> <Multicast Port>
 * Examples:
 *     >client 233.0.41.102 20000
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "socketConnection.h"
#include "packetParser.h"

#define MULTICAST_SO_RCVBUF 500000
#define STREAM_OUTPUT_IP "127.0.0.1"
#define STREAM_OUTPUT_PORT "8000"

int media_Sockfd;
int fecRow_Sockfd;
int fecCol_Sockfd;

int output_Sockfd;

unsigned char* media_RecvBuf;
unsigned char* fecRow_RecvBuf;
unsigned char* fecCol_RecvBuf;

static void fuckUpSituation(const char* errorMsg) {
    fprintf(stderr , "%s\n" , errorMsg);
    if(media_Sockfd >= 0)
        close(media_Sockfd);
    if(fecRow_Sockfd >= 0)
        close(fecRow_Sockfd);
    if(fecCol_Sockfd >= 0)
        close(fecCol_Sockfd);
    if(output_Sockfd >= 0)
        close(output_Sockfd);
    if(media_RecvBuf)
        free(media_RecvBuf);
    if(fecRow_RecvBuf)
        free(fecRow_RecvBuf);
    if(fecCol_RecvBuf)
        free(fecCol_RecvBuf);

    exit(1);
}

int maximumOfThreeNum( int a, int b, int c ) {
   int max = ( a < b ) ? b : a;
   return ( ( max < c ) ? c : max );
}

int main(int argc, char* argv[]) {
    char* multicastIP;
    char* mediaPort;
    char* fecRowPort = (char*)malloc(20);
    char* fecColPort = (char*)malloc(20);

    int recvBufLen;

    if(argc != 3) {
        fprintf(stderr,"Usage: %s <Multicast IP> <Multicast Port>\n", argv[0]);
        exit(1);
    }

    multicastIP = argv[1];

    mediaPort = argv[2];
    sprintf(fecRowPort , "%d" , atoi(argv[2]) + 4);
    sprintf(fecColPort , "%d" , atoi(argv[2]) + 2);

    recvBufLen = 1500;

    media_RecvBuf = (unsigned char*)malloc(recvBufLen*sizeof(unsigned char));
    fecRow_RecvBuf = (unsigned char*)malloc(recvBufLen*sizeof(unsigned char));
    fecCol_RecvBuf = (unsigned char*)malloc(recvBufLen*sizeof(unsigned char));

    media_Sockfd = mCastClientConnectSocket(multicastIP , mediaPort , MULTICAST_SO_RCVBUF);
    if(media_Sockfd < 0)
        fuckUpSituation("mCastClientConnectSocket failed");

    fecRow_Sockfd = mCastClientConnectSocket(multicastIP , fecRowPort , MULTICAST_SO_RCVBUF);
    if(fecRow_Sockfd < 0)
        fuckUpSituation("mCastClientConnectSocket failed");

    fecCol_Sockfd = mCastClientConnectSocket(multicastIP , fecColPort , MULTICAST_SO_RCVBUF);
    if(fecCol_Sockfd < 0)
        fuckUpSituation("mCastClientConnectSocket failed");

    output_Sockfd = uCastConnectSocket(STREAM_OUTPUT_IP , STREAM_OUTPUT_PORT);
    if(output_Sockfd < 0)
        fuckUpSituation("uCastConnectSocket failed");

    fd_set master;
    fd_set read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(media_Sockfd , &master);
    FD_SET(fecRow_Sockfd , &master);
    FD_SET(fecCol_Sockfd , &master);
    int fdmax = maximumOfThreeNum(media_Sockfd , fecRow_Sockfd , fecCol_Sockfd);

    read_fds = master;

    for(;;) {
        read_fds = master;
        select(fdmax+1 , &read_fds , NULL , NULL , NULL);

        if(FD_ISSET(media_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(media_Sockfd , media_RecvBuf , recvBufLen , 0 , NULL , 0)) < 0)
                fuckUpSituation("recvfrom() failed");
            /*rtp_parse(media_RecvBuf , bytes);*/

            if (send(output_Sockfd , media_RecvBuf , bytes , 0) == -1)
                /*perror("send");*/bytes = bytes;
        }

        if(FD_ISSET(fecRow_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(fecRow_Sockfd , fecRow_RecvBuf , recvBufLen , 0 , NULL , 0)) < 0)
                fuckUpSituation("recvfrom() failed");
            /*rtpParser(fecRow_RecvBuf , bytes);*/
        }

        if(FD_ISSET(fecCol_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recvfrom(fecCol_Sockfd , fecCol_RecvBuf , recvBufLen , 0 , NULL , 0)) < 0)
                fuckUpSituation("recvfrom() failed");
            /*rtpParser(fecCol_RecvBuf , bytes);*/
        }
    }

    return 0; /*never reached*/
}
