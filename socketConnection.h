#ifndef INCLUDED_SOCKET_H
#define INCLUDED_SOCKET_H

#include <iostream>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SO_RCVBUF_SIZE 10485760
#define RECVBUFLEN 1500

using namespace std;

class socketUtility {
    public:
        //////////////////////////////////
        int media_Sockfd;
        int fecRow_Sockfd;
        int fecCol_Sockfd;
        int output_Sockfd;
        int fdmax;
        unsigned char *sockRecvBuf;
        //////////////////////////////////
        socketUtility(const char* mediaIP , const char* mediaPort, const char* outputIP , const char* outputPort) {
            char *fecRowPort = (char*)malloc(20);
            char *fecColPort = (char*)malloc(20);
            sprintf(fecRowPort , "%d" , atoi(mediaPort) + 4);
            sprintf(fecColPort , "%d" , atoi(mediaPort) + 2);

            media_Sockfd = listenSocket(mediaIP , mediaPort , SO_RCVBUF_SIZE);
            fecRow_Sockfd = listenSocket(mediaIP , fecRowPort , SO_RCVBUF_SIZE);
            fecCol_Sockfd = listenSocket(mediaIP , fecColPort , SO_RCVBUF_SIZE);
            output_Sockfd = uCastConnectSocket(outputIP , outputPort);

            fdmax = maximumOfThreeNum(media_Sockfd , fecRow_Sockfd , fecCol_Sockfd);

            sockRecvBuf = (unsigned char*)malloc(RECVBUFLEN * sizeof(unsigned char));


        }
        ~socketUtility() {
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

        bool isMulticastAddress(const char* mediaIP) {
            string IPstr(mediaIP);
            string firstByteStr = IPstr.substr(0 , 3);
            if(stoi(firstByteStr) >= 224 && stoi(firstByteStr) <= 239) {
                return true;
            }
            else {
                return false;
            }
        }

        int listenSocket(const char* mediaIP , const char* mediaPort , int recvBufSize) {
            int sockfd;
            struct addrinfo hints = { 0 };
            struct addrinfo* localAddr = 0;
            struct addrinfo* mediaAddr = 0;
            int yes = 1;

            hints.ai_family = AF_INET;
            hints.ai_flags = AI_NUMERICHOST;

            int addrInfoStatus;
            if((addrInfoStatus = getaddrinfo(mediaIP , NULL , &hints , &mediaAddr)) != 0) {
                if(localAddr)
                    freeaddrinfo(localAddr);
                if(mediaAddr)
                    freeaddrinfo(mediaAddr);
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
                return -1;
            }

            hints.ai_family = mediaAddr->ai_family;
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_flags = AI_PASSIVE;

            if((addrInfoStatus = getaddrinfo(NULL , mediaPort , &hints , &localAddr)) != 0) {
                if(localAddr)
                    freeaddrinfo(localAddr);
                if(mediaAddr)
                    freeaddrinfo(mediaAddr);
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
                return -1;
            }

            if((sockfd = socket(localAddr->ai_family , localAddr->ai_socktype , 0)) < 0) {
                perror("socket() failed");
                if(localAddr)
                    freeaddrinfo(localAddr);
                if(mediaAddr)
                    freeaddrinfo(mediaAddr);
                return -1;
            }

            if(setsockopt(sockfd , SOL_SOCKET , SO_REUSEADDR , (char*)&yes , sizeof(int)) == -1) {
                perror("setsockopt() failed");
                if(localAddr)
                    freeaddrinfo(localAddr);
                if(mediaAddr)
                    freeaddrinfo(mediaAddr);
                return -1;
            }

            if(bind(sockfd , localAddr->ai_addr , localAddr->ai_addrlen) != 0) {
                perror("bind() failed");
                if(localAddr)
                    freeaddrinfo(localAddr);
                if(mediaAddr)
                    freeaddrinfo(mediaAddr);
                return -1;
            }

            int outputValue = 0;
            socklen_t outputValue_len = sizeof(outputValue);
            int defaultBuf;

            if(getsockopt(sockfd , SOL_SOCKET , SO_RCVBUF , (char*)&outputValue , &outputValue_len) != 0) {
                perror("getsockopt failed");
                if(localAddr)
                    freeaddrinfo(localAddr);
                if(mediaAddr)
                    freeaddrinfo(mediaAddr);
                return -1;
            }
            defaultBuf = outputValue;

            outputValue = recvBufSize;
            if(setsockopt(sockfd , SOL_SOCKET , SO_RCVBUF , (char*)&outputValue , sizeof(outputValue)) != 0) {
                perror("setsockopt failed");
                if(localAddr)
                    freeaddrinfo(localAddr);
                if(mediaAddr)
                    freeaddrinfo(mediaAddr);
                return -1;
            }
            if(getsockopt(sockfd , SOL_SOCKET , SO_RCVBUF , (char*)&outputValue , &outputValue_len) != 0) {
                perror("getsockopt failed");
                if(localAddr)
                    freeaddrinfo(localAddr);
                if(mediaAddr)
                    freeaddrinfo(mediaAddr);
                return -1;
            }

            printf("tried to set socket receive buffer from %d to %d, got %d\n", defaultBuf , recvBufSize , outputValue);

            if(isMulticastAddress(mediaIP)) {
                struct ip_mreq multicastRequest;
                memcpy(&multicastRequest.imr_multiaddr , &((struct sockaddr_in*)(mediaAddr->ai_addr))->sin_addr , sizeof(multicastRequest.imr_multiaddr));
                multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

                if(setsockopt(sockfd, IPPROTO_IP , IP_ADD_MEMBERSHIP , (char*) &multicastRequest , sizeof(multicastRequest)) != 0) {
                    perror("setsockopt failed");
                    if(localAddr)
                        freeaddrinfo(localAddr);
                    if(mediaAddr)
                        freeaddrinfo(mediaAddr);
                    return -1;
                }
            }

            if(localAddr)
                freeaddrinfo(localAddr);
            if(mediaAddr)
                freeaddrinfo(mediaAddr);
            return sockfd;
        }
        int uCastConnectSocket(const char* unicastIP , const char* unicastPort) {
            int sockfd;
            struct addrinfo hints = { 0 };
            struct addrinfo* res = 0;
            int yes = 1;

            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;

            int addrInfoStatus;
            if((addrInfoStatus = getaddrinfo(unicastIP , unicastPort , &hints , &res)) != 0) {
                if(res)
                    freeaddrinfo(res);
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
                return -1;
            }

            if ((sockfd = socket(res->ai_family , res->ai_socktype , 0)) < 0) {
                perror("socket() failed");
                if(res)
                    freeaddrinfo(res);
                return -1;
            }

            if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
                perror("connect() failed");
                if(res)
                    freeaddrinfo(res);
                return -1;
            }

            if(setsockopt(sockfd , SOL_SOCKET , SO_REUSEADDR , (char*)&yes , sizeof(int)) == -1) {
                perror("setsockopt() failed");
                if(res)
                    freeaddrinfo(res);
                return -1;
            }

            if(res)
                freeaddrinfo(res);
            return sockfd;
        }
        int maximumOfThreeNum( int a , int b , int c ) {
           int max = ( a < b ) ? b : a;
           return ( ( max < c ) ? c : max );
        }
};

#endif
