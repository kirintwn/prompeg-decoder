#include <iostream>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "packetProcessor.h"

using namespace std;

class streamCounter {
    public:
        int recvd;
        int lost;
        uint16_t lastestSN;
        streamCounter() {
            recvd = 0;
            lost = 0;
            lastestSN = 0;
        }
        ~streamCounter(){};
        void printStatus() {
            printf("    recvd: %d , lost: %d , loss rate: %lf\n", recvd , lost , ( 100*lost / (double)(lost+recvd)));
        }
        void updateStatus(const void *buffer) {
            rtpHeader_ *rtpHeader = (rtpHeader_ *)buffer;
            uint16_t thisSN = ntohs(rtpHeader -> sequenceNum);

            if( lastestSN > 0 && (thisSN - 1) > lastestSN ) {
                int lostCounter = (thisSN - lastestSN) -1;
                lost+= lostCounter;
            }

            recvd++;
            lastestSN = thisSN;
            return;
        }
};

class monitor {
    public:
        streamCounter *media;
        streamCounter *fecRow;
        streamCounter *fecCol;
        int recovered;
        monitor() {
            media = new streamCounter();
            fecRow = new streamCounter();
            fecCol = new streamCounter();
            recovered = 0;
        }
        ~monitor(){};
        void printMonitor() {
            printf("media stream:\n");
            media -> printStatus();
            printf("    recovered/lost: %d/%d = %lf\n" , recovered , media -> lost , (100*recovered / (double)media -> lost));
            printf("fecRow stream:\n");
            fecRow -> printStatus();
            printf("fecCol stream:\n");
            fecCol -> printStatus();
            printf("\n");
        }
};
