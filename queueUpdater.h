#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include "packetQueue.h"

int dontSend(queueNode_ *sendIndex , int sendIndexAtTail , queue_ *mediaQueue) {
    if(isEmpty(mediaQueue)) {
        return 1;
    }
    else if( (sendIndexAtTail == 1) && (sendIndex -> next == NULL) ) {
        return 1;
    }
    else {
        return 0;
    }
}

queueNode_ *sendIndex_Locator(queueNode_ *sendIndex , int sendIndexAtTail , queue_ *mediaQueue) {
    if(isEmpty(mediaQueue)) {
        sendIndex = NULL;
    }
    else if(sendIndex == NULL && mediaQueue -> head != NULL) {
        sendIndex = mediaQueue -> head;
    }
    else if( (sendIndexAtTail == 1) && (sendIndex -> next != NULL) ) {
        sendIndex = sendIndex -> next;
    }
    else if( (sendIndexAtTail == 1) && (sendIndex -> next == NULL) ) {
        sendIndex = sendIndex;
    }
    else {
        sendIndex = sendIndex;
    }

    return sendIndex;
}

int isTooLate(uint32_t targetTS , uint32_t currentTS , unsigned maxTime) {
    if(currentTS == 0) {
        printf("currentTS fucked up\n");
        exit(1);
    }
    else {
        uint16_t currentTS_Second = currentTS >> 16;
        uint16_t targetTS_Second = targetTS >> 16;
        if( (targetTS_Second + maxTime) < currentTS_Second ) {
            return 1;
        }
        else {
            return 0;
        }
    }
}

uint32_t getNodeRTPts(queueNode_ *queueNode) {
    if ((queueNode == NULL) || (queueNode -> data.used == 0) ) {
        exit(1);
    }
    else {
        rtpPacket_ *rtpPacket = (rtpPacket_*) queueNode -> data.packetData;
        return rtpPacket -> rtpHeader.ts;
    }
}

int nextPacketTooLate(queueNode_ *sendIndex , uint32_t currentTS , unsigned maxTime) {
    /*
    not too late -> return 0;
    too late -> return quantity of consecutive reserved node
    */
    int counter = 1;
    queueNode_ *tempIndex = sendIndex;

    while(tempIndex -> next != NULL) {
        tempIndex = tempIndex -> next;
        if(tempIndex -> data.used > 0) {
            if( isTooLate(getNodeRTPts(tempIndex) , currentTS , maxTime) ) {
                return counter;
            }
            else {
                return 0;
            }
        }
        else {
            counter++;
        }
    }

    printf("mediaQueue management fucked up!(1)\n");
    exit(1);
}

int sendOneMediaPacket(int output_Sockfd , const void *buffer , size_t length) {
    rtpPacket_ *rtpPacket = (rtpPacket_*) buffer;

    printf("Sending Packet SN: %d\n", rtpPacket -> rtpHeader.sequenceNum);
    if (send(output_Sockfd , rtpPacket , length , 0) == -1)
        perror("send");

    return 0;
}

uint16_t getMinSN(queue_ *mediaQueue) {
    if(mediaQueue == NULL || isEmpty(mediaQueue)) {
        return 0;
    }
    else {
        queueNode_ *queueNode = mediaQueue -> head;
        while(queueNode != NULL) {
            if( queueNode -> data.used == 0 && queueNode -> next != NULL ) {
                queueNode = queueNode -> next;
            }
            else if( queueNode -> data.used == 0 && queueNode -> next == NULL ) {
                return 0;
            }
            else if(queueNode -> data.used > 0) {
                rtpPacket_ *rtpPacket = (rtpPacket_*) queueNode -> data.packetData;
                return rtpPacket -> rtpHeader.sequenceNum;
            }
        }

        printf("totally fucked up\n");
        exit(1);
    }
}

int updateMediaQueue(queue_ *mediaQueue , queue_ *emptyQueue , uint32_t currentTS , unsigned maxTime) {
    if ((mediaQueue == NULL) || (emptyQueue == NULL)) {
        exit(1);
    }
    else {
        if( (currentTS == 0) || (isEmpty(mediaQueue)) ) {
            return -1;
        }

        int deleteCounter = 0;
        queueNode_ *queueNode = mediaQueue -> head;

        while(queueNode != NULL) {
            if(queueNode -> data.used == 0) {
                /*it's a reserved node*/
                int reservedCounter = nextPacketTooLate(queueNode , currentTS , maxTime);
                if(reservedCounter == 0) {
                    /*not too late*/
                    return deleteCounter;
                }
                else {
                    /*too late, consecutive node: reservedCounter*/
                    for (int i = 0 ; i < reservedCounter ; i++) {
                        queueNode = queue_dequeue(mediaQueue);
                        freeNodeToEmptyQueue(emptyQueue , queueNode);
                        deleteCounter++;
                    }
                    queueNode = mediaQueue -> head;
                }
            }
            else {
                /*it's a packet node*/
                int lateFlag = isTooLate(getNodeRTPts(queueNode) , currentTS , maxTime);
                if(lateFlag) {
                    /*too late, dequeue*/
                    queueNode = queue_dequeue(mediaQueue);
                    freeNodeToEmptyQueue(emptyQueue , queueNode);
                    deleteCounter++;
                    queueNode = mediaQueue -> head;
                }
                else {
                    /*not too late*/
                    return deleteCounter;
                }
            }
        }

        return deleteCounter;
    }
}

int updateFECqueue(queue_ *fecQueue , queue_ *emptyQueue , uint16_t minSN) {
    if ( (fecQueue == NULL) || (emptyQueue == NULL) ) {
        exit(1);
    }
    else {
        if( (minSN == 0) || (isEmpty(fecQueue)) ) {
            return -1;
        }

        int deleteCounter = 0;
        queueNode_ *queueNode = fecQueue -> head;

        while(queueNode != NULL) {
            fecPacket_ *fecPacket = (fecPacket_*) queueNode -> data.packetData;

            if(fecPacket -> fecHeader.SNBase < minSN) {
                queueNode = queue_dequeue(fecQueue);
                freeNodeToEmptyQueue(emptyQueue , queueNode);
                deleteCounter++;

                queueNode = fecQueue -> head;
            }
            else {
                return deleteCounter;
            }
        }

        return deleteCounter;
    }
}
