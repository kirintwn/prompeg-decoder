#ifndef INCLUDED_PACKETBUFFER_H
#define INCLUDED_PACKETBUFFER_H

#include <iostream>
#include <stdint.h>
#include <string.h>
#include "packetQueue.h"

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
            if( (currentSN - 1) > lastSN && lastSN != 0 ) {
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
            if(mediaSSRC == 0) {
                mediaSSRC = ntohl(rtpPacket -> rtpHeader.ssrc);
            }

            node *temp = emptyQueue -> dequeue();
            temp -> dataUsed = length;
            temp -> next = NULL;
            memcpy(temp -> dataBuffer , buffer , length);
            mediaQueue -> enqueue(temp);
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
        void updateMediaQueue(int sockfd , int maxTimeRange) {
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
                        int lateFlag = temp -> isTsToolate(currentTS , maxTimeRange);
                        if(lateFlag == 0) {
                            return;
                        }
                        else {
                            temp = mediaQueue -> dequeue();
                            if (send(sockfd , temp -> dataBuffer , temp -> dataUsed , 0) == -1);
                                //perror("send");
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
        void fecRecovery(int times) {
            for(int i = 0 ; i < times ; i++) {
                if(fecQueue -> isEmpty()) {
                    return;
                }
                else {
                    int FINflag = 0;
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
                        else if(recoverFlag == 1) {
                            //recover and delete
                            recoverPacket(temp);
                            recovered++;
                            FINflag++;

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
                        else if(recoverFlag > 1 || recoverFlag == -1) {
                            tempPrev = temp;
                            if(!temp -> next) {
                                tempNext = NULL;
                                break;
                            }
                            else {
                                tempNext = temp -> next;
                                temp = tempNext;
                                continue;
                            }
                        }
                    }
                    
                    if(FINflag == 0) {
                        return;
                    }
                }
            }
        }
        void recoverPacket(node *fecNode) {
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
        }
        void xor_slow(uint8_t *in1, uint8_t *in2, uint8_t *out, int size) {
            for (int i = 0 ; i < size ; i++) {
                out[i] = in1[i] ^ in2[i];
            }
        }
};

#endif
