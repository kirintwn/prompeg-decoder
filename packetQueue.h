#ifndef INCLUDED_PACKETQUEUE_H
#define INCLUDED_PACKETQUEUE_H

#include <iostream>
#include <stdint.h>
#include <string.h>
#include "packetParser.h"

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
                uint32_t thisTS = this -> getTS();
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
                printf("gg: isPacketToolate()\n");
                exit(1);
            }
            else {
                int counter = 1;
                node *temp = this;
                while(temp -> next) {
                    temp = temp -> next;
                    if(temp -> dataUsed > 0) {
                        if(temp -> isTsToolate(currentTS , maxTimeRange)) { //bug resolved!
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

#endif
