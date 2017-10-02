#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define CHECK_BIT(var, pos) !!((var) & (1 << (pos)))
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
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |         SNBase low bits       |        Length Recovery        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |E| PT recovery |                    Mask                       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                           TS recovery                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |X|D|type |index|    Offset     |       NA      |SNBase ext bits|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    :                          FEC Payload                          :
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef struct rtpPacket {
    unsigned int version:2;     /* protocol version */              /*always same*/
    unsigned int padding:1;     /* padding flag */                  /*always same*/
    unsigned int rtpExtension:1;/* header extension flag */         /*always same*/
    unsigned int cc:4;          /* CSRC count */                    /*always same*/
    unsigned int marker:1;      /* marker bit */                    /*always same*/
    unsigned int pt:7;          /* payload type */                  /*GLOBAL INFO*/
    uint16_t sequenceNum;       /* sequence number */               /*IMPORTANT*/
    uint32_t ts;                /* timestamp */                     /*IMPORTANT*/
    uint32_t ssrc;              /* synchronization source */        /*GLOBAL INFO*/
    uint8_t payload[1316];
} __attribute__((packed)) rtpPacket; /*1328 bytes*/

typedef struct fecPacket {
    uint16_t SNBase;            /*SNBase Low bit*/                  /*IMPORTANT*/
    uint32_t tsRecovery;        /*timestamp recovery*/              /*IMPORTANT*/
    unsigned int dimension:1;   /*dimension, Col=0 Row=1*/          /*區分row,col*/
    uint8_t offset;             /*offset, Col=L Row=1*/             /**/
    uint8_t NA;                 /*NA, Col=D Row=L*/                 /**/
    uint8_t payload[1316];
} fecPacket;

typedef struct rtpNode {
    rtpPacket packetContent;
    rtpNode* next;
} rtpNode;

typedef struct fecNode {
    fecPacket packetContent;
    fecNode* next;
} fecNode;

typedef struct myBuffer {
    int isFirstMedia;           /*Initialize: 1*/
    uint16_t rtp_VPXCCMPT;      /*Initialize: 0b1000000000100001*/
    uint32_t MEDIA_PARAM_SSRC;  /*Initialize: 0*/

    rtpNode* rtpList_HEAD;
    fecNode* fecList_HEAD;
} myBuffer;
