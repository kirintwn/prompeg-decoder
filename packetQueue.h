#include <iostream>
#include <stdint.h>
#include <string.h>
#include <vector>
#include "socketConnection.h"
#include "monitor.h"

using namespace std;

class node {
    public:
        //////////////////////////////////
        int dataSize;
        int dataUsed;
        unsigned char *dataBuffer;
        node *next;
        //////////////////////////////////
        node() {
            dataSize = 2048;
            dataUsed = 0;
            next = NULL;
            dataBuffer = (unsigned char*) malloc(2048 * sizeof(uint8_t));
        }
        ~node() {};
        uint32_t getTS() {
            if(dataUsed == 0) {
                return 0;
            }
            else {
                rtpPacket_ *rtpPacket = (rtpPacket_ *) dataBuffer;
                return ntohl(rtpPacket -> rtpHeader.ts);
            }
        }
        uint16_t getSN() {
            if(dataUsed == 0) {
                return 0;
            }
            else {
                rtpPacket_ *rtpPacket = (rtpPacket_ *) dataBuffer;
                return ntohs(rtpPacket -> rtpHeader.sequenceNum);
            }
        }

        uint16_t getSNBase() {
            if(dataUsed != 1344) {

                printf("getSNBase: %d\n" , dataUsed);
                exit(1);
            }
            else {
                fecPacket_ *fecPacket = (fecPacket_ *) dataBuffer;
                return ntohs(fecPacket -> fecHeader.SNBase);
            }
        }
        uint8_t getOffset() {
            if(dataUsed != 1344) {
                printf("getOffset\n");
                exit(1);
            }
            else {
                fecPacket_ *fecPacket = (fecPacket_ *) dataBuffer;
                return fecPacket -> fecHeader.offset;
            }
        }
        uint8_t getNA() {
            if(dataUsed != 1344) {
                printf("getNA\n");
                exit(1);
            }
            else {
                fecPacket_ *fecPacket = (fecPacket_ *) dataBuffer;
                return fecPacket -> fecHeader.NA;
            }
        }
        int isTsToolate(uint32_t currentTS , int maxTimeRange) {
            if(currentTS == 0) {
                return 0;
            }
            else {
                uint32_t thisTS = getTS();
                if(thisTS + maxTimeRange < currentTS) {
                    return 1;
                }
                else {
                    return 0;
                }
            }
        }
        int isPacketToolate(uint32_t currentTS , int maxTimeRange) {
            if(dataUsed > 0) {
                return isTsToolate(currentTS , maxTimeRange);
            }
            else {
                int counter = 1;
                node *temp = this;
                while(temp -> next) {
                    temp = temp -> next;
                    if(temp -> dataUsed > 0) {
                        if(isTsToolate(currentTS , maxTimeRange)) {
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
                printf("fucked up\n");
                exit(1);
            }
        }
};

class queue {
    public:
        //////////////////////////////////
        node *head , *tail;
        int size;
        //////////////////////////////////
        queue() {
            head = NULL;
            tail = NULL;
            size = 0;
        }
        ~queue() {};

        int isEmpty() {
            if(size == 0) {
                return 1;
            }
            else {
                return 0;
            }
        }
        void enqueue(node *target) {
            if(target == NULL) {
                printf("target is NULL\n");
                exit(1);
            }
            else {
                target -> next = NULL;
                if(isEmpty()) {
                    head = target;
                }
                else {
                    tail -> next = target;
                }
                tail = target;
                size++;
                return;
            }
        }
        node *dequeue() {
            if(isEmpty()) {
                return NULL;
            }
            else {
                node* res = head;
                if(head -> next) {
                    head = head -> next;
                }
                else {
                    head = NULL;
                }
                size--;
                res -> next = NULL;
                return res;
            }
        }
};

class packetBuffer {
    public:
        //////////////////////////////////
        queue *emptyQueue;
        queue *mediaQueue;
        queue *fecQueue;

        uint16_t minSN;
        uint16_t lastSN;
        uint32_t currentTS;
        uint32_t mediaSSRC;

        node *sendIndex;
        int sendIndexAtTail;
        int recovered;
        //////////////////////////////////
        packetBuffer() {
            emptyQueue = new queue();
            mediaQueue = new queue();
            fecQueue = new queue();

            for(int i = 0 ; i < 2048 ; i++) {
                node* temp = new node();
                emptyQueue -> enqueue(temp);
            }

            minSN = 0;
            lastSN = 0;
            currentTS = 0;
            mediaSSRC = 0;
            sendIndex = NULL;
            sendIndexAtTail = 0;
            recovered = 0;
        };
        packetBuffer(int quantity) {
            emptyQueue = new queue();
            mediaQueue = new queue();
            fecQueue = new queue();

            for(int i = 0 ; i < quantity ; i++) {
                node* temp = new node();
                emptyQueue -> enqueue(temp);
            }

            minSN = 0;
            lastSN = 0;
            currentTS = 0;
            mediaSSRC = 0;
            sendIndex = NULL;
            sendIndexAtTail = 0;
            recovered = 0;
        };
        ~packetBuffer() {};

        void freeNodeToEmptyQueue(node *target) {
            if(target == NULL) {
                printf("target is NULL\n");
                exit(1);
            }
            else {
                target -> next = NULL;
                target -> dataUsed = 0;
                emptyQueue -> enqueue(target);
            }
        }
        void newMediaPacket(const void *buffer, size_t length) {
            rtpPacket_ *rtpPacket = (rtpPacket_*) buffer;
            uint16_t currentSN = ntohs(rtpPacket -> rtpHeader.sequenceNum);

            if( mediaQueue -> isEmpty() || lastSN == 0 || minSN == 0 || (minSN > currentSN) ) {
                minSN = currentSN;
            }
            else if( (currentSN - 1) > lastSN ) {
                int emptyCounter = (currentSN - lastSN) - 1;
                for(int i = 0 ; i < emptyCounter ; i++) {
                    node *temp = emptyQueue -> dequeue();
                    temp -> dataUsed = 0;
                    temp -> next = NULL;
                    mediaQueue -> enqueue(temp);
                }
            }

            lastSN = currentSN;
            currentTS = ntohl(rtpPacket -> rtpHeader.ts);

            node *temp = emptyQueue -> dequeue();
            temp -> dataUsed = length;
            temp -> next = NULL;
            memcpy(temp -> dataBuffer , buffer , length);
            mediaQueue -> enqueue(temp);

            if(mediaSSRC == 0) {
                mediaSSRC = ntohl(rtpPacket -> rtpHeader.ssrc);
            }
        }
        void newFecPacket(const void *buffer, size_t length) {
            node *temp = emptyQueue -> dequeue();
            temp -> dataUsed = length;
            temp -> next = NULL;
            memcpy(temp -> dataBuffer , buffer , length);
            fecQueue -> enqueue(temp);
        }
        void updateFecQueue() {
            if( minSN == 0 || fecQueue -> isEmpty() ) {
                return;
            }
            else {
                node *temp = fecQueue -> head;
                node *tempPrev = NULL;
                node *tempNext = NULL;
                temp = fecQueue -> head;

                while(temp) {
                    if( (temp -> getSNBase()) < minSN ) {
                        if(!temp -> next) {
                            tempPrev = tempPrev;
                            tempNext = NULL;
                            fecDeleteNode(temp , tempPrev , NULL);
                            break;
                        }
                        else {
                            tempPrev = tempPrev;
                            tempNext = temp -> next;
                            fecDeleteNode(temp , tempPrev , tempNext);
                            temp = tempNext;
                            continue;
                        }
                    }
                    else {
                        if(!temp -> next) {
                            break;
                        }
                        else {
                            tempPrev = temp;
                            tempNext = temp -> next;
                            temp = tempNext;
                            continue;
                        }
                    }
                }

                return;
            }
        }
        void fecDeleteNode(node *target , node *targetPrev , node *targetNext) {
            if(!targetPrev && targetNext) {
                fecQueue -> head = targetNext;
            }
            else if(!targetPrev && !targetNext) {
                fecQueue -> head = NULL;
                fecQueue -> tail = NULL;
            }
            else if(targetPrev && targetNext) {
                targetPrev -> next = targetNext;
            }
            else if(targetPrev && !targetNext) {
                targetPrev -> next = NULL;
                fecQueue -> tail = targetPrev;
            }
            fecQueue -> size--;
            freeNodeToEmptyQueue(target);
        }
        void mediaSender(int output_Sockfd , int maxTimeRange) {
            if( mediaQueue -> isEmpty()) {
                sendIndex = NULL;
                return;
            }
            else if( !(sendIndex) && (mediaQueue -> head) ) {
                sendIndex = mediaQueue -> head;
            }
            else if( sendIndexAtTail && !(sendIndex -> next) ) {
                sendIndex = sendIndex;
                return;
            }
            else if( sendIndexAtTail && (sendIndex -> next) ) {
                sendIndex = sendIndex -> next;
            }
            else {
                sendIndex = sendIndex;
            }

            while(sendIndex) {
                if(sendIndex -> dataUsed == 0) {
                    //check next non-empty node's TS
                    int emptyCounter = sendIndex -> isPacketToolate(currentTS , maxTimeRange);
                    if(emptyCounter == 0) {
                        sendIndexAtTail = 0;
                        sendIndex = sendIndex;
                        break;
                    }
                    else {
                        for (int i = 0 ; i < emptyCounter ; i++) {
                            sendIndex = sendIndex -> next;
                        }
                    }
                }

                //printf("Sending Packet SN: %d\n", sendIndex -> getSN());
                if (send(output_Sockfd , sendIndex -> dataBuffer , sendIndex -> dataUsed , 0) == -1);
                    //perror("send");

                if(sendIndex -> next) {
                    sendIndex = sendIndex -> next;
                    sendIndexAtTail = 0;
                }
                else if(sendIndex == mediaQueue -> tail) {
                    sendIndex = sendIndex;
                    sendIndexAtTail = 1;
                    break;
                }
                else {
                    printf("mediaQueue management fucked up!\n");
                    exit(1);
                }

            }

        }
        void updateMediaQueue(int maxTimeRange) {
            if( currentTS == 0 || mediaQueue -> isEmpty() ) {
                return;
            }
            else {
                node *temp = mediaQueue -> head;
                while(temp) {
                    if(temp -> dataUsed == 0) {
                        int emptyCounter = temp -> isPacketToolate(currentTS , maxTimeRange);
                        if(emptyCounter == 0) {
                            return;
                        }
                        else {
                            for (int i = 0 ; i < emptyCounter ; i++) {
                                temp = mediaQueue -> dequeue();
                                freeNodeToEmptyQueue(temp);
                            }
                            temp = mediaQueue -> head;
                        }
                    }
                    else {
                        int lateFlag = temp -> isPacketToolate(currentTS , maxTimeRange);
                        if(lateFlag == 0) {
                            return;
                        }
                        else {
                            temp = mediaQueue -> dequeue();
                            freeNodeToEmptyQueue(temp);
                            temp = mediaQueue -> head;
                        }
                    }
                }

                return;
            }
        }
        void updateMinSN() {
            node *temp = mediaQueue -> head;
            while(temp) {
                if(temp -> dataUsed == 0 && temp -> next) {
                    temp = temp -> next;
                }
                else if( temp -> dataUsed == 0 && !(temp -> next) ) {
                    minSN = 0;
                    return;
                }
                else if(temp -> dataUsed > 0) {
                    minSN = temp -> getSN();
                    return;
                }
            }
        }
        void bufferMonitor() {
            printf("emptyQueue size: %d\n", emptyQueue -> size);
            printf("mediaQueue size: %d\n", mediaQueue -> size);
            printf("fecQueue size: %d\n\n", fecQueue -> size);
        }
        int isRecoverable(uint16_t SNBase , uint8_t offset , uint8_t NA) {
            node *temp = mediaQueue -> head;
            int emptyCounter = 0;
            if(temp) {
                int differ = SNBase - temp -> getSN();
                if(differ < 0) {
                    //printf("gg , differ < 0: %d\n" , differ);
                    printf("SNBase:%d , minSN:%d , headSN:%d\n", SNBase , minSN , temp->getSN());
                    exit(1);
                }
                else {
                    for(int i = 0 ; i < differ ; i++) {
                        if(temp -> next) {
                            temp = temp -> next;
                        }
                        else {
                            return -1;
                        }
                    }

                    if(temp -> dataUsed == 0) {
                        emptyCounter++;
                    }

                    for(int i = 0 ; i < NA - 1 ; i++) {
                        for(int j = 0 ; j < offset ; j++) {
                            if(temp -> next) {
                                temp = temp -> next;
                            }
                            else {
                                return -1;
                            }
                        }
                        if(temp -> dataUsed == 0) {
                            emptyCounter++;
                        }
                    }
                    return emptyCounter;
                }
            }
            return -1;
        }
        void fecRecovery(int output_Sockfd) {
            if(fecQueue -> isEmpty()) {
                return;
            }
            else {
                node *temp = fecQueue -> head;
                node *tempPrev = NULL;
                node *tempNext = NULL;
                while(temp && temp -> dataUsed > 0) {
                    int recoverFlag = isRecoverable(temp -> getSNBase() , temp -> getOffset() , temp -> getNA());
                    if(recoverFlag == 0) {
                        //delete
                        if(!temp -> next) {
                            tempPrev = tempPrev;
                            tempNext = NULL;
                            fecDeleteNode(temp , tempPrev , NULL);
                            return;
                        }
                        else {
                            tempPrev = tempPrev;
                            tempNext = temp -> next;
                            fecDeleteNode(temp , tempPrev , tempNext);
                            temp = tempNext;
                            continue;
                        }
                    }
                    else if(recoverFlag == 1) {
                        //recover and delete
                        recoverPacket(output_Sockfd , temp);
                        recovered++;

                        if(!temp -> next) {
                            tempPrev = tempPrev;
                            tempNext = NULL;
                            fecDeleteNode(temp , tempPrev , NULL);
                            return;
                        }
                        else {
                            tempPrev = tempPrev;
                            tempNext = temp -> next;
                            fecDeleteNode(temp , tempPrev , tempNext);
                            temp = tempNext;
                            continue;
                        }
                    }
                    else if(recoverFlag > 1 || recoverFlag == -1) {
                        tempPrev = temp;
                        if(!temp -> next) {
                            tempNext = NULL;
                            return;
                        }
                        else {
                            tempNext = temp -> next;
                            temp = tempNext;
                            continue;
                        }
                    }
                }
                return;
            }
        }
        void recoverPacket(int output_Sockfd , node *fecNode) {
            fecPacket_ *fecPacket = (fecPacket_ *) fecNode -> dataBuffer;

            node *target = NULL;
            uint16_t targetSN = 0;
            //fecPacket -> fecHeader.tsRecovery
            //fecPacket -> payload

            node *temp = mediaQueue -> head;
            int differ = fecNode -> getSNBase() - temp -> getSN();
            for(int i = 0 ; i < differ ; i++) {
                temp = temp -> next;
            }

            if(temp -> dataUsed == 0) {
                target = temp;
                targetSN = fecNode -> getSNBase();
            }
            else {
                //xor
                rtpPacket_ *tempRTP = (rtpPacket_ *) temp -> dataBuffer;
                fecPacket -> fecHeader.tsRecovery ^= tempRTP -> rtpHeader.ts;
                xor_slow(fecPacket -> payload , tempRTP -> payload , fecPacket -> payload , 1316);
            }

            for(int i = 0 ; i < fecNode -> getNA() - 1 ; i++) {
                for(int j = 0 ; j < fecNode -> getOffset() ; j++) {
                    temp = temp -> next;
                }
                if(temp -> dataUsed == 0) {
                    target = temp;
                    targetSN = fecNode -> getSNBase() + (i+1)*(fecNode -> getOffset());
                }
                else {
                    //xor
                    rtpPacket_ *tempRTP = (rtpPacket_ *) temp -> dataBuffer;
                    fecPacket -> fecHeader.tsRecovery ^= tempRTP -> rtpHeader.ts;
                    xor_slow(fecPacket -> payload , tempRTP -> payload , fecPacket -> payload , 1316);
                }
            }

            target -> dataUsed = 1328;
            rtpPacket_ *rtpPacket = (rtpPacket_ *) target -> dataBuffer;
            rtpPacket -> rtpHeader.cc = 0;
            rtpPacket -> rtpHeader.extension = 0;
            rtpPacket -> rtpHeader.padding = 0;
            rtpPacket -> rtpHeader.version = 2;
            rtpPacket -> rtpHeader.pt = 33;
            rtpPacket -> rtpHeader.marker = 0;
            rtpPacket -> rtpHeader.sequenceNum = htons(targetSN);
            rtpPacket -> rtpHeader.ts = fecPacket -> fecHeader.tsRecovery;
            rtpPacket -> rtpHeader.ssrc = htonl(mediaSSRC);
            memcpy ( rtpPacket -> payload , fecPacket -> payload , 1316 );

            if (send(output_Sockfd , target -> dataBuffer , target -> dataUsed , 0) == -1);
                //perror("send");
        }
        void xor_slow(uint8_t *in1, uint8_t *in2, uint8_t *out, int size) {
            for (int i = 0 ; i < size ; i++) {
                out[i] = in1[i] ^ in2[i];
            }
        }
};
