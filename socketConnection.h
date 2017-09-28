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

int mCastClientConnectSocket(char* multicastIP , char* multicastPort , int multicastRecvBufSize) {
    int sockfd;
    struct addrinfo hints = { 0 };
    struct addrinfo* localAddr = 0;
    struct addrinfo* multicastAddr = 0;
    int yes = 1;

    hints.ai_family = AF_INET;
    hints.ai_flags = AI_NUMERICHOST;

    int addrInfoStatus;
    if((addrInfoStatus = getaddrinfo(multicastIP , NULL , &hints , &multicastAddr)) != 0) {
        if(localAddr)
            freeaddrinfo(localAddr);
        if(multicastAddr)
            freeaddrinfo(multicastAddr);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
        return -1;
    }

    hints.ai_family = multicastAddr->ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if((addrInfoStatus = getaddrinfo(NULL , multicastPort , &hints , &localAddr)) != 0) {
        if(localAddr)
            freeaddrinfo(localAddr);
        if(multicastAddr)
            freeaddrinfo(multicastAddr);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
        return -1;
    }

    if((sockfd = socket(localAddr->ai_family , localAddr->ai_socktype , 0)) < 0) {
        perror("socket() failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(multicastAddr)
            freeaddrinfo(multicastAddr);
        return -1;
    }

    if(setsockopt(sockfd , SOL_SOCKET , SO_REUSEADDR , (char*)&yes , sizeof(int)) == -1) {
        perror("setsockopt() failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(multicastAddr)
            freeaddrinfo(multicastAddr);
        return -1;
    }

    if(bind(sockfd , localAddr->ai_addr , localAddr->ai_addrlen) != 0) {
        perror("bind() failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(multicastAddr)
            freeaddrinfo(multicastAddr);
        return -1;
    }

    int outputValue = 0;
    socklen_t outputValue_len = sizeof(outputValue);
    int defaultBuf;

    if(getsockopt(sockfd , SOL_SOCKET , SO_RCVBUF , (char*)&outputValue , &outputValue_len) != 0) {
        perror("getsockopt failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(multicastAddr)
            freeaddrinfo(multicastAddr);
        return -1;
    }
    defaultBuf = outputValue;

    outputValue = multicastRecvBufSize;
    if(setsockopt(sockfd , SOL_SOCKET , SO_RCVBUF , (char*)&outputValue , sizeof(outputValue)) != 0) {
        perror("setsockopt failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(multicastAddr)
            freeaddrinfo(multicastAddr);
        return -1;
    }
    if(getsockopt(sockfd , SOL_SOCKET , SO_RCVBUF , (char*)&outputValue , &outputValue_len) != 0) {
        perror("getsockopt failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(multicastAddr)
            freeaddrinfo(multicastAddr);
        return -1;
    }

    printf("tried to set socket receive buffer from %d to %d, got %d\n", defaultBuf , multicastRecvBufSize , outputValue);

    struct ip_mreq multicastRequest;
    memcpy(&multicastRequest.imr_multiaddr , &((struct sockaddr_in*)(multicastAddr->ai_addr))->sin_addr , sizeof(multicastRequest.imr_multiaddr));

    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

    if(setsockopt(sockfd, IPPROTO_IP , IP_ADD_MEMBERSHIP , (char*) &multicastRequest , sizeof(multicastRequest)) != 0) {
        perror("setsockopt failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(multicastAddr)
            freeaddrinfo(multicastAddr);
        return -1;
    }

    if(localAddr)
        freeaddrinfo(localAddr);
    if(multicastAddr)
        freeaddrinfo(multicastAddr);
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
