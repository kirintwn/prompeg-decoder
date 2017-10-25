#include <iostream>
#include <stdint.h>
#include <string.h>
#include "packetProcessor.h"

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
        node *sendIndex;
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
            sendIndex = NULL;
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
            sendIndex = NULL;
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
        }
        void newFecPacket(const void *buffer, size_t length) {
            node *temp = emptyQueue -> dequeue();
            temp -> dataUsed = length;
            temp -> next = NULL;
            memcpy(temp -> dataBuffer , buffer , length);
            fecQueue -> enqueue(temp);
        }
        void updateFecQueue() {
            if( (minSN == 0) || (fecQueue -> isEmpty()) ) {
                return;
            }
            else {
                node *temp = fecQueue -> head;
                while(temp) {
                    fecPacket_ *fecPacket = (fecPacket_*) temp -> dataBuffer;
                    uint16_t tempSNBase = ntohs(fecPacket -> fecHeader.SNBase);

                    if(tempSNBase < minSN) {
                        temp = fecQueue -> dequeue();
                        freeNodeToEmptyQueue(temp);
                        temp = fecQueue -> head;
                    }
                    else {
                        return;
                    }
                }
                return;
            }
        }
        void bufferMonitor() {
            printf("emptyQueue size: %d\n", emptyQueue -> size);
            printf("mediaQueue size: %d\n", mediaQueue -> size);
            printf("fecQueue size: %d\n\n", fecQueue -> size);
        }
};
