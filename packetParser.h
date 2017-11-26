#ifndef INCLUDED_PACKETPARSER_H
#define INCLUDED_PACKETPARSER_H

#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#define CHECK_BIT(var, pos) !!((var) & (1 << (pos)))

using namespace std;


/*
    ==RTP packet==
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |V=2|P|X|  CC   |M|     PT      |       sequence number         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       timestamp(4bytes)                       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |       synchronization source (SSRC) identifier (fixed)        |
    +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    |                       payload(1316bytes)                      |
    |                             ....                              |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
-------------------------------------------------------------------------------
    ==FEC packet==
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                      RTP header(12bytes)                      |
    |                             ....                              |
    |                             ....                              |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |         SNBase low bits       |        Length Recovery        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |E| PT recovery |                    Mask                       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                           TS recovery                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |X|D|type |index|    Offset     |       NA      |SNBase ext bits|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       payload(1316bytes)                      |
    |                             ....                              |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
typedef struct rtpHeader_ {
    unsigned int cc:4;        /* CSRC count */
    unsigned int extension:1; /* header extension flag */
    unsigned int padding:1;   /* padding flag */
    unsigned int version:2;   /* protocol version */

    unsigned int pt:7;        /* payload type */
    unsigned int marker:1;    /* marker bit */

    uint16_t sequenceNum;     /* sequence number */            /*IMPORTANT*/
    uint32_t ts;              /* timestamp */                  /*IMPORTANT*/
    uint32_t ssrc;            /* synchronization source */     /*GLOBAL INFO*/
} __attribute__((packed)) rtpHeader_;

typedef struct fecHeader_ {
    uint16_t SNBase;          /*SNBase Low bit*/               /*IMPORTANT*/
    uint16_t lengthRecovery;  /*length recovery*/              /*always same*/

    unsigned int ptRecovery:7;/*payload type recovery*/        /*always same*/
    unsigned int extension:1; /*extension*/                    /*always same*/

    unsigned int mask:24;     /*mask bit*/                     /*always same*/

    uint32_t tsRecovery;      /*timestamp recovery*/           /*IMPORTANT*/

    unsigned int index:3;     /*index*/                        /*always same*/
    unsigned int type:3;      /*FEC type*/                     /*always same*/
    unsigned int dimension:1; /*dimension, Col=0 Row=1*/       /*區分row,col*/
    unsigned int x:1;         /*X bit*/                        /*always same*/

    uint8_t offset;           /*offset, Col=L Row=1*/
    uint8_t NA;               /*NA, Col=D Row=L*/
    uint8_t SNBaseExtension;  /*SNBase extension bit*/         /*always same*/
} __attribute__((packed)) fecHeader_;

typedef struct rtpPacket_ {
    rtpHeader_ rtpHeader;
    uint8_t payload[1316];
} __attribute__((packed)) rtpPacket_; /*1328 bytes*/

typedef struct fecPacket_ {
    rtpHeader_ rtpHeader;
    fecHeader_ fecHeader;
    uint8_t payload[1316];
} __attribute__((packed)) fecPacket_; /*1344 bytes*/

#endif
